/*
 * CS252: MyMalloc Project
 *
 * The current implementation gets memory from the OS
 * every time memory is requested and never frees memory.
 *
 * You will implement the allocator as indicated in the handout,
 * as well as the deallocator.
 *
 * You will also need to add the necessary locking mechanisms to
 * support multi-threaded programs.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include "MyMalloc.h"

static pthread_mutex_t mutex;

const int arenaSize = 2097152;

void increaseMallocCalls()  { _mallocCalls++; }

void increaseReallocCalls() { _reallocCalls++; }

void increaseCallocCalls()  { _callocCalls++; }

void increaseFreeCalls()    { _freeCalls++; }

extern void atExitHandlerInC()
{
    atExitHandler();
}

/* 
 * Initial setup of allocator. First chunk is retrieved from the OS,
 * and the fence posts and freeList are initialized.
 */
void initialize()
{
    // Environment var VERBOSE prints stats at end and turns on debugging
    // Default is on
    _verbose = 1;
    const char *envverbose = getenv("MALLOCVERBOSE");
    if (envverbose && !strcmp(envverbose, "NO")) {
        _verbose = 0;
    }

    pthread_mutex_init(&mutex, NULL);
    void *_mem = getMemoryFromOS(arenaSize);

    // In verbose mode register also printing statistics at exit
    atexit(atExitHandlerInC);

    // establish fence posts
    ObjectHeader * fencePostHead = (ObjectHeader *)_mem;
    fencePostHead->_allocated = 1;
    fencePostHead->_objectSize = 0;

    char *temp = (char *)_mem + arenaSize - sizeof(ObjectHeader);
    ObjectHeader * fencePostFoot = (ObjectHeader *)temp;
    fencePostFoot->_allocated = 1;
    fencePostFoot->_objectSize = 0;

    // Set up the sentinel as the start of the freeList
    _freeList = &_freeListSentinel;

    // Initialize the list to point to the _mem
    temp = (char *)_mem + sizeof(ObjectHeader);
    ObjectHeader *currentHeader = (ObjectHeader *)temp;
    currentHeader->_objectSize = arenaSize - (2*sizeof(ObjectHeader)); // ~2MB
    currentHeader->_leftObjectSize = 0;
    currentHeader->_allocated = 0;
    currentHeader->_listNext = _freeList;
    currentHeader->_listPrev = _freeList;
    _freeList->_listNext = currentHeader;
    _freeList->_listPrev = currentHeader;

    // Set the start of the allocated memory
    _memStart = (char *)currentHeader;

    _initialized = 1;
}

/* 
 * TODO: In allocateObject you will handle retrieving new memory for the malloc
 * request. The current implementation simply pulls from the OS for every
 * request.
 *
 * @param: amount of memory requested
 * @return: pointer to start of useable memory
 */
void * allocateObject(size_t size)
{
    // Make sure that allocator is initialized
    if (!_initialized)
        initialize();

    /* Add the ObjectHeader to the size and round the total size up to a 
     * multiple of 8 bytes for alignment.
     */
    size_t real_size = (size + sizeof(ObjectHeader) + 7) & ~7;
	//^^ steps 1 and 2 already complete ^^

	//Traverse free list from beginning and find 1st block big enough
	ObjectHeader *o = _freeList->_listNext;
	void *_mem = NULL;	//the pointer to be returned
	int added2MB = 0;
	while (o != &_freeListSentinel || !added2MB) {
		if (o == &_freeListSentinel && !added2MB) {
			_mem = getMemoryFromOS(arenaSize);
			//fence posts
			ObjectHeader * fencePostHead = (ObjectHeader *)_mem;
			fencePostHead->_allocated = 1;
			fencePostHead->_objectSize = 0;
			char *temp= (char *)_mem + arenaSize - sizeof(ObjectHeader);
			ObjectHeader * fencePostFoot = (ObjectHeader *)temp;
			fencePostFoot->_allocated = 1;
			fencePostFoot->_objectSize = 0;
			//add to beginning of free list
			temp = (char *)_mem + sizeof(ObjectHeader);
			ObjectHeader *currentHeader = (ObjectHeader *)temp;
			currentHeader->_objectSize = arenaSize - (2*sizeof(ObjectHeader));
			currentHeader->_leftObjectSize = 0;
			currentHeader->_allocated = 0;
			currentHeader->_listNext = o->_listNext;
			currentHeader->_listPrev = o;
			o->_listNext->_listPrev = currentHeader;
			o->_listNext = currentHeader;

			o = o->_listNext;
			added2MB = 1;
		}
		//split and remove
		if (o->_objectSize >= real_size + sizeof(ObjectHeader) + 8)
		{
			//update free list
			size_t newSize = o->_objectSize - real_size;
			o->_objectSize = newSize;
			//update the newly allocated mem
			o = (ObjectHeader *)((char *)o + newSize);
			o->_objectSize = real_size;
			o->_leftObjectSize = newSize;
			o->_allocated = 1;
			o->_listNext = NULL;
			o->_listPrev = NULL;
			
			//update the leftObjectSize of the following block in mem
			ObjectHeader *right = (ObjectHeader *)((char *)o + o->_objectSize);
			if (!(right->_allocated == 1 && right->_objectSize == 0))//if not a fence post
				right->_leftObjectSize = o->_objectSize;

			//get the pointer to useable mem
			_mem = (void *)((char *)o + sizeof(ObjectHeader));
			break;
		}
		//just remove
		else if (o->_objectSize >= real_size)
		{
			//update free list
			o->_listPrev->_listNext = o->_listNext;
			o->_listNext->_listPrev = o->_listPrev;
			//update the newly allocated mem
			o->_allocated = 1;
			o->_listNext = NULL;
			o->_listPrev = NULL;
			//get the pointer to usable mem
			_mem = (void *)((char *)o + sizeof(ObjectHeader));
			break;
		}
		o = o->_listNext;
	}

	pthread_mutex_unlock(&mutex);

	// Return a pointer to useable memory
	return _mem;

/*
    // Naively get memory from the OS every time
    void *_mem = getMemoryFromOS(arenaSize); 

    // Store the size in the header
    ObjectHeader *o = (ObjectHeader *)_mem;
    o->_objectSize = roundedSize;

    pthread_mutex_unlock(&mutex);

    // Return a pointer to useable memory
    return (void *)((char *)o + sizeof(ObjectHeader));
*/
}

/* 
 * TODO: In freeObject you will implement returning memory back to the free
 * list, and coalescing the block with surrounding free blocks if possible.
 *
 * @param: pointer to the beginning of the block to be returned
 * Note: ptr points to beginning of useable memory, not the block's header
 */
void freeObject(void *ptr)
{
    // Add your implementation here
    //get pointer to header and pointers left and right of header
    ObjectHeader *o = (ObjectHeader *)((char *)ptr - sizeof(ObjectHeader));
    ObjectHeader *left = (ObjectHeader *)((char *)o - o->_leftObjectSize);
    ObjectHeader *right = (ObjectHeader *)((char *)o + o->_objectSize);
    if (!(left->_allocated) && !(right->_allocated)) {
	left->_objectSize = left->_objectSize + o->_objectSize + right->_objectSize;
	//corner case
	if (right->_listNext != left) {
		left->_listNext = right->_listNext;
		right->_listNext->_listPrev = left;
	}
	else {
		left->_listNext = &_freeListSentinel;
		left->_listPrev = &_freeListSentinel;
		_freeList->_listNext = left;
		_freeList->_listPrev = left;
	}
	o->_allocated = 0;
	//right->_listNext->_listPrev = left;
	right->_listNext = NULL;
	right->_listPrev = NULL;
    }
    else if (!(left->_allocated)) {
	left->_objectSize += o->_objectSize;
	o->_allocated = 0;
	right->_leftObjectSize = left->_objectSize;
    }
    else if (!(right->_allocated)) {
	o->_objectSize += right->_objectSize;
	o->_allocated = 0;
	o->_listNext = right->_listNext;
	o->_listPrev = right->_listPrev;
	right->_listNext->_listPrev = o;
	right->_listPrev->_listNext = o;
	right->_listNext = NULL;
	right->_listPrev = NULL;
	ObjectHeader *rightMem = (ObjectHeader *)((char *)o + o->_objectSize);
	if (!(rightMem->_allocated == 1 && rightMem->_objectSize == 0))//if not a fence post
		rightMem->_leftObjectSize = o->_objectSize;
    }
    else {
	o->_allocated = 0;
	o->_listNext = _freeList->_listNext;
	o->_listPrev = _freeList;
	_freeList->_listNext->_listPrev = o;
	_freeList->_listNext = o;
    }
    return;
}

/* 
 * Prints the current state of the heap.
 */
void print()
{
    printf("\n-------------------\n");

    printf("HeapSize:\t%zd bytes\n", _heapSize );
    printf("# mallocs:\t%d\n", _mallocCalls );
    printf("# reallocs:\t%d\n", _reallocCalls );
    printf("# callocs:\t%d\n", _callocCalls );
    printf("# frees:\t%d\n", _freeCalls );

    printf("\n-------------------\n");
}

/* 
 * Prints the current state of the freeList
 */
void print_list() {
    printf("FreeList: ");
    if (!_initialized) 
        initialize();

    ObjectHeader * ptr = _freeList->_listNext;

    while (ptr != _freeList) {
        long offset = (long)ptr - (long)_memStart;
        printf("[offset:%ld,size:%zd]", offset, ptr->_objectSize);
        ptr = ptr->_listNext;
        if (ptr != NULL)
            printf("->");
    }
    printf("\n");
}

/* 
 * This function employs the actual system call, sbrk, that retrieves memory
 * from the OS.
 *
 * @param: the chunk size that is requested from the OS
 * @return: pointer to the beginning of the chunk retrieved from the OS
 */
void * getMemoryFromOS(size_t size)
{
    _heapSize += size;

    // Use sbrk() to get memory from OS
    void *_mem = sbrk(size);

    // if the list hasn't been initialized, initialize memStart to mem
    if (!_initialized)
        _memStart = _mem;

    return _mem;
}

void atExitHandler()
{
    // Print statistics when exit
    if (_verbose)
        print();
}

/*
 * C interface
 */

extern void * malloc(size_t size)
{
    pthread_mutex_lock(&mutex);
    increaseMallocCalls();

    return allocateObject(size);
}

extern void free(void *ptr)
{
    pthread_mutex_lock(&mutex);
    increaseFreeCalls();

    if (ptr == 0) {
        // No object to free
        pthread_mutex_unlock(&mutex);
        return;
    }

    freeObject(ptr);
}

extern void * realloc(void *ptr, size_t size)
{
    pthread_mutex_lock(&mutex);
    increaseReallocCalls();

    // Allocate new object
    void *newptr = allocateObject(size);

    // Copy old object only if ptr != 0
    if (ptr != 0) {

        // copy only the minimum number of bytes
        ObjectHeader* hdr = (ObjectHeader *)((char *) ptr - sizeof(ObjectHeader));
        size_t sizeToCopy =  hdr->_objectSize;
        if (sizeToCopy > size)
            sizeToCopy = size;

        memcpy(newptr, ptr, sizeToCopy);

        //Free old object
        freeObject(ptr);
    }

    return newptr;
}

extern void * calloc(size_t nelem, size_t elsize)
{
    pthread_mutex_lock(&mutex);
    increaseCallocCalls();

    // calloc allocates and initializes
    size_t size = nelem *elsize;

    void *ptr = allocateObject(size);

    if (ptr) {
        // No error; initialize chunk with 0s
        memset(ptr, 0, size);
    }

    return ptr;
}

