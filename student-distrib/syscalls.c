#include "syscalls.h"

#include "paging.h"
#include "rofs.h"
#include "rtc.h"
#include "terminal.h"
#include "x86_desc.h"

// All file ops
fileops_t stdin_ops = {terminal_open, fail, terminal_read, fail};
fileops_t stdout_ops = {terminal_open, fail, fail, terminal_write};
fileops_t rtc_ops = {rtc_open, rtc_close, rtc_read, rtc_write};
fileops_t dir_ops = {dir_open, dir_close, dir_read, fail};
fileops_t file_ops = {file_open, file_close, file_read, fail};
fileops_t fail_ops = {fail, fail, fail, fail};

uint8_t processes_flags = 0;

/*
 * can_execute()
 *
 * DESCRIPTION: check whether another terminal can be opened
 *
 * INPUTS: none
 * OUTPUTS: none
 * SIDE EFFECTS: returns 0 if can't execute, and 1 if can execute
 *
*/
uint8_t can_execute() {
    int i;
    int total = 0;
    for (i = 0; i < 3; i++) {
        total += terminal[i].num_processes;
    }
    return total < MAX_PROCESSES;
}

/*
 * halt(uint8_t status)
 *
 * DESCRIPTION: terminates current process
 *
 * INPUTS: status
 * OUTPUTS: status
 * SIDE EFFECTS: restores values of parent process
 *
*/
int32_t halt(uint8_t status) {
    cli();

    int i;
    pcb_t *pcb = get_current_pcb();

    for(i = 2; i < MAX_FILES; i++)
    {
        if((pcb->files[i].flags) & FILE_OPEN)
        {
            pcb->files[i].fileops.close(i);
        }
    }

    // update number of processes running in current terminal
    terminal[term_cur-1].num_processes--;
    processes_flags &= ~(1 << pcb->pid);

    if (terminal[term_cur-1].num_processes == 0) {
        execute("shell");
    }

    if (processes_flags == 0) {
        // Nothing is running, fire up a shell
        execute("shell");
    }

    terminal[term_cur - 1].term_pid = pcb->parent_pid;
    remap(VIRTUAL_START, PHYSICAL_START + pcb->parent_pid * FOUR_MB_BLOCK);
    tss.esp0 = pcb->parent_esp;
    tss.ss0 = KERNEL_DS;

    asm volatile("movl %0, %%eax \n\t"
                 "movl %1, %%esp \n\t"
                 "movl %2, %%ebp \n\t"
                 "jmp HALT_RET_LABEL \n\t"
                 :
                 : "r"((uint32_t)status), "r"(pcb->parent_esp), "r"(pcb->parent_ebp)
                 : "%eax"
             );
    return status;
}

/*
 * execute(const int8_t *command)
 *
 * DESCRIPTION: load and execute a new program
 *
 * INPUTS: command
 * OUTPUTS: 0 on sucess, -1 on failure
 * SIDE EFFECTS: hands processor off to new program
 *
*/
int32_t execute(const int8_t *command) {
    cli();

    if (command == NULL) {
        sti();
        return -1;
    }

    uint32_t com_start;
    int8_t com_buf[COMMAND_SIZE] = {0};
    int8_t arg_buf[COMMAND_SIZE] = {0};
    uint8_t buffer[MAGIC_SIZE] = {0};
    uint8_t i;
    int arg_start;

    // Must clear arg_buf manually when called as interrupt
    memset(arg_buf, 0, COMMAND_SIZE);

    // Copy command
    for(i = 0; (command[i] != ' ') && (command[i] != '\0') && (command[i] != '\n'); i++)
    {
        com_buf[i] = (int8_t)command[i];
    }
    com_buf[i] = '\0';

    if (strncmp(com_buf, "exit", 4) == 0) {
        // We got exit passed to execute for some reason...
        halt(0);
        return 0;
    }

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

    // Check if max number of processes are being run
    if (!can_execute())
    {
        printf("Maximum number of processes reached\n");
        return 0;
    }

    // Copy all the arguments if we hit a space
    if (command[i] == ' ') {
        arg_start = i + 1;
        for (i = 0; command[i + arg_start] != '\0' && command[i + arg_start] != '\n'; i++) {
            arg_buf[i] = (int8_t)command[i + arg_start];
        }
        arg_buf[i] = '\0';
    }


    // Read first instruction
    read_data(dentry.inode_num, 24, buffer, MAGIC_SIZE);
    com_start = *((uint32_t*)buffer);

    // Create pcb
    pcb_t *pcb_new = create_pcb();
    if(pcb_new == NULL) {
        return -1;
    }

    // update number of processes running in current terminal
    terminal[term_cur-1].num_processes++;
    terminal[term_cur-1].term_pid = pcb_new->pid;
    asm volatile (
        "movl %%ebp, %0\n\t"
        "movl %%esp, %1\n\t"
        : "=r" (pcb_new->parent_ebp), "=r" (pcb_new->parent_esp)
    );

    // Map memory and move program code to execution start
    remap(VIRTUAL_START, PHYSICAL_START + pcb_new->pid * FOUR_MB_BLOCK);
    read_data(dentry.inode_num, 0, (uint8_t *) EXECUTE_START, FOUR_MB_BLOCK);

    // Set up flags
    tss.ss0 = KERNEL_DS;
    tss.esp0 = PHYSICAL_START - pcb_new->pid * EIGHT_KB_BLOCK - MAGIC_SIZE;

    //pcb_new->parent->esp = esp_temp;
    //pcb_new->parent->ebp = ebp_temp;
    strncpy(pcb_new->args, arg_buf, MAX_ARGS_LENGTH);

    // Context switch
    asm volatile (
        "movw $0x2B, %%ax\n\t"
        "movw %%ax, %%ds\n\t"
        "movl $0x83FFFFC, %%eax\n\t"
        "pushl $0x2B\n\t"
        "pushl %%eax\n\t"
        "pushfl\n\t"
        "popl %%edx\n\t"
        "orl $0x200, %%edx\n\t"
        "pushl %%edx\n\t"
        "pushl $0x23\n\t"
        "pushl %0\n\t"
        "iret\n\t"
        "HALT_RET_LABEL:\n\t"
        "leave\n\t"
        "ret\n\t"
        : // No outputs
        : "r" (com_start)
        : "%eax", "%edx"
    );

    return 0;
}

/*
 * read(int32_t fd, void *buf, int32_t nbytes)
 *
 * DESCRIPTION: read data
 *
 * INPUTS: fd - where to read data from
 *         buf - data
 *         nbytes - amount of data to read
 * OUTPUTS: 0 on sucess, -1 on failure
 * SIDE EFFECTS: reads data
 *
*/  
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

/*
 * write(int32_t fd, void *buf, int32_t nbytes)
 *
 * DESCRIPTION: writes data
 *
 * INPUTS: fd - where to write data to
 *         buf - data
 *         nbytes - amount of data to write
 * OUTPUTS: 0 on sucess, -1 on failure
 * SIDE EFFECTS: writes data
 *
*/
int32_t write(int32_t fd, const void *buf, int32_t nbytes) {
    if (fd < 0 || fd >= MAX_FILES || buf == NULL) {
        return -1;
    }

    pcb_t *pcb = get_current_pcb();

    if (!(pcb->files[fd].flags & FILE_OPEN)) {
        // File has not been opened - invalid
        return -1;
    }

    return pcb->files[fd].fileops.write(fd, buf, nbytes);
}

/*
 * open(const int8_t *filename)
 *
 * DESCRIPTION: access the filesystem
 *
 * INPUTS: filename
 * OUTPUTS: 0 on sucess, -1 on failure
 * SIDE EFFECTS: allocates fd, setup data to handle file type
 *
*/
int32_t open(const int8_t *filename) {
    if (filename == NULL) {
        return -1;
    }

    dentry_t dentry;
    if (read_dentry_by_name(filename, &dentry)) {
        // File not found
        return -1;
    }

    pcb_t *pcb = get_current_pcb();

    int i;
    for (i = 2; i < MAX_FILES; i++) {
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

    return i;
}

/*
 * close(int32_t fd)
 *
 * DESCRIPTION: closes file
 *
 * INPUTS: fd
 * OUTPUTS: 0 on sucess, -1 on failure
 * SIDE EFFECTS: deallocated fd, maing it available
 *
*/
int32_t close(int32_t fd) {
    if (fd < 0 || fd >= MAX_FILES) {
        return -1;
    }

    pcb_t *pcb = get_current_pcb();

    if (!(pcb->files[fd].flags & FILE_OPEN)) {
        // File isn't open, can't close
        return -1;
    }

    // Close file
    int32_t err = pcb->files[fd].fileops.close(fd);

    // Set closed flag on no error
    if (!err) {
        pcb->files[fd].flags &= ~FILE_OPEN;
    }

    return err;
}

/*
 * getargs(int8_t *buf, int32_t nbytes)
 *
 * DESCRIPTION: reads command line argument into user-level buffer
 *
 * INPUTS: buf - data to read
 *         nbytes - amount of data to read
 * OUTPUTS: 0 on sucess, -1 on failure
 * SIDE EFFECTS: argument stored as task data for loaded program
 *
*/
int32_t getargs(int8_t *buf, int32_t nbytes) {
    if (buf == NULL) {
        return -1;
    }

    pcb_t *pcb = get_current_pcb();

    if (strlen(pcb->args) == 0) {
        return -1;
    }

    if (nbytes > MAX_ARGS_LENGTH) {
        nbytes = MAX_ARGS_LENGTH;
    }

    memcpy(buf, pcb->args, nbytes);

    return 0;
}

/*
 * vidmap(uint8_t **screen_start)
 *
 * DESCRIPTION: maps video memory to virtual address
 *
 * INPUTS: screen_start
 * OUTPUTS: 0 on sucess, -1 on failure
 * SIDE EFFECTS: maps video memeory to pre-set virtual address
 *
*/
int32_t vidmap(uint8_t **screen_start) {
    if (screen_start == NULL || screen_start == (uint8_t**) FOUR_MB_BLOCK) {
        return -1;
    }
    // check if lcoation provided by user is valid
    /*if ((int32_t) screen_start < VIRTUAL_START || (int32_t) screen_start > VIRTUAL_END){
        return -1;
    }*/
    remapWithPageTable(VIRTUAL_END,VIDEOMEM);
    *screen_start = (uint8_t *) VIRTUAL_END;
    return 0;
}

/*
 * set_handler(int32_t signum, void *handler_address)
 *
 * for signal handling
 *
*/
int32_t set_handler(int32_t signum, void *handler_address) {
    return -1;
}

/*
 * sigreturn()
 *
 * for signal handling
 *
*/
int32_t sigreturn() {
    return -1;
}

/*
 * fail()
 *
 * DESCRIPTION: failure
 *
 * INPUTS: none
 * OUTPUTS: -1
 * SIDE EFFECTS: none
 *
*/
int32_t fail() {
    return -1;
}

/*
 * get_new_pid()
 *
 * DESCRIPTION: get pid 
 *
 * INPUTS: none
 * OUTPUTS: 0 on sucess, -1 on failure
 * SIDE EFFECTS: updates processes_flags
 *
*/
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

/*
 * create_pcb()
 *
 * DESCRIPTION: creates a new pcb 
 *
 * INPUTS: none
 * OUTPUTS: new pcb
 * SIDE EFFECTS: sets all relevant data for new pcb
 *
*/
pcb_t *create_pcb() {
    int32_t pid = get_new_pid();
    if (pid < 0) {
        return NULL;
    }

    pcb_t *pcb = get_pcb(pid);
    pcb->pid = pid;

    if (terminal[term_cur - 1].num_processes == 0) {
        pcb->parent_pid = pid;
    } else {
        pcb->parent_pid = terminal[term_cur - 1].term_pid;
    }

    pcb->files[0].fileops = stdin_ops;
    pcb->files[0].flags = FILE_OPEN;
    pcb->files[0].pos = 0;

    pcb->files[1].fileops = stdout_ops;
    pcb->files[1].flags = FILE_OPEN;
    pcb->files[1].pos = 0;

    int i;
    for (i = 2; i < MAX_FILES; i++) {
        pcb->files[i].fileops = fail_ops;
        pcb->files[i].flags = 0;
        pcb->files[i].inode = 0;
        pcb->files[i].pos = 0;
    }

    memset(pcb->args, 0, MAX_ARGS_LENGTH);

    return pcb;
}

/*
 * get_current_pcb()
 *
 * DESCRIPTION: gets the current pcb 
 *
 * INPUTS: none
 * OUTPUTS: current pcb
 * SIDE EFFECTS: gets current pcb
 *
*/
pcb_t *get_current_pcb() {
    pcb_t *pcb;
    asm volatile(
        "andl %%esp, %%eax\n\t"
        : "=a" (pcb)
        : "a" (PCB_MASK)
        : "cc"
    );
    return pcb;
}

/*
 * get_pcb(uint32_t pid)
 *
 * DESCRIPTION: get pcb
 *
 * INPUTS: pid - process identification number
 * OUTPUTS: physical memory of pcb
 * SIDE EFFECTS: none
 *
*/
pcb_t *get_pcb(uint32_t pid) {
    return (pcb_t *)(PHYSICAL_START - EIGHT_KB_BLOCK * (pid + 1));
}

uint8_t get_processes_flags()
{
  return processes_flags;
}
int8_t get_active_process()
{
  return active_process;
}
