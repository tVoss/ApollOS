#include "rofs.h"

#include "lib.h"

#include "syscalls.h"

boot_block_t *boot_block;
inode_t *inodes;
data_block_t *data_blocks;

/*
 * init_rofs(base)
 *
 * DESCRIPTION: Sets up the file system for reading
 *
 * INPUTS: 	base - the address of the start of the fs
 * OUTPUTS: none
 *
 * SIDE EFFECTS: Sets pointers to fs objects
 *
 */
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

/*
 * list_all_files()
 *
 * DESCRIPTION: Testing function that lists all the files in the fs
 *
 * INPUTS: 	none
 * OUTPUTS: none
 *
 * SIDE EFFECTS: Prints file names, types, and size to screen
 *
 */
void list_all_files() {
    int i;
    int8_t buf[33];
    printf("Listing all files in filesys_img:\n");
    for (i = 0; i < boot_block->num_dir_entries; i++) {
        strncpy(buf, boot_block->dentries[i].file_name, FILE_NAME_LENGTH);
        buf[32] = 0;
        printf("file_name: %s, file_type: %d, file_size: %d\n",
            buf,
            boot_block->dentries[i].file_type,
            inodes[boot_block->dentries[i].inode_num].length);
    }
}

/*
 * read_dentry_by_name(fname, dentry)
 *
 * DESCRIPTION: Attempts to read a directory entry on the file system by name
 *
 * INPUTS: 	fname - the name of the file
 * OUTPUTS: dentry - the populated directory entry
 *
 * RETURNS: -1 on error, 0 otherwise
 * SIDE EFFECTS: none
 */
int32_t read_dentry_by_name(const int8_t *fname, dentry_t *dentry) {
    int i;
    for (i = 0; i < boot_block->num_dir_entries; i++) {
        // If file names aren't equal, continue looking
        if (strncmp(fname, boot_block->dentries[i].file_name, FILE_NAME_LENGTH)) {
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

/*
 * read_dentry_by_index(index, dentry)
 *
 * DESCRIPTION: Attempts to read a directory entry on the file system by index
 *
 * INPUTS: 	index - the index of the file
 * OUTPUTS: dentry - the populated directory entry
 *
 * RETURNS: -1 on error, 0 otherwise
 * SIDE EFFECTS: none
 */
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

/*
 * read_data(inode, offset, buf, length)
 *
 * DESCRIPTION: Writes the data contained by the inode to a buffer
 *
 * INPUTS: 	inode - the inode index that contains data information
 *          offset - how many bytes from the start of the file to start reading
 *          length - the max amount of bytes to read
 * OUTPUTS: buf - A buffer where the data is placed
 *
 * RETURNS: -1 on error, otherwise the amount of bytes read
 * SIDE EFFECTS: none
 */
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
        int block = (i + offset) >> 12;             // >> 12 ~= / 4096
        int block_offset = (i + offset) & 0xFFF;    // & 0xFFF ~= % 4096
        buf[i] = data_blocks[node->data_block_num[block]][block_offset];
    }

    return i;
}

int32_t file_open(const int8_t *filename) {
    dentry_t dentry;
    if (read_dentry_by_name(filename, &dentry)) {
        return -1;
    }

    if (dentry.file_type != file) {
        return -1;
    }

    return 0;
}

int32_t file_close(int32_t fd) {
    if (fd < 0 || fd > MAX_FILES) {
        return -1;
    }
    return 0;
}

int32_t file_read(int32_t fd, void *buf, int32_t nbytes) {
    if (fd < 0 || fd > MAX_FILES) {
        return -1;
    }

    file_t *file = &get_current_pcb()->files[fd];
    int32_t bytes_read = read_data(file->inode, file->pos, (uint8_t *) buf, nbytes);

    if (bytes_read < 0) {
        return -1;
    }

    file->pos += bytes_read;

    return 0;
}

int32_t dir_open(const int8_t *filename) {
    dentry_t dentry;
    if (read_dentry_by_name(filename, &dentry)) {
        return -1;
    }

    if (dentry.file_type != dir) {
        return -1;
    }

    return 0;
}

int32_t dir_close(int32_t fd) {
    if (fd < 0 || fd > MAX_FILES) {
        return -1;
    }
    return 0;
}

int32_t dir_read(int32_t fd, void *buf, int32_t nbytes) {

    // See if we've reached the end of file
    file_t *file = &get_current_pcb()->files[fd];
    if (file->pos >= boot_block->num_dir_entries) {
        return 0;
    }

    // Get the dentry
    dentry_t *dentry = &boot_block->dentries[file->pos];

    // Copy number of bytes requested up to max file name length
    int32_t copy_length = nbytes > FILE_NAME_LENGTH ? FILE_NAME_LENGTH : nbytes;
    strncpy((int8_t *)buf, dentry->file_name, copy_length);

    // Increase the position and determine number of bytes copied
    file->pos++;
    int32_t bytes_read = strlen(dentry->file_name);
    return bytes_read > FILE_NAME_LENGTH ? FILE_NAME_LENGTH : bytes_read;
}
