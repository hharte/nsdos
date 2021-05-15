/*
 * Utility to list directory contents of North Star DOS floppy disks,
 * and optionally extract the files.
 *
 * www.github.com/hharte/nsdos
 *
 * (c) 2021, Howard M. Harte
 *
 * Reference: North Star System Software Manual Rev 2.1
 *
 */

#define _CRT_SECURE_NO_DEPRECATE

#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define NS_BLOCK_SIZE		(256)
#define DIR_ENTRIES_DD		(16 * 8)
#define DIR_ENTRIES_SD		(16 * 4)
#define SNAME_LEN	8
#define DOUBLE_DENSITY_FLAG	(0x80)

#define FILE_TYPE_DEFAULT		(0)
#define FILE_TYPE_OBJECT		(1)
#define FILE_TYPE_BASIC_SRC		(2)
#define FILE_TYPE_BASIC_DATA	(3)

const char* file_type_str[] = {
	"Default      ",
	"Object Code  ",
	"BASIC Program",
	"BASIC Data   ",
};

/* North Star DOS Directory Entry */
typedef struct ns_dir_entry {
	char sname[SNAME_LEN];
	uint16_t disk_address;
	uint16_t block_count;
	uint8_t file_type;
	uint8_t type_dependent_info[3];
} ns_dir_entry_t;

/* Function prototypes */
int ns_read_dir_entries(FILE* stream, ns_dir_entry_t* dir_entries);
void ns_list_dir_entry(ns_dir_entry_t* dir_entry);
int ns_extract_file(ns_dir_entry_t* dir_entry, FILE* instream, char *path);

#if defined(_WIN32)
#define strncasecmp(x,y,z) _strnicmp(x,y,z)
#endif

int main(int argc, char *argv[])
{
	FILE* instream;
	ns_dir_entry_t *dir_entry_list;
	int dir_entry_cnt;
	int i;
	int result;
	int extracted_file_count = 0;
	int status = 0;

	printf("North Star DOS File Utility (c) 2021 - Howard M. Harte\n\n");

	if (argc < 2) {
		printf("usage is: %s <filename.nsi> [command] [<filename>|<path>]\n", argv[0]);
		printf("\t<filename.nsi> North Star DOS Disk Image in .nsi format.\n");
		printf("\t[command]      LI - List files\n");
		printf("\t               EX - Extract files to <path>\n");
		printf("If no command is given, LIst is assumed.\n");
		return (-1);
	}

	if (!(instream = fopen(argv[1], "rb"))) {
		fprintf(stderr, "Error Openening %s\n", argv[1]);
		return (-ENOENT);
	}

	dir_entry_list = (ns_dir_entry_t*)calloc(DIR_ENTRIES_DD, sizeof(ns_dir_entry_t));

	if (dir_entry_list == NULL) {
		fprintf(stderr, "Memory allocation of %d bytes failed\n", (int)(DIR_ENTRIES_DD * sizeof(ns_dir_entry_t)));
		status = -ENOMEM;
		goto exit_main;
	}

	dir_entry_cnt = ns_read_dir_entries(instream, dir_entry_list);

	if (dir_entry_cnt == 0) {
		fprintf(stderr, "File not found\n");
		status = -ENOENT;
		goto exit_main;
	}

	/* Parse the command, and perform the requested action. */
	if ((argc == 2) || (!strncasecmp(argv[2], "LI", 2))) {
		printf("Filename  DA BLKS D TYP Type          Metadata\n");

		for (i = 0; i < dir_entry_cnt; i++) {
			ns_list_dir_entry(&dir_entry_list[i]);
		}
	} else {
		if (argc < 4) {
			printf("filename required.\n");
			status = -1;
			goto exit_main;
		} else if (!strncasecmp(argv[2], "EX", 2)) {
			for (i = 0; i < dir_entry_cnt; i++) {
				result = ns_extract_file(&dir_entry_list[i], instream, argv[3]);
				if (result == 0) {
					extracted_file_count++;
				}
			}
			printf("Extracted %d files.\n", extracted_file_count);
		}
	}

exit_main:
	if (dir_entry_list) free(dir_entry_list);

	if (instream != NULL) fclose(instream);

	return status;
}

int ns_read_dir_entries(FILE* stream, ns_dir_entry_t* dir_entries)
{
	int dir_entry_max = DIR_ENTRIES_SD;
	int dd_flag;
	int dir_entry_index = 0;
	ns_dir_entry_t* dir_entry;

	for (int i = 0; i < dir_entry_max; i++) {
		size_t readlen;
		dir_entry = &dir_entries[dir_entry_index];
		readlen = fread(dir_entry, sizeof(ns_dir_entry_t), 1, stream);

		if (readlen == 1) {
			if (memcmp(dir_entry->sname, "        ", SNAME_LEN)) {

				/* Bit 7 of the file type indicates double-density */
				dd_flag = (dir_entry->file_type & DOUBLE_DENSITY_FLAG);

				if (dd_flag) {
					dir_entry->block_count *= 2;
					dir_entry_max = DIR_ENTRIES_DD;
				}

				dir_entry_index++;
			}
		}
	}
	return dir_entry_index;
}

void ns_list_dir_entry(ns_dir_entry_t* dir_entry)
{
	char fname[SNAME_LEN + 1];

	strncpy(fname, dir_entry->sname, sizeof(fname));
	fname[SNAME_LEN] = '\0';
	if (strncmp(fname, "        ", SNAME_LEN)) {
		int dd_flag;
		uint8_t file_type;

		/* Bit 7 of the file type indicates double-density */
		dd_flag = (dir_entry->file_type & DOUBLE_DENSITY_FLAG);
		file_type = dir_entry->file_type & ~(DOUBLE_DENSITY_FLAG);

		printf("%s %3d  %3d %c %3d ", fname, dir_entry->disk_address, dir_entry->block_count, dd_flag ? 'D' : ' ', file_type);

		if (file_type < (sizeof(file_type_str) / sizeof(file_type_str[0]))) {
			printf("%s", file_type_str[file_type]);
		}
		else {
			printf("(Unknown)    ");
		}

		switch (file_type) {
		case FILE_TYPE_OBJECT:
			/* Binary object file, loaded with "GO," type-dependent data contains the load address */
			printf(" Load addr: %04X\n", dir_entry->type_dependent_info[0] | (dir_entry->type_dependent_info[1] << 8));
			break;
		case FILE_TYPE_BASIC_SRC:
			/* BASIC Source code, type-dependent data contains the actual program size in blocks */
			printf(" Actual Size: %d\n", dir_entry->type_dependent_info[0]);
			break;
		default:
			printf(" %02X,%02X,%02X\n", dir_entry->type_dependent_info[0], dir_entry->type_dependent_info[1], dir_entry->type_dependent_info[2]);
			break;
		}
	}
}

int ns_extract_file(ns_dir_entry_t* dir_entry, FILE* instream, char *path)
{
	uint8_t file_type;
	char fname[SNAME_LEN + 1];
	int dd_flag;

	strncpy(fname, dir_entry->sname, sizeof(fname));
	fname[SNAME_LEN] = '\0';

	if (!strncmp(fname, "        ", SNAME_LEN)) {
		return (-ENOENT);
	}

	/* Truncate the filename if a space is encountered. */
	for (int j = 0; j < strlen(fname); j++) {
		if (fname[j] == ' ') fname[j] = '\0';
	}

	/* Bit 7 of the file type indicates double-density */
	dd_flag = (dir_entry->file_type & DOUBLE_DENSITY_FLAG);
	file_type = dir_entry->file_type &= ~(DOUBLE_DENSITY_FLAG);

	if ((dir_entry->block_count > 0) && (dir_entry->disk_address > 0)) {
		FILE* ostream;
		int file_offset;
		uint8_t* file_buf;
		int file_len;
		char output_filename[32];
		
		file_len = dir_entry->block_count * NS_BLOCK_SIZE;

		switch (file_type) {
		case FILE_TYPE_DEFAULT:
			snprintf(output_filename, sizeof(output_filename), "%s/%s.DEFAULT", path, fname);
			break;
		case FILE_TYPE_OBJECT:
			snprintf(output_filename, sizeof(output_filename), "%s/%s.OBJECT_L%04X", path, fname,
				dir_entry->type_dependent_info[0] | (dir_entry->type_dependent_info[1] << 8));
			break;
		case FILE_TYPE_BASIC_SRC:
			file_len = dir_entry->type_dependent_info[0] * NS_BLOCK_SIZE;
			snprintf(output_filename, sizeof(output_filename), "%s/%s.BASIC", path, fname);
			break;
		case FILE_TYPE_BASIC_DATA:
			snprintf(output_filename, sizeof(output_filename), "%s/%s.BASIC_DATA", path, fname);
			break;
		default:
			snprintf(output_filename, sizeof(output_filename), "%s/%s.TYPE_%d", path, fname,
				file_type);
			break;
		}

		output_filename[sizeof(output_filename) - 1] = '\0';
		if (!(ostream = fopen(output_filename, "wb"))) {
			printf("Error Openening %s\n", output_filename);
			return (-ENOENT);
		} else if ((file_buf = (uint8_t*)calloc(1, file_len))) {
			file_offset = dir_entry->disk_address * NS_BLOCK_SIZE * (dd_flag ? 2 : 1);
			printf("%8s -> %s (%d bytes)\n", fname, output_filename, file_len);

			fseek(instream, file_offset, SEEK_SET);
			fread(file_buf, file_len, 1, instream);
			fwrite(file_buf, file_len, 1, ostream);
			free(file_buf);
			fclose(ostream);
			return (0);
		} else {
			printf("Memory allocation of %d bytes failed\n", file_len);
			return (-ENOMEM);
		}
	}

	return (-ENOENT);
}
