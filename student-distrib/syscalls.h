#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include "types.h"

#define MAX_FILES 8
#define MAX_ARGS_LENGTH 128
#define MAX_PROCESSES_MASK 0x3F
#define MAX_PROCESSES 6

// Flags
#define FILE_OPEN 0x00000001

#define COMMAND_SIZE 128

#define MAGIC_SIZE 4
#define MAGIC0 0x7F
#define MAGIC1 0x45
#define MAGIC2 0x4C
#define MAGIC3 0x46

#define USER_STACK      0x83FFFFC
#define EIGHT_KB_BLOCK  0x2000
#define FOUR_MB_BLOCK   0x400000
#define EIGHT_MB_BLOCK  0x800000
#define PROCESS_SIZE    4096

#define VIRTUAL_START  0x8000000      // 128MB
#define VIRTUAL_END    0x8400000      // 132MB, end of 4 MB page
#define PHYSICAL_START 0x800000
#define EXECUTE_START  0x8048000

#define VIDEOMEM    0xB8000

#define PCB_MASK 0x00FFE000

typedef struct fileops {
    int32_t (*open) (const int8_t *filename);
    int32_t (*close) (int32_t fd);
    int32_t (*read) (int32_t fd, void *buf, int32_t nbytes);
    int32_t (*write) (int32_t fd, const void *buf, int32_t nbytes);
} fileops_t;

typedef struct file {
    int32_t inode;
    int32_t flags;
    int32_t pos;
    fileops_t fileops;
} file_t;

typedef struct pcb {
    file_t files[MAX_FILES];
    uint8_t pid;
    int8_t args[MAX_ARGS_LENGTH];
    uint32_t esp;
    uint32_t ebp;

    uint32_t parent_pid;
    uint32_t parent_esp;
    uint32_t parent_ebp;
} pcb_t;

int32_t halt(uint8_t status);

int32_t execute(const int8_t *command);

int32_t read(int32_t fd, void *buf, int32_t nbytes);

int32_t write(int32_t fd, const void *buf, int32_t nbytes);

int32_t open(const int8_t *filename);

int32_t close(int32_t fd);

int32_t getargs(int8_t *buf, int32_t nbytes);

int32_t vidmap(uint8_t **screen_start);

int32_t set_handler(int32_t signum, void *handler_address);

int32_t sigreturn();

int32_t fail();

pcb_t *create_pcb();

pcb_t *get_current_pcb();

pcb_t *get_pcb(uint32_t pid);


#endif
