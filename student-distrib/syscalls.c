#include "syscalls.h"

#include "rofs.h"
#include "rtc.h"
#include "terminal.h"

fileops_t stdin_ops = {terminal_open, terminal_close, terminal_read, fail};
fileops_t stdout_ops = {terminal_open, terminal_close, fail, terminal_write};
fileops_t rtc_ops = {rtc_open, rtc_close, rtc_read, rtc_write};
fileops_t dir_ops = {fail, fail, fail, fail};
fileops_t file_ops = {fail, fail, fail, fail};
fileops_t fail_ops = {fail, fail, fail, fail};

int32_t halt(uint8_t status) {
    return -1;
}

int32_t execute(const int8_t *command) {
    return -1;
}

int32_t read(int32_t fd, void *buf, int32_t nbytes) {
    if (fd < 0 || fd >= MAX_FILES || buf == NULL) {
        return -1;
    }

    pcb_t *pcb = get_current_pcb(); // Get this somehow

    if (!(pcb->files[fd].flags & FILE_OPEN)) {
        // File has not been opened - invalid
        return -1;
    }

    return pcb->files[fd].fileops.read(fd, (int8_t *)buf, nbytes);
}

int32_t write(int32_t fd, const void *buf, int32_t nbytes) {
    if (fd < 0 || fd >= MAX_FILES || buf == NULL) {
        return -1;
    }

    pcb_t *pcb = get_current_pcb();

    if (!(pcb->files[fd].flags & FILE_OPEN)) {
        // File has not been opened - invalid
        return -1;
    }

    return pcb->files[fd].fileops.write(fd, (int8_t *)buf, nbytes);
}

int32_t open(const int8_t *filename) {
    dentry_t dentry;
    if (read_dentry_by_name(filename, &dentry)) {
        // File not found
        return -1;
    }

    pcb_t *pcb = get_current_pcb();

    int i;
    for (i = 0; i < MAX_FILES; i++) {
        if (!(pcb->files[i].flags & FILE_OPEN)) {
            // Open slot for file
            pcb->files[i].flags |= FILE_OPEN;
            pcb->files[i].pos = 0;
            break;
        }
    }

    if (i == MAX_FILES) {
        // 8 files are already open
        return -1;
    }

    switch ((file_type_t) dentry.file_type) {
        case rtc:
            pcb->files[i].fileops = rtc_ops;
            break;
        case dir:
            pcb->files[i].fileops = dir_ops;
            break;
        case file:
            pcb->files[i].fileops = file_ops;
            break;
        default:
            // Unknown filetype
            pcb->files[i].flags &= ~FILE_OPEN;
            return -1;
    }

    if (pcb->files[i].fileops.open(filename)) {
        // Error opening
        pcb->files[i].flags &= ~FILE_OPEN;
        return -1;
    }

    pcb->files[i].inode = dentry.inode_num;
    return 0;
}

int32_t close(int32_t fd) {
    if (fd < 0 || fd >= MAX_FILES) {
        return -1;
    }

    pcb_t *pcb = get_current_pcb();

    if (!(pcb->files[fd].flags & FILE_OPEN)) {
        // File isn't open, can't close
        return -1;
    }

    // Set closed flag
    pcb->files[fd].flags &= ~FILE_OPEN;

    // Close file
    return pcb->files[fd].fileops.close(fd);
}

int32_t getargs(int8_t *buf, int32_t nbytes) {
    if (buf == NULL) {
        return -1;
    }

    pcb_t *pcb = NULL;

    if (nbytes > MAX_ARGS_LENGTH) {
        nbytes = MAX_ARGS_LENGTH;
    }

    strncpy(buf, pcb->args, nbytes);

    return 0;
}

int32_t vidmap(uint8_t **screen_start) {
    return -1;
}

int32_t set_handler(int32_t signum, void *handler_address) {
    return -1;
}

int32_t sigreturn() {
    return -1;
}

int32_t fail() {
    return -1;
}

pcb_t *get_current_pcb() {
    return NULL;
}
