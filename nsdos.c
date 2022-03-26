/*
 * Utility to list directory contents of North Star DOS floppy disks,
 * and optionally extract the files.
 *
 * www.github.com/hharte/nsdos
 *
 * Copyright (c) 2021, Howard M. Harte
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
#include "./nsdos.h"

const char kPathSeparator =
#ifdef _WIN32
    '\\';
#else  /* ifdef _WIN32 */
    '/';
#endif /* ifdef _WIN32 */

const char *file_type_str[] = {
    "Default      ",
    "Object Code  ",
    "BASIC Program",
    "BASIC Data   ",
    "Backup Index ",
    "Backup Data  ",
    "CPM Workfile ",
    "CPM Unit     "
};

int main(int argc, char *argv[]) {
    FILE *instream;
    ns_dir_entry_t *dir_entry_list;
    nsdos_args_t    args;
    int positional_arg_cnt;
    int dir_entry_cnt;
    int i;
    int result;
    int extracted_file_count = 0;
    int status               = 0;

    positional_arg_cnt = parse_args(argc, argv, &args);

    if (positional_arg_cnt == 0) {
        printf("North Star DOS File Utility (c) 2021 - Howard M. Harte\n");
        printf("https://github.com/hharte/nsdos\n\n");

        printf("usage is: %s <filename.nsi> [command] [<filename>|<path>] [-q] [-b=n]\n", argv[0]);
        printf("\t<filename.nsi> North Star DOS Disk Image in .nsi format.\n");
        printf("\t[command]      LI - List files\n");
        printf("\t               EX - Extract files to <path>\n");
        printf("\tFlags:\n");
        printf("\t      -q       Quiet: Don't list file details during extraction.\n");
        printf("\t      -d=n     Directory block count=n: limit directory size to n blocks.\n");
        printf("\n\tIf no command is given, LIst is assumed.\n");
        return -1;
    }

    if (!(instream = fopen(args.image_filename, "rb"))) {
        fprintf(stderr, "Error Openening %s\n", argv[1]);
        return -ENOENT;
    }

    dir_entry_list = (ns_dir_entry_t *)calloc(DIR_ENTRIES_DD, sizeof(ns_dir_entry_t));

    if (dir_entry_list == NULL) {
        fprintf(stderr, "Memory allocation of %d bytes failed\n", (int)(DIR_ENTRIES_DD * sizeof(ns_dir_entry_t)));
        status = -ENOMEM;
        goto exit_main;
    }

    dir_entry_cnt = ns_read_dir_entries(instream, dir_entry_list, args.dir_block_cnt);

    if (dir_entry_cnt == 0) {
        fprintf(stderr, "File not found\n");
        status = -ENOENT;
        goto exit_main;
    }

    /* Parse the command, and perform the requested action. */
    if ((positional_arg_cnt == 1) | (!strncasecmp(args.operation, "LI", 2))) {
        printf("Filename  DA BLKS D TYP Type          Metadata\n");

        for (i = 0; i < dir_entry_cnt; i++) {
            ns_list_dir_entry(&dir_entry_list[i]);
        }

        if (dir_entry_cnt > 0) {
            if ((dir_entry_cnt == 1) && (!memcmp(dir_entry_list[0].sname, "FORMAT  ", SNAME_LEN))) {
                fprintf(stderr,
                        "\nDisk is Lifeboat CP/M %s-density.\n",
                        dir_entry_list[0].file_type & DOUBLE_DENSITY_FLAG ? "double" : "single");
                status = -EPERM;
            } else {
                for (i = 0; i < dir_entry_cnt; i++) {
                    if (!memcmp(dir_entry_list[i].sname, "CPM DATA ", SNAME_LEN)) {
                        fprintf(stderr,
                                "\nDisk is Northstar CP/M %s-density.\n",
                                dir_entry_list[0].file_type & DOUBLE_DENSITY_FLAG ? "double" : "single");
                        status = -EPERM;
                    }
                }
            }
        }
    } else {
        if (positional_arg_cnt < 2) {
            fprintf(stderr, "filename required.\n");
            status = -EBADF;
            goto exit_main;
        } else if (!strncasecmp(args.operation, "EX", 2)) {
            for (i = 0; i < dir_entry_cnt; i++) {
                result = ns_extract_file(&dir_entry_list[i], instream, args.output_path, args.quiet);

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

int parse_args(int argc, char *argv[], nsdos_args_t *args) {
    int positional_arg_cnt = 0;

    memset(args, 0, sizeof(nsdos_args_t));

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            switch (positional_arg_cnt) {
                case 0:
                    snprintf(args->image_filename, sizeof(args->image_filename), "%s", argv[i]);
                    break;
                case 1:
                    snprintf(args->operation,      sizeof(args->operation),      "%s", argv[i]);
                    break;
                case 2:
                    snprintf(args->output_path,    sizeof(args->output_path),    "%s", argv[i]);
                    break;
            }
            positional_arg_cnt++;
        } else {
            char flag = argv[i][1];

            switch (flag) {
                case 'd':
                    args->dir_block_cnt = (unsigned int)atoi(&argv[i][3]);

                    if (args->dir_block_cnt > 8) {
                        printf("Warning: Invalid directory block count %d ignored.\n",
                               args->dir_block_cnt);

                        args->dir_block_cnt = 8;
                    }
                    break;
                case 'f':
                    args->force = 1;
                    break;
                case 'q':
                    args->quiet = 1;
                    break;
                default:
                    printf("Unknown option '-%c'\n", flag);
                    break;
            }
        }
    }
    return positional_arg_cnt;
}

int ns_read_dir_entries(FILE *stream, ns_dir_entry_t *dir_entries, int dir_block_cnt) {
    int dir_entry_max = DIR_ENTRIES_SD;
    int dd_flag;
    int dir_entry_index = 0;
    ns_dir_entry_t *dir_entry;

    if (dir_block_cnt > 0) {
        dir_entry_max = dir_block_cnt * DIR_ENTRIES_PER_BLK;
    }

    for (int i = 0; i < dir_entry_max; i++) {
        size_t readlen;
        dir_entry = &dir_entries[dir_entry_index];
        readlen   = fread(dir_entry, sizeof(ns_dir_entry_t), 1, stream);

        if (readlen == 1) {
            if (memcmp(dir_entry->sname, "        ", SNAME_LEN)) {
                /* Bit 7 of the file type indicates double-density */
                dd_flag = (dir_entry->file_type & DOUBLE_DENSITY_FLAG);

                if (dd_flag) {
                    dir_entry->block_count *= 2;

                    if (dir_block_cnt == 0) {
                        dir_entry_max = DIR_ENTRIES_DD;
                    }
                }

                dir_entry_index++;
            }
        }
    }
    return dir_entry_index;
}

void ns_list_dir_entry(ns_dir_entry_t *dir_entry) {
    char fname[SNAME_LEN + 1];

    snprintf(fname, sizeof(fname), "%s", dir_entry->sname);

    if (strncmp(fname, "        ", SNAME_LEN)) {
        int dd_flag;
        uint8_t file_type;

        /* Bit 7 of the file type indicates double-density */
        dd_flag   = (dir_entry->file_type & DOUBLE_DENSITY_FLAG);
        file_type = dir_entry->file_type & ~(DOUBLE_DENSITY_FLAG);

        printf("%s %3d  %3d %c %3d ", fname, dir_entry->disk_address, dir_entry->block_count, dd_flag ? 'D' : ' ', file_type);

        if (file_type < (sizeof(file_type_str) / sizeof(file_type_str[0]))) {
            printf("%s", file_type_str[file_type]);
        } else {
            printf("(Unknown)    ");
        }

        switch (file_type) {
            case FILE_TYPE_OBJECT:
                /* Binary object file, loaded with "GO," type-dependent data
                   contains the load address */
                printf(" Load addr: %04X\n", dir_entry->type_dependent_info[0] | (dir_entry->type_dependent_info[1] << 8));
                break;
            case FILE_TYPE_BASIC_SRC:
                /* BASIC Source code, type-dependent data contains the actual
                   program size in blocks */
                printf(" Actual Size: %d\n", dir_entry->type_dependent_info[0]);
                break;
            default:
                printf(" %02X,%02X,%02X\n",
                       dir_entry->type_dependent_info[0],
                       dir_entry->type_dependent_info[1],
                       dir_entry->type_dependent_info[2]);
                break;
        }
    }
}

int ns_extract_file(ns_dir_entry_t *dir_entry, FILE *instream, char *path, int quiet) {
    uint8_t file_type;
    char    dos_fname[SNAME_LEN + 1];
    char    fname[SNAME_LEN + 1];
    int     dd_flag;

    snprintf(dos_fname, sizeof(dos_fname), "%s", dir_entry->sname);

    if (!strncmp(dos_fname, "        ", SNAME_LEN)) {
        return -ENOENT;
    }

    /* Truncate the filename if a space is encountered. */
    for (unsigned int j = 0; j < strnlen(dos_fname, sizeof(dos_fname)); j++) {
        if (dos_fname[j] == ' ') dos_fname[j] = '\0';
    }

    snprintf(fname, sizeof(fname), "%s", dos_fname);

    /* Replace '/' with '-' in output filename. */
    char *current_pos = strchr(fname, '/');

    while (current_pos) {
        *current_pos = '-';
        current_pos  = strchr(current_pos, '/');
    }

    /* Replace '*' with 's' in output filename. */
    current_pos = strchr(fname, '*');

    while (current_pos) {
        *current_pos = 's';
        current_pos  = strchr(current_pos, '*');
    }

    /* Bit 7 of the file type indicates double-density */
    dd_flag   = (dir_entry->file_type & DOUBLE_DENSITY_FLAG);
    file_type = dir_entry->file_type &= ~(DOUBLE_DENSITY_FLAG);

    FILE *ostream;
    int   file_offset;
    uint8_t *file_buf;
    int  file_len;
    char output_filename[256];

    file_len = dir_entry->block_count * NS_BLOCK_SIZE;

    switch (file_type) {
        case FILE_TYPE_DEFAULT:
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.DEFAULT",      path, kPathSeparator, fname);
            break;
        case FILE_TYPE_OBJECT:
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.OBJECT_L%04X", path, kPathSeparator, fname,
                     dir_entry->type_dependent_info[0] | (dir_entry->type_dependent_info[1] << 8));
            break;
        case FILE_TYPE_BASIC_SRC:
            file_len = dir_entry->type_dependent_info[0] * NS_BLOCK_SIZE;
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.BASIC",        path, kPathSeparator, fname);
            break;
        case FILE_TYPE_BASIC_DATA:
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.BASIC_DATA",   path, kPathSeparator, fname);
            break;
        case FILE_TYPE_BACKUP_INDEX:
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.BACKUP_INDEX", path, kPathSeparator, fname);
            break;
        case FILE_TYPE_BACKUP_DATA:
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.BACKUP_INDEX", path, kPathSeparator, fname);
            break;
        case FILE_TYPE_CPM_WORKFILE:
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.CPM_WORKFILE", path, kPathSeparator, fname);
            break;
        case FILE_TYPE_CPM_UNIT:
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.CPM_UNIT",     path, kPathSeparator, fname);
            break;
        case FILE_TYPE_ASP_SEQ:
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.ASP_SEQ",      path, kPathSeparator, fname);
            break;
        case FILE_TYPE_ASP_RANDOM:
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.ASP_RANDOM",   path, kPathSeparator, fname);
            break;
        case FILE_TYPE_ASP_INDEX:
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.ASP_INDEX",    path, kPathSeparator, fname);
            break;
        default:
            snprintf(output_filename, sizeof(output_filename), "%s%c%s.TYPE_%d",      path, kPathSeparator, fname,
                     file_type);
            break;
    }

    output_filename[sizeof(output_filename) - 1] = '\0';

    if (!(ostream = fopen(output_filename, "wb"))) {
        printf("Error Openening %s\n", output_filename);
        return -ENOENT;
    } else if ((file_buf = (uint8_t *)calloc(1, file_len))) {
        file_offset = dir_entry->disk_address * NS_BLOCK_SIZE * (dd_flag ? 2 : 1);

        if (!quiet) printf("%8s -> %s (%d bytes)\n", dos_fname, output_filename, file_len);

        if (0 == fseek(instream, file_offset, SEEK_SET)) {
            if (1 == fread(file_buf, file_len, 1, instream)) {
                if (1 != fwrite(file_buf, file_len, 1, ostream)) {
                    printf("Error writing output file.\n");
                } else {}
            } else {
                printf("Error reading image file.\n");
            }
        } else {
            printf("Error: Failed to seek image file.\n");
        }
        free(file_buf);
        fclose(ostream);
        return 0;
    }

    printf("Memory allocation of %d bytes failed\n", file_len);
    fclose(ostream);
    return -ENOMEM;
}
