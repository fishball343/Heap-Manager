#include <stdio.h> //needed for size_t
#include <unistd.h> //needed for sbrk
#include <assert.h> //For asserts
#include "dmm.h"

/*******README *****************

FILES MODIFIED:
dmm.c

TEAM: Justin Wang, Parit Burinthratikul

HOURS SPENT: 10

SOURCES: Randal E. Bryant and David R. O’Hallaron. Dynamic Memory Management, Chapter 9 from Computer Systems: A Programmer’s Perspective

IMPLEMENTAION AND EXPLANATION:

The heap manager was implemented using a linked list of metadata_t structs or blocks of memory. These linked lists are doubly linked and 
contain a metadata_t* header to indicate the size of the blocks (integer), whether the block is allocated (isFree=1) or free (isFree=0) (integer), 
and the pointers to the previous and next blocks. Although we added the isFree integer into our header, it does not change the value of METADATA_T_ALIGNED 
as that is rounded to 32 or 64 depending on the machine (dmm.h). However it does  affect our test_basic results since it adds an additional 8 bytes into 
the first block of the malloc call. This means that test_basic would not run in the default MAX_HEAP_SIZE of 1024 bytes heap but would work properly 
by increasing the heap to a 1032 or more bytes due to having extra  
 
Using a doubly linked list containing all of the blocks (free and not free thus implicit free-list) was beneficial  to the coalesce, free and split 
operations primarily because it allowed for simpler implementation. For instance, the free command was done by changing the header of that block at 
the particular address so that the header denotes that the block is free. Coalescing between two blocks was done by checking if the previous block and 
the current block, or the current block and next block were both free and merged the two blocks if such was the case. This is far more easier than making 
a linked list of only free blocks (explicit free-list) since we can just check the headers of adjacent blocks and coalesce them accordingly if both are free. 

Since the project was implemented using first fit algorithm, the dmalloc call was done by iterating through the linked lists of blocks until we found 
the first free block that had more or equal bytes than the desired size of the malloc. If the free block and the desired size of malloc were equal, 
we changed the header to indicate that the block became allocated or not free. If the free block size was bigger, a split operation is needed. 
Since we used first fit algorithm, we changed the header of the free block to become allocated. We then created a new header for the remaining 
space of the block which is casted to be free. The allocated block then points next to this free block of remaining space which now points to the old
 next call of the now allocated block. 

One major downside of the doubly linked list is that you have to iterate more than the number of free blocks since we obviously included the allocated 
blocks into the linked list. Despite the longer runtime in the malloc call, the free and coalesce remains O(1) since the algorithm only deals with 
that desired block to be freed.  If we created a linked list of only free blocks, then we can shorten the run time of malloc since it would iterate 
through less blocks however coalesce would become a O(n) algorithm as we would have to iterate through the free block list to determine our coalesced blocks.


FEEDBACK: 

We enjoyed working on the assignment since it was refreshing to relearn C. It was challenging but interesting in that there were two different approaches (implicit and explicit) to the assignment in which each path offer tradeoffs and benefits. I felt that our code was well implemented since it passed all tests and had 81% success rate in test_stress2. Debugging was obviously a pain since we did not have much experience with GDB. 

RESULTS:

-test_basic displayed desired results

-test_coalesce displayed the desired results 

-test_stress1 shows that the malloc at ptr[3] failed correctly for size 6291456. This is correct since the call far exceed the 1MB MAX_HEAP_SIZE limit.

-test_stress2 using a MAX_HEAP_SIZE of 1MB displayed the following results

Loop count: 50000, malloc successful: 41377, malloc failed: 8623, execution time: 0.12 seconds

A success rate of 82.6% shows that our implementation of first fit algorithm is very efficient. 

-test_stress3 displayed the same efficiency as test_stress2 since it is essentially the same test with the results of each block printed out. The remaining text of the result is very large so I would not included it here. 

Based on these tests, I believe that dmm.c is coded correctly. 
 */

 typedef struct metadata {
       /* size_t is the return type of the sizeof operator. Since the size of
 	* an object depends on the architecture and its implementation, size_t 
	* is used to represent the maximum size of any object in the particular
 	* implementation.
	* size contains the size of the data object or the amount of free
 	* bytes 
	*/
 	size_t size;
 	int isFree;
 	struct metadata* next;
	struct metadata* prev; //What's the use of prev pointer?
} metadata_t;

/* freelist maintains all the blocks which are not in use; freelist is kept
 * always sorted to improve the efficiency of coalescing 
 */

 static metadata_t* freelist = NULL;
 void* dmalloc(size_t numbytes) {
	if(freelist == NULL) { 			//Initialize through sbrk call first time
		if(!dmalloc_init())
			return NULL;
	}
	
	assert(numbytes > 0);
	numbytes=ALIGN(numbytes);
	metadata_t* header1 = freelist;
	int totalmem = numbytes+ METADATA_T_ALIGNED;
	while(numbytes > header1->size  || header1->isFree == 1 ) {
		
		header1 = header1->next;
/* Reached the end of the heap */
		if(header1 == NULL) {
			
			return NULL;
		}
	}

	void* ptr=NULL;
	
	if(header1->size <= totalmem){
		header1->isFree = 1;
	}
	else { 
		ptr = (void*) header1;
		ptr = ptr + totalmem;
		metadata_t* leftover = (metadata_t *)ptr;
		leftover->size = header1->size-totalmem;
		leftover->prev = header1;
		leftover->next = header1->next;

				
	
		if (header1->next!=NULL){
				leftover->next->prev= leftover;
		}
		leftover->isFree = 0;
		if(leftover->next != NULL && leftover->next->isFree == 0 ) {
		
		leftover->size = leftover->size + leftover->next->size + METADATA_T_ALIGNED;
		if(leftover->next->next != NULL){
		leftover->next->next->prev =leftover;
	}
		leftover->next = leftover->next->next;
		
	}
		header1->size = numbytes;
		header1->next = leftover;
		header1->isFree = 1;
		
		
	}
	ptr = (void*) header1;
	ptr += METADATA_T_ALIGNED;
	
	return ptr;
}

void dfree(void* ptr) {

	ptr -= METADATA_T_ALIGNED;
	metadata_t* header1 = (metadata_t*) ptr;
	/* marks the block as free */
	header1->isFree = 0;
	/* coalesce */

	if(header1->next != NULL && header1->next->isFree == 0 ) {
		header1->size = header1->size + header1->next->size + METADATA_T_ALIGNED;
		if(header1->next->next != NULL){
		header1->next->next->prev =header1;
	}
		header1->next = header1->next->next;
		
	}

	if(header1->prev != NULL && header1->prev->isFree == 0 ) {
		header1->prev->size = header1->prev->size + header1->size + METADATA_T_ALIGNED;
		if(header1->next != NULL){
		header1->next->prev = header1->prev;
	}
		header1->prev->next = header1->next;
	}



}







bool dmalloc_init() {

	/* Two choices: 
 	* 1. Append prologue and epilogue blocks to the start and the end of the freelist
 	* 2. Initialize freelist pointers to NULL
 	* Note: We provide the code for 2. Using 1 will help you to tackle the
 	* corner cases succinctly.
 	*/

 	size_t max_bytes = ALIGN(MAX_HEAP_SIZE);
	freelist = (metadata_t*) sbrk(max_bytes); // returns heap_region, which is initialized to freelist
	/* Q: Why casting is used? i.e., why (void*)-1? */
	if (freelist == (void *)-1)
		return false;
	freelist->next = NULL;
	freelist->prev = NULL;
	freelist->size = max_bytes-METADATA_T_ALIGNED;
	freelist->isFree = 0;
	return true;
}

/*Only for debugging purposes; can be turned off through -NDEBUG flag*/
void print_freelist() {
	metadata_t *freelist_head = freelist;
	while(freelist_head != NULL) {
		DEBUG("\tFreelist Size:%zd, Head:%p, Prev:%p, Next:%p,Free:%d\t",freelist_head->size,freelist_head,freelist_head->prev,freelist_head->next,freelist_head->isFree);
		freelist_head = freelist_head->next;
	}
	DEBUG("\n");
}
