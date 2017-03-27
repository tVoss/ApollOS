#include "rofs.h"

#include "lib.h"

// FOR TESTING
#include "keyboard.h"

boot_block_t *boot_block;
inode_t *inodes;
data_block_t *data_blocks;

void init_rofs(void *base) {
    // Size is used multiple times
    uint32_t boot_block_size = sizeof(boot_block_t);

    // Boot block is at start of memory
    boot_block = (boot_block_t *) base;
    // Then inodes
    inodes = (inode_t *) (base + boot_block_size);
    // Then the data
    data_blocks = (data_block_t *) (base + boot_block_size + boot_block->num_inodes * sizeof(inode_t));
}

void list_all_files() {
    int i;
    int8_t buf[33];
    printf("Listing all files in filesys_img:\n");
    for (i = 0; i < boot_block->num_dir_entries; i++) {
        strncpy(buf, boot_block->dentries[i].file_name, 32);
        buf[32] = 0;
        printf("file_name: %s, file_type: %d, file_size: %d\n",
            buf,
            boot_block->dentries[i].file_type,
            inodes[boot_block->dentries[i].inode_num].length);
    }
}

int32_t read_dentry_by_name(const int8_t *fname, dentry_t *dentry) {
    int i;
    for (i = 0; i < boot_block->num_dir_entries; i++) {
        // If file names aren't equal, continue looking
        if (strncmp(fname, boot_block->dentries[i].file_name, 32)) {
            continue;
        }

        // Directory Entry has been found, copy it
        memcpy(dentry, &boot_block->dentries[i], sizeof(dentry_t));

        // Return Success
        return 0;
    }

    // Return Failure
    return -1;
}

int32_t read_dentry_by_index(uint32_t index, dentry_t *dentry) {
    if (index >= boot_block->num_dir_entries) {
        // Invalid index
        return -1;
    }

    // Copy the valid entry
    memcpy(dentry, &boot_block->dentries[index], sizeof(dentry_t));

    // Return Success
    return 0;
}

int32_t read_data(uint32_t inode, uint32_t offset, uint8_t *buf, uint32_t length) {
    if (inode >= boot_block->num_inodes) {
        // Invalid index
        return -1;
    }

    inode_t *node = &inodes[inode];
    if (offset > node->length) {
        // Invalid offset
        return -1;
    }

    if (offset + length > node->length) {
        // Length too long, just truncate
        length = node->length - offset;
    }

    int i;
    for (i = 0; i < length; i++) {
        int block = (i + offset) / 4096;
        int block_offset = (i + offset) % 4096;
        buf[i] = data_blocks[node->data_block_num[block]][block_offset];
    }

    return i;
}
