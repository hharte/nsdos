/*
 * North Star DOS filesystem definitions.
 *
 * www.github.com/hharte/nsdos
 *
 * Copyright (c) 2021, Howard M. Harte
 *
 * Reference: North Star System Software Manual Rev 2.1
 *
 */

#ifndef NSDOS_H_
#define NSDOS_H_

#include <stdio.h>
#include <stdint.h>

#define NS_BLOCK_SIZE           (256)
#define DIR_ENTRIES_PER_BLK     (16)
#define DIR_ENTRIES_DD          (DIR_ENTRIES_PER_BLK * 8)
#define DIR_ENTRIES_SD          (DIR_ENTRIES_PER_BLK * 4)
#define SNAME_LEN               (8)
#define DOUBLE_DENSITY_FLAG     (0x80)

#define FILE_TYPE_DEFAULT       (0)
#define FILE_TYPE_OBJECT        (1)
#define FILE_TYPE_BASIC_SRC     (2)
#define FILE_TYPE_BASIC_DATA    (3)
#define FILE_TYPE_BACKUP_INDEX  (4)
#define FILE_TYPE_BACKUP_DATA   (5)
#define FILE_TYPE_CPM_WORKFILE  (6)
#define FILE_TYPE_CPM_UNIT      (7)
#define FILE_TYPE_ASP_SEQ       (18)
#define FILE_TYPE_ASP_RANDOM    (19)
#define FILE_TYPE_ASP_INDEX     (20)


/* North Star DOS Directory Entry */
typedef struct ns_dir_entry {
    char     sname[SNAME_LEN];
    uint16_t disk_address;
    uint16_t block_count;
    uint8_t  file_type;
    uint8_t  type_dependent_info[3];
} ns_dir_entry_t;

typedef struct nsdos_args {
    char         image_filename[256];
    char         output_path[256];
    char         operation[8];
    unsigned int dir_block_cnt;
    int          force;
    int          quiet;
} nsdos_args_t;

/* Function prototypes */
int  parse_args(int argc, char *argv[], nsdos_args_t *args);
int  ns_read_dir_entries(FILE *stream, ns_dir_entry_t *dir_entries, int dir_block_cnt);
void ns_list_dir_entry(ns_dir_entry_t *dir_entry);
int  ns_extract_file(ns_dir_entry_t *dir_entry, FILE *instream, char *path, int quiet);

#if defined(_WIN32)
# define strncasecmp(x, y, z) _strnicmp(x, y, z)
#endif /* if defined(_WIN32) */

#endif  // NSDOS_H_
