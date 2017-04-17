#include "syscalls.h"

#include "paging.h"
#include "rofs.h"
#include "rtc.h"
#include "terminal.h"
#include "x86_desc.h"

// All file ops
fileops_t stdin_ops = {terminal_open, terminal_close, terminal_read, fail};
fileops_t stdout_ops = {terminal_open, terminal_close, fail, terminal_write};
fileops_t rtc_ops = {rtc_open, rtc_close, rtc_read, rtc_write};
fileops_t dir_ops = {fail, fail, fail, fail};
fileops_t file_ops = {file_open, file_close, file_read, fail};
fileops_t fail_ops = {fail, fail, fail, fail};

uint8_t processes_flags = 0;
uint8_t keyboard_flag = 0;
pcb_t *current_pcb = NULL;

int32_t halt(uint8_t status) {
    int i;
    pcb_t *pcb = get_current_pcb();
    uint32_t esp_temp = pcb->parent->esp;
    uint32_t ebp_temp = pcb->parent->ebp;

    //current = pcb_new->parent;
    remap(VIRTUAL_START, PHYSICAL_START + pcb->parent->pid * FOUR_MB_BLOCK);
    tss.esp0 = PHYSICAL_START - pcb->parent->pid * EIGHT_KB_BLOCK - MAGIC_SIZE;
    tss.ss0 = KERNEL_DS;

    for(i = 2; i < MAX_FILES; i++)
    {
        if((pcb->files[i].flags) & FILE_OPEN)
        {
            pcb->files[i].fileops.close(i);
        }
    }

    processes_flags &= ~pcb->pid;

    asm volatile("movl %0, %%eax \n\t"
                 "movl %1, %%esp \n\t"
                 "movl %2, %%ebp \n\t"
                 "jmp HALT_RET_LABEL \n\t"
                 :
                 : "r"((uint32_t)status), "r"(esp_temp), "r"(ebp_temp)
                 : "%eax", "%esp", "%ebp"
             );
    return status;
}

int32_t execute(const int8_t *command) {
    uint32_t esp_temp;
    uint32_t esb_temp;
    uint32_t com_start;
    int8_t com_buf[COMMAND_SIZE];
    int8_t arg_buf[COMMAND_SIZE];
    uint8_t buffer[MAGIC_SIZE];
    uint8_t i;
    int arg_start;
    int arg_finish;

    // Copy over esp and esb
    asm volatile("movl %%esp,%0 \n\t"
                 "movl %%ebp,%0 \n\t"
                 : "=r"(esp_temp), "=r"(esb_temp)
                 );

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
        arg_buf[arg_finish] = (int8_t)command[i];
    }
    arg_buf[arg_finish] = '\0';

    // Read the file
    dentry_t dentry;
    if (read_dentry_by_name(com_buf, &dentry)) {
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
    //memcpy((uint8_t *)EXECUTE_START, buffer, PROCESS_SIZE);
    read_data(dentry.inode_num, 0, buffer, inodes[dentry.inode_num].length);

    // Set up flags
    uint32_t temp_ds = USER_DS;
    uint32_t temp_cs = USER_CS;
    uint32_t temp_ret = 0;
    uint32_t temp_eip = com_start;
    tss.ss0 = KERNEL_DS;
    tss.esp0 = PHYSICAL_START - pcb_new->pid * EIGHT_KB_BLOCK - MAGIC_SIZE;

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
                  : "=r"(temp_ret)
                  :"r"(temp_eip), "r"(temp_cs), "r"(temp_ds), "r"(USER_STACK)
                  :"%eax"
    );

    return temp_ret;
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

int32_t get_new_pid () {
    int i;
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (processes_flags & (1 << i)) {
            continue;
        }

        processes_flags |= (1 << i);
        return i;
    }

    return -1;
}

pcb_t *create_pcb() {
    int32_t pid = get_new_pid();
    if (pid < 0) {
        return NULL;
    }

    pcb_t *pcb = (pcb_t *) (PHYSICAL_START - (EIGHT_KB_BLOCK * (pid + 1)));
    pcb->pid = pid;

    pcb->parent = current_pcb;
    if (pcb->parent == NULL) {
        pcb->parent = pcb;
    }

    current_pcb = pcb;

    pcb->files[0].fileops = stdin_ops;
    pcb->files[0].flags = FILE_OPEN;
    pcb->files[0].pos = 0;

    pcb->files[1].fileops = stdout_ops;
    pcb->files[1].flags = FILE_OPEN;
    pcb->files[1].pos = 0;

    return pcb;
}

pcb_t *get_current_pcb() {
    int32_t esp;
    asm volatile (
        "mov %%ESP, %0"
        : "=c"(esp)
    );
    return (pcb_t *)(esp & PCB_MASK);
}
