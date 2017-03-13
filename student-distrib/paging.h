/*
*	paging.h - paging.c header file
*/
#include "types.h"

//global arrays
extern uint32_t pageDir[1024] __attribute__((aligned(4096)));
extern uint32_t pageTable[1024] __attribute__((aligned(4096)));

//functions (descriptions in .c file)
void init_paging();
void remap(uint32_t vAddr, uint32_t pAddr);
void remapWithPageTable(uint32_t vAddr, uint32_t pAddr);
void remapVideo(uint32_t vAddr, uint32_t pAddr);
void remapToPage(uint32_t vAddr, uint32_t pAddr, uint32_t page);
void refresh_tbl(void);
