#ifndef ROFS_H_
#define ROFS_H_

#include "types.h"

#define FILE_NAME_LENGTH 32

typedef enum file_type {
    rtc = 0,
    dir = 1,
    file = 2
} file_type_t;

typedef struct dentry {
    int8_t file_name[FILE_NAME_LENGTH];
    uint32_t file_type;
    uint32_t inode_num;
    uint8_t reserved[24];
} dentry_t;

typedef struct boot_block {
    uint32_t num_dir_entries;
    uint32_t num_inodes;
    uint32_t num_data_blocks;
    uint8_t reserved[52];
    dentry_t dentries[63];
} boot_block_t;

typedef struct inode {
    uint32_t length;
    uint32_t data_block_num[1023];
} inode_t;

typedef uint8_t data_block_t[4096];

extern boot_block_t *boot_block;
extern inode_t *inodes;
extern data_block_t *data_blocks;

void init_rofs(void *base);
// Helper function before ls is implemented
void list_all_files();
int32_t read_dentry_by_name(const int8_t *fname, dentry_t *dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length);

int32_t file_open(const int8_t *filename);
int32_t file_close(int32_t fd);
int32_t file_read(int32_t fd, void *buf, int32_t nbytes);

int32_t dir_open(const int8_t *filename);
int32_t dir_close(int32_t fd);
int32_t dir_read(int32_t fd, void *buf, int32_t nbytes);

#endif
