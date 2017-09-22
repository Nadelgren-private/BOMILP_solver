#ifndef ESA_THREADPOOL_H
#define ESA_THREADPOOL_H
#include <stdbool.h>
#include "esa_threads.h"

/**
 * Function pointer typedef. A simple function with only one void* input,
 * much like the prototypical (in the context of esa_thread utilities) 
 * "pthread_create" function expects, which also returns void*. Note
 * that this does us no good here, since a worker thread does not know
 * what to do with return values.
 */
typedef void *(*esa_funcp) (void *input);

/**
 * A generic command structure.
 */
typedef struct {
	esa_funcp func;
	void *input;
#ifdef DEBUG
	char name[32];
#endif
} esa_command;

/**
 * Threadpool definition. Actual implementation is hidden.
 */
typedef struct esa_threadpool_ esa_threadpool;

/**
 * Initialize a threadpool with a given number of workers. User
 * is responsible for calling ***_free when finished.
 * @param n_workers the number of worker threads desired
 */
esa_threadpool *  esa_threadpool_init(int n_workers);

/**
 * Free a previously allocated threadpool. Note that this will halt
 * program execution until all current worker threads have finished.
 * @param threadpool a previously allocated esa_threadpool
 */
void              esa_threadpool_free(esa_threadpool *threadpool);

/**
 * Halt current thread/program execution until all worker
 * threads have returned.
 * @param threadpool an esa_threadpool instance
 */
void              esa_threadpool_join(esa_threadpool *threadpool);

/**
 * Add a command to threadpool. It will be handled by the first
 * available worker thread.
 * @param threadpool an esa_threadpool instance
 * @param command the command to be executed
 * @return 0 on success, 1 on failure (threadpool is NULL or queue is full)
 */
int               esa_threadpool_push_command(esa_threadpool *threadpool, esa_command command);

/**
 * Check if the threadpool is finished, i.e. all worker threads are 
 * sleeping and the command queue is empty.
 * @param threadpool an esa_threadpool instance
 */
bool              esa_threadpool_is_finished(esa_threadpool *threadpool);

#endif