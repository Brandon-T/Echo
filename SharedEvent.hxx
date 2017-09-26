//
//  SharedEvent.hxx
//  IPC
//
//  Created by Brandon on 2017-09-24.
//  Copyright © 2017 Brandon. All rights reserved.
//

#ifndef SHAREDEVENT_HXX_INCLUDED
#define SHAREDEVENT_HXX_INCLUDED

#if defined(_WIN32) || defined(_WIN64)
#include <pthread.h>
#include <windows.h>
#else
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include <ctime>
#include <cstdint>
#include <string>
#include <chrono>

#include "Time.hxx"

#ifndef _POSIX_THREAD_PROCESS_SHARED
//#error SHARED_MUTEXES NOT SUPPORTED
#endif

class Mutex
{
private:
    typedef struct
    {
        int ref_count;
        pthread_mutex_t mutex;
        pthread_mutexattr_t mutex_attr;
    } shared_mutex_info;

    bool shared;
    shared_mutex_info* info;

public:
    Mutex();
    Mutex(void* shm);
    ~Mutex();

    Mutex(const Mutex &other) = delete;
    Mutex& operator = (const Mutex &other) = delete;

    static constexpr std::size_t MutexSize() {return sizeof(shared_mutex_info);}

    shared_mutex_info* data() {return info;}
    const shared_mutex_info* data() const {return info;}

    bool lock();
    bool try_lock();
    bool timed_lock(unsigned long milliseconds);
    bool unlock();


    template<typename Rep, typename Period>
    bool try_lock_for(const std::chrono::duration<Rep, Period>& relative_time);

    template<typename Duration>
    bool try_lock_until(const std::chrono::time_point<std::chrono::high_resolution_clock, Duration>& absolute_time);

    template<typename Clock, typename Duration>
    bool try_lock_until(const std::chrono::time_point<Clock, Duration>& absolute_time);
};

class Semaphore
{
private:
    typedef struct
    {
        int ref_count;
        pthread_mutex_t mutex;
        pthread_mutexattr_t mutex_attr;
        pthread_cond_t condition;
        pthread_condattr_t condition_attr;
        int count;
    } shared_semaphore_info;
    
    bool shared;
    shared_semaphore_info* info;
    
public:
    Semaphore();
    Semaphore(void* shm);
    ~Semaphore();
    
    Semaphore(const Semaphore &other) = delete;
    Semaphore& operator = (const Semaphore &other) = delete;
    
    static constexpr std::size_t SemaphoreSize() {return sizeof(shared_semaphore_info);}
    
    shared_semaphore_info* data() {return info;}
    const shared_semaphore_info* data() const {return info;}
    
    bool wait();
    bool try_wait();
    bool timed_wait(unsigned long milliseconds);
    bool signal();
    bool signal_all();
    
    
    template<typename Rep, typename Period>
    bool try_wait_for(const std::chrono::duration<Rep, Period>& relative_time);
    
    template<typename Duration>
    bool try_wait_until(const std::chrono::time_point<std::chrono::high_resolution_clock, Duration>& absolute_time);
    
    template<typename Clock, typename Duration>
    bool try_wait_until(const std::chrono::time_point<Clock, Duration>& absolute_time);
};

class SharedEvent
{
private:
    Mutex* mutex_info;
    std::string name;

public:
    SharedEvent(const char* name);
    ~SharedEvent();

    SharedEvent(const SharedEvent &other) = delete;
    SharedEvent& operator = (const SharedEvent &other) = delete;
};

#endif // SHAREDEVENT_HXX_INCLUDED
