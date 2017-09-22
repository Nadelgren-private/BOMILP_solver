#include <stdint.h>
#include <math.h>
#include "esa_threadpool.h"
#include <stdlib.h>

#ifdef DEBUG
#include <stdio.h>
#endif

#define ESA_DEFAULT_COMMAND_QUEUE_SIZE 1024

/**
 * Command queue structure. Stores up to size-1 commands.
 * Note that this is a circular queue.
 */
typedef struct {
	esa_command *commands;
	int size;
	int head;
	int tail;
} esa_command_queue;

/**
 * Threadpool worker struct. Each threadpool maintains a reference
 * to its owning threadpool so that it can retrieve commands.
 */
typedef struct {
	esa_threadpool *threadpool;
	esa_thread thread;
	
	int exit;
	int id;
} esa_threadpool_worker;

/**
 * A threadpool structure containing workers, a command queue, and
 * some synchronization objects.
 */
struct esa_threadpool_ {
	esa_threadpool_worker **workers;
 	esa_command_queue command_queue;
 	esa_mutex mutex;
	esa_cond cond;
	int n_workers;
	// int                       worker_idx;
};

void *esa_threadpool_worker_thread(void *input);
//----------------------------------------------------------
// COMMAND QUEUE //

// static inline int esa_command_queue_capacity(esa_command_queue *q)
// {
// 	return (q->head < q->tail ? q->tail - q->head : q->size - q->head + q->tail) - 1;
// }

// static inline int esa_command_queue_occupied(esa_command_queue *q)
// {
// 	return (q->head < q->tail ? q->size - q->tail + q->head : q->head - q->tail);
// }

/**
 * Determine the remaining capacity in a command queue.
 */
#define ESA_COMMAND_QUEUE_CAPACITY(q) \
	((q.head < q.tail ? q.tail - q.head : q.size - q.head + q.tail) - 1)

/**
 * Determine the number of occupants in a command queue.
 */
#define ESA_COMMAND_QUEUE_OCCUPIED(q) \
	((q.head < q.tail ? q.size - q.tail + q.head : q.head - q.tail))

/**
 * Push a command into a queue.
 */
static int q_push(esa_command_queue *q, esa_command command)
{
	// printf("pushing (h,t: %i, %i), capacity: %i\n", q->head, q->tail, esa_command_queue_capacity(q));
	if (ESA_COMMAND_QUEUE_CAPACITY((*q))) {
		q->commands[q->head++] = command;
		q->head = q->head % q->size;
		return 0;
	}

	return 1;
}

/**
 * Pop and return a command from a queue.
 */
static esa_command *q_pop(esa_command_queue *q)
{
	// printf("\npopping (h,t: %i, %i)\n", q->head, q->tail);
	if (q->head == q->tail) return NULL;

	esa_command *command = &q->commands[q->tail++];
	q->tail = q->tail % q->size;

	return command;
}

/**
 * Initialize the members of a command queue pointed to by "q".
 */
static int esa_command_queue_initp(esa_command_queue *q)
{
	q->commands = calloc(ESA_DEFAULT_COMMAND_QUEUE_SIZE, sizeof(esa_command));
	if (!q->commands) return 1;

	q->head = q->tail = 0;
	q->size = ESA_DEFAULT_COMMAND_QUEUE_SIZE;
	return 0;
}
//-----------------------------------------------------------
// THREADPOOL WORKER

void esa_threadpool_worker_free(esa_threadpool_worker *worker, esa_threadpool *threadpool)
{
	if (!worker || !threadpool) return;
	// printf("wreeing worker %i\n", worker->id);

	worker->exit = 1;
	// printf("waking worker %i\n", worker->id);
	esa_cond_broadcast(&threadpool->cond);
	esa_thread_join(worker->thread, NULL);

	free(worker);
}

esa_threadpool_worker *esa_threadpool_worker_init(esa_threadpool *threadpool, int id)
{
	esa_threadpool_worker *worker;
	if ((worker = malloc(sizeof(esa_threadpool_worker))) == NULL)
		return NULL;

	worker->threadpool = threadpool;
	worker->exit = 0;
	worker->id = id;
	return worker;
}

//-----------------------------------------------------------
// THREADPOOL //

esa_threadpool *esa_threadpool_init(int n_workers)
{
	esa_threadpool *threadpool = calloc(1, sizeof(esa_threadpool));
	if (!threadpool) return NULL;

	if (esa_command_queue_initp(&threadpool->command_queue))
		goto init_error;

	if (esa_mutex_init(&threadpool->mutex, NULL) != ESA_THREAD_RESULT_SUCCESS) goto init_error;
	if (esa_cond_init(&threadpool->cond, NULL) != ESA_THREAD_RESULT_SUCCESS) goto init_error;

	// initialize the array of workers
	threadpool->workers = malloc(n_workers  *sizeof(esa_threadpool_worker*));
	if (!threadpool->workers) goto init_error;

	// initialize each worker and start
	int i;
	for (i = 0; i < n_workers; i++) {
		// initialize worker
		threadpool->workers[i] = esa_threadpool_worker_init(threadpool, i);
		if (!threadpool->workers[i]) goto init_error;

		// start thread
		if (esa_thread_start(&threadpool->workers[i]->thread, NULL, esa_threadpool_worker_thread, (void*)threadpool->workers[i]) != ESA_THREAD_RESULT_SUCCESS) {
			goto init_error;
		}
	}

	threadpool->n_workers = n_workers;

	return threadpool;
	//---------------------
	init_error:
		// printf("\nesa_threadpool_init FAILED\n");
		esa_threadpool_free(threadpool);
		return NULL;
}

void esa_threadpool_free(esa_threadpool *threadpool)
{
	if (!threadpool) return;

	// threadpool->command_queue.head = threadpool->command_queue.tail = 0;

	// printf("esa_threadpool_free: freeing workers\n");
	// join worker's threads and free workers
	if (threadpool->workers) {
		int i;
		for (i = 0; i < threadpool->n_workers; i++) {
			esa_threadpool_worker_free(threadpool->workers[i], threadpool);
		}
		free(threadpool->workers);
	}
	// esa_mutex_unlock(&threadpool->mutex);

	// free command queue
	if (threadpool->command_queue.commands) {
		free(threadpool->command_queue.commands);
	}

	// destroy cond & mutex
	esa_cond_destroy(&threadpool->cond);
	esa_mutex_destroy(&threadpool->mutex);

	free(threadpool);
}

void esa_threadpool_join(esa_threadpool *threadpool)
{
	if (threadpool->workers) {
		int i;
		for (i = 0; i < threadpool->n_workers; i++) {
			esa_threadpool_worker *worker = threadpool->workers[i];
			if (worker) {
				worker->exit = 1;
				// printf("waking worker %i\n", worker->id);
				esa_cond_broadcast(&threadpool->cond);
				esa_thread_join(worker->thread, NULL);
			}
		}
	}
}

int esa_threadpool_push_command(esa_threadpool *threadpool, esa_command command)
{
	// int err = esa_mutex_trylock(&threadpool->mutex);
	// if (err == ESA_THREAD_RESULT_SUCCESS) {
	// 	printf("success!");
	// 	esa_cond_broadcast(&threadpool->cond);
	// 	esa_mutex_unlock(&threadpool->mutex);
	// }
	if (threadpool == NULL) return 1;
	return q_push(&threadpool->command_queue, command);
}

bool esa_threadpool_is_finished(esa_threadpool *threadpool)
{
	// printf("n_occupied is: %i\n", esa_command_queue_occupied(&threadpool->command_queue));
	return ESA_COMMAND_QUEUE_OCCUPIED(threadpool->command_queue) == 0;
}

//-----------------------------------------------------------

void *esa_threadpool_worker_thread(void *input)
{
	if (input == NULL) return NULL;
	esa_threadpool_worker *worker = (esa_threadpool_worker*)input;
	esa_threadpool *threadpool = worker->threadpool;

	int res = esa_setCurrentThreadMaxPriority();
	if (res != ESA_THREAD_RESULT_SUCCESS) {
		printf("Could not set thread to max priority!\n");
	}

	int err;
	esa_command *command = NULL;
	//-------------------------

	while (!worker->exit) {
		err = esa_mutex_lock(&threadpool->mutex);
		// sleep while waiting for a command
		while (ESA_COMMAND_QUEUE_OCCUPIED(threadpool->command_queue) == 0 && !worker->exit) {
			// printf("going to sleep (%i)\n", worker->id);
			esa_cond_wait(&threadpool->cond, &threadpool->mutex);
			// printf("awoken (%i)\n", worker->id);
		}
	
		esa_command *command = q_pop(&threadpool->command_queue);
#ifdef DEBUG
		printf("Worker %i will run command: %s\n", worker->id, command->name);
#endif
		esa_cond_broadcast(&threadpool->cond);

		esa_mutex_unlock(&threadpool->mutex);

		if (command) {
			command->func(command->input);
		}
#ifdef DEBUG
		printf("Worker %i finished command: %s\n", worker->id, command->name);
#endif
	}

	printf("Worker %i exiting. Over and out.\n", worker->id);
	return NULL;
}