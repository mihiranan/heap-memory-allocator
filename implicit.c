#include <stdio.h>  // for printf
#include <string.h>  // for memmove
#include "./allocator.h"
#include "./debug_break.h"

#define ALIGNMENT 8
#define MAX_REQUEST_SIZE (1 << 30)
#define LEAST_3_SIGBITS ~0x7

static void *segment_start;
static size_t segment_size;
static char *segment_end;
static size_t nused;
typedef size_t header;
static header* start_hdr;


/* MAIN FUNCTION : myinit
 * -----------------------
 * Given a non-NULL heap_start pointer and a 
 * heap_size value, initializes the heap by 
 * giving the global variables values. 
 * Returns true if heap was properly initialized
 * and returns false if parameters were not valid
 * (heap not able to be initialized).
 */
bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < ALIGNMENT) {  // heap_size must be at least 8 bytes
        return false;
    }
    nused = 0;  // resets nused for every script call
    nused += ALIGNMENT;
    segment_start = heap_start;
    segment_size = heap_size;
    segment_end = (char *) segment_start + segment_size;
    start_hdr = segment_start;
    *start_hdr = heap_size - ALIGNMENT;
    return true;
}

/* HELPER FUNCTION : statusAllocated
 * ----------------------------------
 * Turns on least significant bit in header
 * to indicate that the corresponding heap 
 * block is used. */
void statusAllocated(header* hdr) {
    *hdr |= 1;
}

/* HELPER FUNCTION : statusFree
 * ------------------------------
 * Turns off least significant bit in header
 * to indicate that the corresponding heap 
 * block is free */
void statusFree(header* hdr) {
    *hdr &= ~1;
}

/* HELPER FUNCTION : getSize
 * --------------------------  
 * Returns the size of the payload by turning
 * off the least three significant bits (which 
 * hold the free/used status)
 */
size_t getSize(header* hdr) {
    return (*hdr & LEAST_3_SIGBITS);
}

/* HELPER FUNCTION : accessPayload
 * --------------------------------
 * Given a header pointer, return the associated
 * payload pointer (which is eight bytes past
 * the header since the header size is eight).
 */
void* accessPayload(header* hdr) {
    return (char*) hdr + ALIGNMENT;
}

/* HELPER FUNCTION : accessHeader
 * -------------------------------
 * Given a payload pointer, return the associated
 * header pointer (which is eight bytes before the
 * payload since the the header size is eight).
 */ 
header* accessHeader(void* payload) {
    return (header*) ((char*) payload - ALIGNMENT);
}

/* HELPER FUNCTION : isAllocated
 * ------------------------------
 * Given a header pointer, return true if the 
 * the heap block is allocated/user and return 
 * false if the heap block is free.
 */
bool isAllocated(header* hdr) {
    return ((*hdr & 1) == 1);
}

/* HELPER FUNCTION : roundup
 * --------------------------
 * Given a number and a multiple (must be a power of 2),
 * returns the rounded up version of the number.
 * Used to make sure requested_size in mymalloc is 
 * rounded up the the next biggest multiple of eight.
 */
size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

/* HELPER FUNCTION : nextBlock
 * ----------------------------
 * Given a header pointer, returns a new header 
 * pointer to the next heap block by doing pointer 
 * arithmetic and leveraging previous helper functions.
 */
header* nextBlock(header* hdr) {
    size_t block_size = getSize(hdr);
    void* payload = accessPayload(hdr);
    header* nxt = (header*)((char*) payload + block_size);
    return nxt;
}

/* MAIN FUNCTION : mymalloc
 * -------------------------
 * Given a user-inputted requested size (the amount the user 
 * wants allocated on the heap), find the first heap block
 * that would be greater than or equal to the rounded up version
 * of requested_size (next biggest multiple of 8).
 *
 * If the heap block found is greater than request_size bytes, 
 * splitting is implemented.
 *
 * Returns pointer to the payload of the allocated heap block if
 * successful or return NULL if there is no space on the heap for 
 * the requested size.
 */
void *mymalloc(size_t requested_size) {
    // rounds up to next biggest multiple of 8 from requested_size
    size_t actual_size = roundup(requested_size, ALIGNMENT);  
    header *ptr = start_hdr;
    
    // adjusts ptr to point to the heap block that is free and
    // is greater than or equal to actual_size
    while (isAllocated(ptr) || actual_size > *ptr) {
        ptr = nextBlock(ptr);
    }
    if (getSize(ptr) == actual_size) {  // if heap block size is the same as actual_size
        statusAllocated(ptr);
        void *load = accessPayload(ptr);
        nused += actual_size;
        return load;
    }
    // if heap block is more bytes than actual_size (REQUIRES SPLITTING)
    if (getSize(ptr) >= (actual_size + ALIGNMENT)) {
        size_t og_size = getSize(ptr);
        *ptr = actual_size;
        statusAllocated(ptr);
        header *split = nextBlock(ptr);
        *split = og_size - actual_size - ALIGNMENT;
        statusFree(split);
        void *load = accessPayload(ptr);
        nused += actual_size + ALIGNMENT;
        return load;
    }
    return NULL;  
}

/* MAIN FUNCTION: myfree
 * ----------------------
 * Given a pointer to a heap block's payload, change
 * the status bit of the corresponding header to free 
 * (turn off least significant bit)
 */
void myfree(void *ptr) {
    if (ptr == NULL) {  // if invalid pointer is given
        return;
    }
    header *hdr = accessHeader(ptr);
    nused -= getSize(hdr);
    statusFree(hdr);
}

/* MAIN FUNCTION - myrealloc
 * --------------------------
 * Given an old pointer to a heap block payload and the new size
 * that the block is expanding to, returns a pointer to the payload
 * of a heap block that is new_size bytes large.
 */
void *myrealloc(void *old_ptr, size_t new_size) {
    // if no old_ptr specified, just do regular mymalloc
    if (old_ptr == NULL) {
        void *result = mymalloc(new_size);
        return result;
    }
    size_t old_size = getSize(accessHeader(old_ptr));
    // if specified new_size is smaller than the old_size, do not change anything.
    // myrealloc only expands
    if (new_size <= old_size) {
        return old_ptr;
    }
    // mymalloc a bigger heap block
    void *result = mymalloc(new_size);
    memcpy(result, old_ptr, old_size);  // copies memory from old block to new block
    myfree(old_ptr);
    return result;
}

/* HELPER FUNCTION : validate_heap
 * ----------------------
 * Goes through the entire heap and counts the number
 * of bytes used and then compares that to nused which 
 * has been doing the same thing but as the operations 
 * were being done.
 */
bool validate_heap() {  
    size_t check_nused = 0;
    header *ptr = segment_start;

    if (nused > segment_size) {
        printf("ERROR! More heap bytes used than are in segment_size.");
        breakpoint();
        return false;
    }
    // Going through the heap and adding to check_nused
    while ((char *) ptr != segment_end) {
        if (!isAllocated(ptr)) {
            check_nused += ALIGNMENT;
            ptr = nextBlock(ptr);
        } else {
            check_nused += getSize(ptr) + ALIGNMENT;
            ptr = nextBlock(ptr);
        }
    }
    // Should be equal if heap was allocated successfully
    if (check_nused != nused) {
        printf("ERROR! nused and check_nused do not match up.");
        breakpoint();
        return false;
    }
    return true;
}

/* HELPER FUNCTION : dump_heap
 * ----------------------------
 * Prints out the the block contents of the heap. 
 * Called from gdb when tracing through programs.  
 * It prints out the total range of the heap, and
 * information about each block within it.
 */
void dump_heap() {
    void *ptr = segment_start;
    // Goes through the entire heap and prints out the size of each block and its status
    while (ptr != segment_end)  {
        if (!isAllocated(ptr)) {
            printf("Block Size: %lu, Free\n", getSize(ptr));
            ptr = nextBlock(ptr);
        } else {
            printf("Block Size: %lu, Allocated\n", getSize(ptr));
            ptr = nextBlock(ptr);
        }
    }
}
