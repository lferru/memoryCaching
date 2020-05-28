// Luis Ferrufino
// Project 3
// CS 367-002
// caching.c

#include <stdio.h>
#include <stdlib.h>
#include "memory_system.h"
/* Feel free to implement additional header files as needed */

// HELPER FUNCTION PROTOTYPES:

// The work for get_physical_address() and get_byte() has been divided into
//the following functions.

// lookUpTlb() takes in the vpn and vpo and looks up the TLB. It is called by the
//get_physical_address() function. It returns a physical address.
int lookUpTlb(int vpn, int vpo);

// lookUpPageTable() takes in the vpn, vpo, index, and tag, and then looks up
//the Page Table. It is called by the lookUpTlb() function when it misses.
int lookUpPageTable(int vpn, int vpo, int index, int tag);

// updatePageTable() takes in the vpn and ppn and then updates the page table.
//It is called by lookUpPageTable() after a miss has been resolved.
void updatePageTable(int vpn, int ppn);

// updateTlb() takes in the index, tag, and ppn and then updates the TLB. It is
//called by the lookUpPageTable() and lookUpTlb() after a miss by the TLB has 
//been resolved.
void updateTlb(int index, int tag, int ppn);

// lookUpCache() takes in a physical address and looks up the cache. It is called
//by get_byte(). It returns the requested byte.
int lookUpCache(int physAddr);

// updateCache() takes in an index, tag, and block of four bytes and updates the
//cache. It is called by the lookUpCache() function after a miss has been
//resolved. It returns an integer, 0 or 1, specifying which entry it was loaded
//into.
int updateCache(int index, int tag, int datum);

// readCacheBlock() takes in an index, an integer (0 or 1) specifying the entry,
//and an offset, and then returns the requested byte. It is called by
//lookUpCache().
int readCacheBlock(int index, int i, int offset);

// STRUCT DEFINITIONS:

// For each struct, the ppn's, tags, data, time stamps, and valid bits are 
//stored in parallel arrays. In addition, no pointers are used as all structs
//are instead global variables.

struct tlb {

  int ppnVector[16];
  int tagVector[16];
  int validVector[16];
};

struct tlb Tlb;

struct pageTable {

  int ppnVector[512];
  int validVector[512];
};

struct pageTable PageTable;

// each entry for the data is stored on a two-dimensional array of size two
//(for each entry) by thirty-two (for each line). The tags and valid bits
//for each entry are stored in a parallel fashion. timeStamps[] is a one-
//dimensional array because it only keeps track of which entry on a line is
//older (0 means the first entry is older, and 1 means the second entry is
//older).
struct cache {

  int data[32][2];
  int tags[32][2];
  int timeStamps[32];
  int validBits[32][2];
};

struct cache Cache;

// HELPER FUNCTION DEFINITIONS:
int lookUpTlb(int vpn, int vpo) {
  
  //Extract the index and tag:
  int index = ( vpn & 0x0000000f );
  int tag = ( vpn & 0x000001f0 ) >> 4;
 
  if ( Tlb.validVector[index] == 1 && Tlb.tagVector[index] == tag ) {
    
    //Hit: 
    int physAddr = ( Tlb.ppnVector[index] ) << 9;
    physAddr += vpo;
    log_entry(ADDRESS_FROM_TLB, physAddr);
    return physAddr;

  } else {
    
    //Miss, so look up the Page Table:
    int physAddr = ( lookUpPageTable(vpn, vpo, index, tag) );
    return physAddr;
  }
}

int lookUpPageTable(int vpn, int vpo, int index, int tag)
{

  if ( PageTable.validVector[vpn] == 1 ) {
   
    //Hit: 
    int physAddr = ( PageTable.ppnVector[vpn] ) << 9;
    physAddr += vpo;
    updateTlb(index, tag, PageTable.ppnVector[vpn]);
    log_entry(ADDRESS_FROM_PAGETABLE, physAddr);
    return physAddr;
  } else {
   
    //Miss: 
    int ppn = load_frame(vpn);
    int physAddr = ( ppn ) << 9;
    physAddr += vpo;
    log_entry(ADDRESS_FROM_PAGE_FAULT_HANDLER, physAddr);
    updatePageTable(vpn, ppn);
    updateTlb(index, tag, ppn);
    return physAddr;
  }
}

void updatePageTable(int vpn, int ppn) {

  PageTable.validVector[vpn] = 1;
  PageTable.ppnVector[vpn] = ppn;
}

void updateTlb(int index, int tag, int ppn) {

  Tlb.validVector[index] = 1;
  Tlb.tagVector[index] = tag;
  Tlb.ppnVector[index] = ppn;
}

int lookUpCache(int physAddr) {

  int tag = ( physAddr & 0x003ffe00 ) >> 9;
  int index = ( physAddr & 0x0000007c ) >> 2;
  int offset =  ( physAddr & 0x00000003 );

  for ( int i = 0; i < 2; i++ ) {
    
    // Look at both entries on the line specified by the index. If we never
    //get into this if block, then we have a miss:
    
    if ( Cache.validBits[index][i] == 1 && Cache.tags[index][i] == tag ) {
      //Hit: 
      int datum = readCacheBlock(index, i, offset);
      log_entry(DATA_FROM_CACHE, datum);
      return datum;
    }
  }
  
  //Miss:
  int newDataBlock = get_word(physAddr);
  int i = updateCache(index, tag, newDataBlock);
  int datum = readCacheBlock(index, i, offset);
  log_entry(DATA_FROM_MEMORY, datum);
  return datum;
}

int updateCache(int index, int tag, int datum) {
  
  // Here we go through the replacement algorithm:

  // Every time data is loaded onto an entry, we mark the unaffected entry as
  //the oldest one by way of timeStamps[].

  if ( Cache.validBits[index][0] == 0 && Cache.validBits[index][1] == 0 ) {
    
    // Since both entries are invalid, we use the first entry:
    Cache.data[index][0] = datum;
    Cache.tags[index][0] = tag;
    Cache.validBits[index][0] = 1;
    Cache.timeStamps[index] = 1;
    return 0;
  } else if ( Cache.validBits[index][0] == 0 && Cache.validBits[index][1] == 1 ) {

    // Use the only invalid entry (first one):
    Cache.data[index][0] = datum;
    Cache.tags[index][0] = tag;
    Cache.validBits[index][0] = 1;
    Cache.timeStamps[index] = 1;
    return 0;
  } else if ( Cache.validBits[index][0] == 1 && Cache.validBits[index][1] == 0 ) {

    // Use the only invalid entry (second one):
    Cache.data[index][1] = datum;
    Cache.tags[index][1] = tag;
    Cache.validBits[index][1] = 1;
    Cache.timeStamps[index] = 0;
    return 1;
  } else {
  
    // Since both entries are invalid, we must compare time stamps and replace
    //the oldest entry.
    if ( Cache.timeStamps[0] == 0 ) { 

      // First entry is the oldest, so replace it.
      Cache.data[index][0] = datum;
      Cache.tags[index][0] = tag;
      Cache.timeStamps[index] = 1;
      return 0;
    } else {

      // Second entry is the oldest, so replace it.
      Cache.data[index][1] = datum;
      Cache.tags[index][1] = tag;
      Cache.timeStamps[index] = 0;
      return 1;
    }
  }
}

int readCacheBlock(int index, int i, int offset) {

  //Here, data is extracted from an entry on the cache. Bits must be
  //masked and shifted as the bytes are stored in little-endian format.

  if ( offset == 0 ) {

    return Cache.data[index][i] & 0x000000ff;
  } else if ( offset == 1 ) {

    return ( Cache.data[index][i] & 0x0000ff00 ) >> 8;
  } else if ( offset == 2 ) {
  
    return ( Cache.data[index][i] & 0x00ff0000 ) >> 16;
  } else {
    
    //When offset == 3:
    return ( Cache.data[index][i] & 0xff000000 ) >> 24;
  }
}

void
initialize() {
/* if there is any initialization you would like to have, do it here */
/*  This is called for you in the main program */

  //Here, all elements from the arrays from the structs are set to zero.

  for ( int i = 0; i < 16; i++ ) {

    Tlb.ppnVector[i] = 0;
    Tlb.tagVector[i] = 0; 
    Tlb.validVector[i] = 0;
  }

  for ( int i = 0; i < 512; i++ ) {
 
    PageTable.ppnVector[i] = 0;
    PageTable.validVector[i] = 0;
  }

  for ( int i = 0; i < 32; i++ ) {

    for ( int j = 0; j < 2; j++ ) {

      Cache.data[i][j] = 0;
      Cache.tags[i][j] = 0;
      Cache.validBits[i][j] = 0;
    }
    Cache.timeStamps[i] = 0;
  }
}

/* You will implement the two functions below:
 *     * you may add as many additional functions as you like
 *     * you may add header files for your data structures
 *     * you MUST call the relevant log_entry() functions (described below)
 *          or you will not receive credit for all of your hard work!
 */

int
get_physical_address(int virt_address) {
/*
   Convert the incoming virtual address to a physical address. 
     * if virt_address too large, 
          log_entry(ILLEGALVIRTUAL,virt_address); 
          return -1
     * if PPN is in the TLB, 
	  compute PA 
          log_entry(ADDRESS_FROM_TLB,PA);
          return PA
     * else use the page table function to get the PPN:
          * if VPN is in the Page Table
	          compute PA 
                  add the VPN and PPN in the TLB
	          log_entry(ADDRESS_FROM_PAGETABLE,PA);
	          return PA
	  * else load the frame into the page table
	          PPN = load_frame(VPN) // use this provided library function
                  add the VPN and PPN in to the page table
                  add the VPN and PPN in to the TLB
 		  compute PA
		  log_entry(ADDRESS_FROM_PAGE_FAULT_HANDLER,PA);
 		  return PA
*/

    if ( ( virt_address & 0xfffc0000 ) != 0 ) {
  
      // In the event that virt_address is too big:
      log_entry(ILLEGALVIRTUAL, virt_address);
      return -1;
    }
    int vpn = ( virt_address & 0x0003fe00 ) >> 9;
    int vpo = ( virt_address & 0x000001ff ); 
    int PA = lookUpTlb(vpn, vpo);

    return PA;
}



char
get_byte(int phys_address) {
/*
   Use the incoming physical address to find the relevant byte. 
     * if data is in the cache, use the offset (last 2 bits of address)
          to compute the byte to be returned data
          log_entry(DATA_FROM_CACHE,byte);
          return byte 
     * else use the function get_long_word(phys_address) to get the 
          4 bytes of data where the relevant byte will be at the
          given offset (last 2 bits of address)
          log_entry(DATA_FROM_MEMORY,byte);
          return byte

NOTE: if the incoming physical address is too large, there is an
error in the way you are computing it...
*/
   char byte;
   byte = lookUpCache(phys_address);
   return byte;
}

