/*
* paging.c - initializes paging
*/

#include "paging.h"
#include "types.h"

#define ARR_SIZE 1024
#define FOUR_KB 4096
#define FOUR_MB 4194304
#define VID_MEM_LOC 0xB8
#define RW_FLAGS 0x87
#define PAGE_DIR_FLAGS 0x83
#define RWP_FLAGS 7

//global arrays
uint32_t pageDir[ARR_SIZE] __attribute__((aligned(FOUR_KB)));
uint32_t pageTable[ARR_SIZE] __attribute__((aligned(FOUR_KB)));
uint32_t userTable[ARR_SIZE] __attribute__((aligned(FOUR_KB)));
uint32_t videoTable[ARR_SIZE] __attribute__((aligned(FOUR_KB)));

/*
* Function: init_paging()
* Description: Maps kernal memory and video memory, sets the rest not present.
* Inputs: none
* Outputs: none
*/
void init_paging()
{
  int i;
  for(i = 0; i <  ARR_SIZE; i++)
  {
    //set the read/write flag to allow reading and writing
    pageDir[i] = 0x00000002;

    //sets 12 bit to i and 0-11 are zero'd. Set r/w flag
    pageTable[i] = (FOUR_KB * i) | 2;
  }

  //add page table to the directory and set r/w and present flags
  pageDir[0] = (unsigned int) pageTable | 3;

  //map kernal block (4 MB), set size, rw, and present flags
  pageDir[1] = FOUR_MB | PAGE_DIR_FLAGS;

  //page table entry for video memory
  pageTable[VID_MEM_LOC] |= 3;

  //turn on paging
  asm volatile(
             "movl %0, %%eax;"
             "movl %%eax, %%cr3;"
             "movl %%cr4, %%eax;"
             "orl $0x00000010, %%eax;"
             "movl %%eax, %%cr4;"
             "movl %%cr0, %%eax;"
             "orl $0x80000000, %%eax;"
             "movl %%eax, %%cr0;"
             :
             :"r"(pageDir)
             :"%eax"
             );
}


/*
* Function: remap
* Description: Maps physical and virtual pages
* Inputs: vAddr - virtual address to be mapped
*         pAddr - physical address to be mapped
* Outputs: none
*/
void remap(uint32_t vAddr, uint32_t pAddr)
{
  //page directory entry (divide by 4MB)
  uint32_t entry = vAddr / FOUR_MB;
  //sets size, user, present, r/w  flags
  pageDir[entry] = pAddr | RW_FLAGS;
  //refresh
  refresh_tbl();
}

/*
* Function: remapWithPageTable
* Description: Maps 4MB chunk of memory vAddr to the first page of the user page table
* Inputs: vAddr - virtual address to be mapped
*         pAddr - physical address to be mapped
* Outputs: none
*/
void remapWithPageTable(uint32_t vAddr, uint32_t pAddr)
{
  //page directory entry (divide by 4MB)
  uint32_t entry = vAddr / FOUR_MB;
  //sets user level, read/write, present flags
  pageDir[entry] = ((unsigned int)userTable) | RWP_FLAGS;
  //sets user, read/write, present flags
  userTable[0] = pAddr | RWP_FLAGS;
  //refresh
  refresh_tbl();
}

/*
* Function: remapVideoWithPageTable
* Description: Maps 4MB chunk of memory vAddr to the first page of the video page table
* Inputs: vAddr - virtual address to be mapped
*         pAddr - physical address to be mapped
* Outputs: none
*/
void remapVideo(uint32_t vAddr, uint32_t pAddr)
{
  //page directory entry (divide by 4MB)
  uint32_t entry = vAddr / FOUR_MB;
  //sets user level, read/write, present flags
  pageDir[entry] = ((unsigned int)videoTable) | RWP_FLAGS;
  //sets user, read/write, present flags
  videoTable[0] = pAddr | RWP_FLAGS;
  refresh_tbl();
}

/*
* Function: remapWithPageTableToPage
* Description: Maps 4MB chunk of memory at the vAddr to the specified page of the user page table
* Inputs: vAddr - virtual address to be mapped
*         pAddr - physical address to be mapped
*         page - page desired for maping
* Outputs: none
*/
void remapToPage(uint32_t vAddr, uint32_t pAddr, uint32_t page)
{
  //page directory entry
  uint32_t entry = vAddr / FOUR_MB;
  //sets user level, read/write, present flags
  pageDir[entry] = ((unsigned int)userTable) | RWP_FLAGS;
  //sets user, read/write, present flags
  userTable[page] = pAddr | RWP_FLAGS;
  //refresh
  refresh_tbl();
}


/*
* Function: refresh_tbl
* Description: Refreshes the tbl
* Inputs: none
* Outputs: none
*/
void refresh_tbl(void){
	asm volatile("mov %%cr3, %%eax;"
               "mov %%eax, %%cr3;"
               :
               :
               :"%eax"
               );
}
