#ifndef SYSCALLS_H_
#define SYSCALLS_H_

#include "types.h"

#define MAX_FILES 8
#define MAX_ARGS_LENGTH 128
#define MAX_PROCESSES 2

// Flags
#define FILE_OPEN 0x00000001

typedef struct fileops {
    int32_t (*open) (const int8_t filename);
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
    uint8_t parent_pid;
    int8_t args[MAX_ARGS_LENGTH];
} pcb_t;

int32_t halt(uint8_t status);

int32_t execute(const int8_t *command);

int32_t read(int32_t fd, void *buf, int32_t nbytes);

int32_t write(int32_t fd, const void *buf, int32_t nbytes);

int32_t open(const int8_t *filename);

int32_t close(int32_t fd);

int32_t getargs(uint8_t *buf, int32_t nbytes);

int32_t vidmap(uint8_t **screen_start);

int32_t set_handler(int32_t signum, void *handler_address);

int32_t sigreturn();

int32_t fail();

pcb_t *get_current_pcb();


#endif
