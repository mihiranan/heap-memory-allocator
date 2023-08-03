#include <stdio.h>  // for printf
#include <string.h>  // for memmove
#include "./allocator.h"
#include "./debug_break.h"

#define LEAST_3_SIGBITS ~0x7
#define ALIGNMENT 8
#define MAX_REQUEST_SIZE (1 << 30)
#define MIN_REQUEST_SIZE 24  // the minimum number of bytes for an "empty" heap

// link struct that will be used to build the linked list of free heap blocks
typedef struct link {
    struct link *next;
    struct link *previous;
} link;

static void *segment_start;  // variable that keeps track of the start of the heap (from myinit)
static size_t segment_size;  // variable that stores the size of the heap (from myinit)
static char *segment_end;  // variable that stores the end of the heap (from myinit)
typedef size_t header;  // typedef header for easier readability and less confusion
static header* start_hdr;  // header of the start of the heap (from myinit)
link *linked_start;  // linked list that points will continually be updated as the list is built
static int blocks_allocated;  // keeps track of the number of allocated blocks in the heap (for validate_heap_


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
    if (heap_size >= ALIGNMENT) {  // makes sure that the heap_size is at least 8 bytes
        blocks_allocated = 0;
        segment_start = heap_start;
        segment_size = heap_size;
        segment_end = (char *) segment_start + segment_size;
        start_hdr = segment_start;
        *start_hdr = heap_size - ALIGNMENT;
        linked_start = (link *) ((char*) start_hdr + ALIGNMENT);
        return true;
    }
    return false;  // if heap_size is less than 8 bytes
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
 * If the number is less than the minimum size though,
 * this function will return the minimum size.
 */
size_t roundup(size_t sz, size_t mult) {
    if (MIN_REQUEST_SIZE > sz) {
        return MIN_REQUEST_SIZE;
    }
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

/* HELPER FUNCTION : linkFree
 * ----------------------------
 * Given a heap block that should be free, add it
 * to the linked list using the "last-in first-out"
 * explicit free list design logic.
 */
void linkFree(link *block) {
    // if linked list has some elements
    if (linked_start != NULL) {
        block->next = linked_start;
        block->previous = NULL;
        linked_start->previous = block;
        linked_start = block;
    } else {  // if linked list is empty
        block->next = NULL;
        block->previous = NULL;
        linked_start = block;
    }
}

/* HELPER FUNCTION : unlinkFree
 * ----------------------------
 * Given a heap block that we want to set as allocated,
 * rewire the linked list to fit it in using the 
 * "last-in first-out explicit free list design logic".
 */
void unlinkFree(link *block) {
    link *before_block = block->previous;
    link *after_block = block->next;
    // if before and after blocks exist 
    if (before_block != NULL && after_block != NULL) {
        before_block->next = after_block;
        after_block->previous = before_block;
     // if there is nothing before but there is a block afte   
    } else if (before_block == NULL && after_block != NULL) {
        after_block->previous = NULL;
        linked_start = after_block;
     // if there is nothing after but there is a block before
    } else if (before_block != NULL && after_block == NULL) {
        before_block->next = NULL;
    } else {  // if before and after blocks do not exist
        linked_start = NULL;
    }         
}

/* HELPER FUNCTION : coalesce
 * ----------------------------
 * Given a payload, check if there is a free
 * block next to it. If so, unify them into 
 * a bigger free block.
 */
bool coalesce(void *payload) {
    header *curr = accessHeader(payload);
    header *neighbor = nextBlock(curr);
    if (!isAllocated(neighbor)) {
        size_t neighbor_size = getSize(neighbor);
        *curr += neighbor_size + ALIGNMENT;
        unlinkFree((link *) accessPayload(neighbor));
        return true;
    }
    return false;
}

/* HELPER FUNCTION : splitting
 * ----------------------------
 * If the free block found is greater than the number of 
 * bytes we are trying to allocate on the heap, we must do
 * splitting to set the unallocated part of the free block as 
 * free. Otherwise, it is a wastage of space on the heap.
 */
link *splitting(header *hdr, size_t actual_size, size_t og_size, link *list) {
    *hdr = actual_size;
    header *split = nextBlock(hdr);
    blocks_allocated++;
    *split = og_size - actual_size - ALIGNMENT;
    link *neighbor = (link *) accessPayload(split);
    linkFree(neighbor);
    unlinkFree(list);
    statusAllocated(hdr);
    return list;
}

/* MAIN FUNCTION : mymalloc
 * -------------------------
 * Given a user-inputted requested size (the amount the user 
 * wants allocated on the heap), find the best free block from the
 * linked list that is greater than or equal to the rounded up version
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
    size_t actual_size = roundup(requested_size, ALIGNMENT);
    link *list = linked_start;
    while (list != NULL) {
        header *hdr = accessHeader(list);
        size_t og_size = getSize(hdr);
        if (og_size < actual_size) {
            list = list->next;  // finding the right free bloock in the linked list
            continue;
        }
        if (og_size >= actual_size + MIN_REQUEST_SIZE) {
            return splitting(hdr, actual_size, og_size, list);  // goes to splitting helper function
        } else {  // if the free block found fits the actual_size perfectly
            unlinkFree(list);
            statusAllocated(hdr);
            blocks_allocated++;
            return list;
        }
    }
    return NULL;
}

/* MAIN FUNCTION: myfree
 * ----------------------
 * Given a pointer to a heap block's payload, change
 * the status bit of the corresponding header to free 
 * (turn off least significant bit).
 * Includes coalescing!
 */
void myfree(void *ptr) {
    if (ptr != NULL) {  // makes sure that an invalid pointer is not given
        header *hdr = accessHeader(ptr);
        link *freed = (link *) ptr;
        linkFree(freed);
        coalesce(ptr);  // goes to coalesce helper function
        statusFree(hdr);
        blocks_allocated--;
    }
}

/* MAIN FUNCTION - myrealloc
 * --------------------------
 * Given an old pointer to a heap block payload and the new size
 * that the block is expanding to, returns a pointer to the payload
 * of a heap block that is new_size bytes large.
 */
void *myrealloc(void *old_ptr, size_t new_size) {
    new_size = roundup(new_size, ALIGNMENT);
    // if no old_ptr specified, just do regular mymalloc
    if (old_ptr == NULL) {
        return mymalloc(new_size);
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

/* HELPER FUNCTION: linkedListWrong
 * ---------------------------------
 * Given a block in the linked list that should be free,
 * check if the list is wired incorrectly in terms of the 
 * order or if an allocated block is included.
 */
bool linkedListWrong(link *curr) {
    header *curr_hdr = accessHeader(curr);
    // if the order of the list does not make sense
    if ((curr->previous != NULL && curr->previous->next != curr) ||
        (curr->next != NULL && curr->next->previous != curr)) {  
        return true;
    }
        // if something in the list is not free
    if (isAllocated(curr_hdr)) {  
        return true;
    }
    return false;
}

/* HELPER FUNCTION : validate_heap
 * --------------------------------
 * Goes through the entire linked list and calls the
 * linkedListWrong helper function to make sure it is 
 * not wired incorrectly.
 * In adddition, it goes through the entire heap and 
 * makes sure that the number of allocated blocks matches
 * the block_allocated variable that was being continually
 * updated as myfree and mymalloc were being called.
 */
bool validate_heap() {
    bool result =  true;

    // checks whether the number of allocated blocks checks out
    header *ptr = segment_start;
    int check_allocated = 0;
    while ((char *) ptr != segment_end) {
        if (isAllocated(ptr)) {
            check_allocated++;
        }
        ptr = nextBlock(ptr);
    }
    // Should be equal if heap blocks were allocated properly
    if (check_allocated != blocks_allocated) {
        printf("ERROR! nused and check_nused do not match up.");
        breakpoint();
        result = false;
    }

    // checks if the linked list was built correctly
    link *curr = linked_start;
    if (curr == NULL) {
        result = false;
    }
    while (curr != NULL) {
        if (linkedListWrong(curr)) {
            result = false;
        }
        curr = curr->next;
    }
    return result;
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
