#ifndef ROFS_H_
#define ROFS_H_

#include "types.h"

typedef struct dentry {
    uint8_t file_name[32];
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

extern void init_rofs(void *base);
extern int32_t read_dentry_by_name(const uint8_t *fname, dentry_t *dentry);
extern int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry);
extern int32_t read_data(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length);

#endif
