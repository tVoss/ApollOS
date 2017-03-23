#include "rofs.h"

#include "lib.h"

boot_block_t *boot_block;
inode_t *inodes;
data_block_t *data_blocks;

void init_rofs(void *base) {
    uint32_t boot_block_size = sizeof(boot_block_t);

    boot_block = (boot_block_t *) base;
    inodes = (inode_t *) (base + boot_block_size);
    data_blocks = (data_block_t *) (base + boot_block_size + boot_block->num_inodes * sizeof(inode_t));
}

int32_t read_dentry_by_name(const uint8_t *fname, dentry_t *dentry) {
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

    int read_bytes;
    int cur_byte;
    int data_block_index;
    int data_block_num;
    int data_block_offset;
    for(read_bytes = 0, cur_byte = offset; read_bytes < length && cur_byte < inodes[inode].length; read_bytes++, cur_byte++) {
        data_block_index = cur_byte / 4096;
        data_block_num = inodes[inode].data_block_num[data_block_index];
        data_block_offset = cur_byte % 4096;
        buf[cur_byte] = data_blocks[data_block_num][data_block_offset];
    }

    return read_bytes;
}
