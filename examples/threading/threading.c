#include "threading.h"
#include <pthread.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

// Optional: use these functions to add debug or error prints to your application
// #define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter

    struct thread_data* thread_func_args = (struct thread_data *) thread_param;

    usleep(thread_func_args->wait_to_obtain_ms * 1000.0);

    int ret_value = pthread_mutex_lock(thread_func_args->mutex);

    if (ret_value != 0){
        thread_func_args->thread_complete_success = false;
        ERROR_LOG("Failed to acquire mutex");
        return thread_param;
    }

    usleep(thread_func_args->wait_to_release_ms * 1000.0);

    ret_value = pthread_mutex_unlock(thread_func_args->mutex);

    if (ret_value != 0){
        thread_func_args->thread_complete_success = false;
        ERROR_LOG("Failed to acquire mutex");
        return thread_param;
    }

    thread_func_args->thread_complete_success = true;

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);

    // initialize thread data
    struct thread_data* data = (struct thread_data*)malloc(sizeof(struct thread_data));
    data->mutex = mutex;
    data->wait_to_obtain_ms = wait_to_obtain_ms;
    data->wait_to_release_ms = wait_to_release_ms;
    data->thread_complete_success = false;

    int ret_value = pthread_create(thread, &thread_attr, threadfunc, data);

    if (ret_value != 0){
        ERROR_LOG("Error occured while creating the thread");
        return false;
    }

    DEBUG_LOG("Thread started with ID %lu", *thread);


    return true;
}

