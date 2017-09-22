#ifndef ESA_THREADS_H
#define ESA_THREADS_H

typedef enum {
	ESA_THREAD_RESULT_FAILURE = -1,
	ESA_THREAD_RESULT_SUCCESS = 0
} esa_thread_result_t;

// Windows
// TODO: figure out windows threads if we ever get there
#ifdef WIN32
#define esa_thread pthread_t
#define esa_nice nice
#define esa_thread_start pthread_create
#define esa_thread_join pthread_join

#define esa_mutex pthread_mutex_t
#define esa_mutex_init pthread_mutex_init
#define esa_mutex_lock pthread_mutex_lock
#define esa_mutex_trylock pthread_mutex_trylock
#define esa_mutex_unlock pthread_mutex_unlock
#define esa_mutex_destroy pthread_mutex_destroy

#define esa_cond pthread_cond_t
#define esa_cond_init pthread_cond_init
#define esa_cond_signal pthread_cond_signal
#define esa_cond_broadcast pthread_cond_broadcast
#define esa_cond_destroy pthread_cond_destroy

esa_thread_result_t esa_setCurrentThreadMaxPriority();
esa_thread_result_t esa_setThreadMaxPriority();


// Unix (android, mac, osx, etc.)
#else
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>

#define esa_thread pthread_t
#define esa_nice nice
#define esa_thread_start pthread_create
#define esa_thread_join pthread_join
// #define esa_thread_set_name(a) pthread_setname_np(a)
// #define esa_thread_get_name(a, b) pthread_getname_np(pthread_self(), a, b)
#define esa_mutex pthread_mutex_t
#define esa_mutex_init pthread_mutex_init
#define esa_mutex_lock pthread_mutex_lock
#define esa_mutex_trylock pthread_mutex_trylock
#define esa_mutex_unlock pthread_mutex_unlock
#define esa_mutex_destroy pthread_mutex_destroy

#define esa_cond pthread_cond_t
#define esa_cond_init pthread_cond_init
#define esa_cond_wait pthread_cond_wait
#define esa_cond_signal pthread_cond_signal
#define esa_cond_broadcast pthread_cond_broadcast
#define esa_cond_destroy pthread_cond_destroy

esa_thread_result_t esa_setThreadMaxPriority(pthread_t thread);
esa_thread_result_t esa_setCurrentThreadMaxPriority();

#endif // end #ifdef WIN32

static inline void nssleep(long ns)
{
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = ns;
	nanosleep(&ts, NULL);
}

#endif // end #ifdef ESA_THREADS_H
