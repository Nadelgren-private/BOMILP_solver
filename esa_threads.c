#include "esa_threads.h"
// #include "esa_print.h"

#ifdef WIN32
esa_thread_result_t esa_setCurrentThreadMaxPriority() {
	return ESA_THREAD_RESULT_FAILURE;
}

esa_thread_result_t esa_setThreadMaxPriority(pthread_t thread) {
	return ESA_THREAD_RESULT_FAILURE;
}

#else
esa_thread_result_t esa_setThreadMaxPriority(pthread_t thread)
{
	// set policy and priority
	struct sched_param params;
	params.sched_priority = sched_get_priority_max(SCHED_FIFO);
	int ret = pthread_setschedparam(thread, SCHED_FIFO, &params);
	if (ret != 0) return ESA_THREAD_RESULT_FAILURE;

	// check policy and priority
	int policy = 0;
	ret = pthread_getschedparam(thread, &policy, &params);
	if (ret != 0 || policy != SCHED_FIFO)
		return ESA_THREAD_RESULT_FAILURE;

	return ESA_THREAD_RESULT_SUCCESS;
}

esa_thread_result_t esa_setCurrentThreadMaxPriority() {
#ifdef __ANDROID__
	esa_nice(-20);
	return ESA_THREAD_RESULT_SUCCESS;
#else
	return esa_setThreadMaxPriority(pthread_self());
#endif
}
#endif