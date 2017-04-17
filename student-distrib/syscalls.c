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
    pcb_t * pcb_new = get_addr();
    uint32_t esp_temp = pcb_new->parent->esp;
    uint32_t esb_temp = pcb_new->parent->esb;
    current = pcb_new->parent;
}

int32_t execute(const int8_t *command) {
    uint32_t esp_temp;
    uint32_t esb_temp;
    uint32_t com_start;
    uint8_t arg_buf[COMMAND_SIZE];
    uint8_t com_buf[COMMAND_SIZE];
    uint8_t buffer[MAGIC_SIZE];
    uint8_t i;
    int arg_start;
    int arg_finish;

    // Copy over esp and esb
    asm volatile("movl %%esp,%0 \n\t"
                 "movl %%ebp,%0 \n\t"
                 : "=r"(esp_temp), "=r"(esb_temp)
                 );

    if (processes_running > MAX) { //processes_running must be updated in create_pcb and halt.
        return -1;
    }

    // Copy command
    for(i = 0; (command[i] != ' ') && (command[i] != '\0') && (command[i] != '\n'); i++)
    {
        com_buf[i] = (int8_t)command[i];
    }
    com_buf[i] = '\0';

    // Copy all the arguments
    arg_start = i + 1;
    for(arg_finish = arg_start; (command[i] != '\0') && (command[i] != '\n'); arg_finish++)
    {
        arg_buf[j] = (int8_t)command[i];
    }
    arg_buf[arg_finish] = '\0';

    // Read the file
    dentry_t dentry;
    if (read_dentry_by_name((uint8_t*) com_buf, &dentry)) {
        return -1;
    }

    // Check ELF magic number
    read_data(dentry.inode_num, 0, buffer, MAGIC_SIZE);
    if(buffer[0] != MAGIC0 || buffer[1] != MAGIC1 || buffer[2] != MAGIC2 || buffer[3] != MAGIC3) {
        return -1;
    }

    // Read first instruction
    read_data(dentry.inode_num, 24, buffer, MAGIC_SIZE);
    com_start = *((uint32_t*)buffer);

    // Create pcb
    pcb_t *pcb_new = create_pcb();
    if(pcb_new == NULL) {
        return -1;
    }

    // Map memory and move program code to execution start
    remap(VIRTUAL_START, PHYSICAL_START + pcb_new->pid * FOUR_MB_BLOCK);
    memcpy(EXECUTE_START, buffer, PROCESS_SIZE);

    // Set up flags
    uint32_t ds_temp = USER_DS;
    uint32_t cs_temp = USER_CS;
    uint32_t ret_temp = 0;
    uint32_t eip_temp = com_start;
    tss.ss0 = KERNAL_DS;
    tss.esp0 = PHYSICAL_START - pcb_new->pid * KB8 - MAGIC_SIZE;
    pcb_new->parent->esp = esp_temp;
    pcb_new->parent->ebp = esb_temp;

    pcb_new->args = arg_buf;


    // Not sure
    if (strncmp("shell", (int8_t*)com_buf, COMMAND_SIZE) != 0) {
        keyboard_flag = 1;
    }

    // Context switch
    asm volatile ("xorl %%eax, %%eax \n\t" //clear eax
                  "movw $0x2B, %%ax \n\t"
                  "movw %%ax, %%ds \n\t"
                  "pushl %3 \n\t" //push SS
                  "pushl %4 \n\t" //push USER_STACK
                  "pushf \n\t" //push flags
                  "popl %%eax \n\t"
                  "orl  $0x200, %%eax \n\t"
                  "pushl %%eax \n\t"
                  "pushl %2 \n\t" //push CS
                  "pushl %1 \n\t" //push EIP
                  "IRET \n\t"
                  "HALT_RET_LABEL:  \n\t"
                  "movl %%eax, %0"
                  : "=r"(return_halt)
                  :"r"(eip), "r"(temp_cs), "r"(temp_ds), "r"(USER_STACK)
                  :"%eax"
    );


    return return_halt;
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
