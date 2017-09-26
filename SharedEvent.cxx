//
//  SharedEvent.cxx
//  IPC
//
//  Created by Brandon on 2017-09-24.
//  Copyright © 2017 Brandon. All rights reserved.
//

#include "SharedEvent.hxx"

#if defined(_WIN32) || defined(_WIN64)
/*int gettimeofday(struct timeval* tp, struct timezone* tz)
{
    FILETIME file_time;
    SYSTEMTIME system_time;
    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);

    static const uint64_t epoch = ((uint64_t)116444736000000000ULL);
    uint64_t time = static_cast<uint64_t>(file_time.dwLowDateTime) + (static_cast<uint64_t>(file_time.dwHighDateTime) << 32);

    tp->tv_sec  = static_cast<long>((time - epoch) / 10000000L);
    tp->tv_usec = static_cast<long>(system_time.wMilliseconds * 1000);
    return 0;
}*/

int gettimeofday(struct timeval* tp, struct timezone* tz)
{
    uint64_t time = get_file_time() - 116444736000000000ULL;
    tp->tv_sec = static_cast<long>(time / 10000000ULL);
    tp->tv_usec = static_cast<long>(time % 10000000ULL) / 10;
    return 0;
}
#endif // defined


int pthread_mutex_timedlock (pthread_mutex_t *mutex, const struct timespec *timeout)
{
    int retcode = 0;
    struct timeval now;
    struct timespec sleeptime;
    
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 10000000; //10ms
    
    while ((retcode = pthread_mutex_trylock (mutex)) == EBUSY)
    {
        gettimeofday(&now, nullptr);
        
        if (now.tv_sec >= timeout->tv_sec && (now.tv_usec * 1000) >= timeout->tv_nsec)
        {
            return ETIMEDOUT;
        }
        
        nanosleep (&sleeptime, nullptr);
    }
    
    return retcode;
}


Mutex::Mutex() : shared(false), info(new shared_mutex_info)
{
    info->ref_count = 0;
    pthread_mutexattr_init(&info->mutex_attr);
    pthread_mutexattr_setpshared(&info->mutex_attr, PTHREAD_PROCESS_PRIVATE);
    pthread_mutex_init(&info->mutex, &info->mutex_attr);
}

Mutex::Mutex(void* shm) : shared(true), info(static_cast<shared_mutex_info*>(shm))
{
    if(!info->ref_count)
    {
        info->ref_count = 0;
        pthread_mutexattr_init(&info->mutex_attr);
        pthread_mutexattr_setpshared(&info->mutex_attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&info->mutex, &info->mutex_attr);
    }
    ++info->ref_count;
}

Mutex::~Mutex()
{
    if(--info->ref_count <= 0)
    {
        pthread_mutex_unlock(&info->mutex);
        pthread_mutex_destroy(&info->mutex);
        pthread_mutexattr_destroy(&info->mutex_attr);
        delete(!shared ? info : nullptr);
    }
}

bool Mutex::lock()
{
    int res = pthread_mutex_lock(&info->mutex);
    info->ref_count += res != EDEADLK && !res ? 1 : 0;
    return res != EDEADLK && !res;
}

bool Mutex::try_lock()
{
    int res = 0;
    do
    {
        res = pthread_mutex_trylock(&info->mutex);
    }
    while(res == EINTR);
    info->ref_count += res != EBUSY && res != EDEADLK && !res ? 1 : 0;
    return res != EBUSY && res != EDEADLK && !res;
}


template<typename Rep, typename Period>
bool Mutex::try_lock_for(const std::chrono::duration<Rep, Period>& relative_time)
{
    std::chrono::steady_clock::duration rtime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(relative_time);
    if(std::ratio_greater<std::chrono::steady_clock::period, Period>())
    {
        ++rtime;
    }
    return try_lock_until(std::chrono::steady_clock::now() + rtime);
}

template<typename Duration>
bool Mutex::try_lock_until(const std::chrono::time_point<std::chrono::high_resolution_clock, Duration>& absolute_time)
{
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::seconds> sec = std::chrono::time_point_cast<std::chrono::seconds>(absolute_time);
    std::chrono::nanoseconds nano = std::chrono::duration_cast<std::chrono::nanoseconds>(absolute_time - sec);

    struct timespec ts =
    {
        static_cast<std::time_t>(sec.time_since_epoch().count()),
        static_cast<long>(nano.count())
    };

    return !pthread_mutex_timedlock(&info->mutex, &ts);
}

template<typename Clock, typename Duration>
bool Mutex::try_lock_until(const std::chrono::time_point<Clock, Duration>& absolute_time)
{
    return try_lock_until(std::chrono::high_resolution_clock::now() + (absolute_time - Clock::now()));
}

bool Mutex::timed_lock(unsigned long milliseconds)
{
    if(!milliseconds)
    {
        return lock();
    }

    #if defined(_WIN32) || defined(_WIN64)
    struct timespec ts;
    struct timeval now;
    gettimeofday(&now, nullptr);
    ts.tv_sec = now.tv_sec + milliseconds / 1000;
    ts.tv_nsec = now.tv_usec * 1000 + (milliseconds % 1000) * 1000000;
    ts.tv_sec += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;
    #else
    struct timespec ts;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    ts.tv_sec = now.tv_sec + milliseconds / 1000;
    ts.tv_nsec = now.tv_nsec + (milliseconds % 1000) * 1000000;
    ts.tv_sec += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;
    #endif

    int res = pthread_mutex_timedlock(&info->mutex, &ts);
    info->ref_count += res != ETIMEDOUT && res != EDEADLK && !res ? 1 : 0;
    return res != ETIMEDOUT && res != EDEADLK && !res;
}

bool Mutex::unlock()
{
    bool res = !pthread_mutex_unlock(&info->mutex);
    info->ref_count -= res ? 1 : 0;
    return res;
}



SharedEvent::SharedEvent(const char* name) : mutex_info(), name(name)
{
    
}

SharedEvent::~SharedEvent()
{

}








Semaphore::Semaphore() : shared(false), info(new shared_semaphore_info)
{
    info->ref_count = 0;
    pthread_mutexattr_init(&info->mutex_attr);
    pthread_condattr_init(&info->condition_attr);
    pthread_mutexattr_setpshared(&info->mutex_attr, PTHREAD_PROCESS_PRIVATE);
    pthread_condattr_setpshared(&info->condition_attr, PTHREAD_PROCESS_PRIVATE);
    pthread_mutex_init(&info->mutex, &info->mutex_attr);
    pthread_cond_init(&info->condition, &info->condition_attr);
    info->count = 0;
}

Semaphore::Semaphore(void* shm) : shared(true), info(static_cast<shared_semaphore_info*>(shm))
{
    if(!info->ref_count)
    {
        info->ref_count = 0;
        pthread_mutexattr_init(&info->mutex_attr);
        pthread_condattr_init(&info->condition_attr);
        pthread_mutexattr_setpshared(&info->mutex_attr, PTHREAD_PROCESS_SHARED);
        pthread_condattr_setpshared(&info->condition_attr, PTHREAD_PROCESS_SHARED);
        pthread_mutex_init(&info->mutex, &info->mutex_attr);
        pthread_cond_init(&info->condition, &info->condition_attr);
        info->count = 0;
    }
    ++info->ref_count;
}

Semaphore::~Semaphore()
{
    if(--info->ref_count <= 0)
    {
        pthread_cond_destroy(&info->condition);
        pthread_condattr_destroy(&info->condition_attr);
        
        pthread_mutex_unlock(&info->mutex);
        pthread_mutex_destroy(&info->mutex);
        pthread_mutexattr_destroy(&info->mutex_attr);
        delete(!shared ? info : nullptr);
    }
}

bool Semaphore::wait()
{
    int res = pthread_mutex_lock(&info->mutex);
    
    if (!res && res != EBUSY && res != EDEADLK)
    {
        while (info->count <= 0)
        {
            pthread_cond_wait(&info->condition, &info->mutex);
        }
        
        --info->count;
        pthread_mutex_unlock(&info->mutex);
        return true;
    }
    return false;
}

bool Semaphore::try_wait()
{
    int res = 0;
    do
    {
        res = pthread_mutex_trylock(&info->mutex);
    }
    while(res == EINTR);
    
    if (!res && res != EBUSY && res != EDEADLK)
    {
        while (info->count <= 0)
        {
            pthread_cond_wait(&info->condition, &info->mutex);
        }
        
        --info->count;
        pthread_mutex_unlock(&info->mutex);
        return true;
    }
    
    return false;
}


template<typename Rep, typename Period>
bool Semaphore::try_wait_for(const std::chrono::duration<Rep, Period>& relative_time)
{
    std::chrono::steady_clock::duration rtime = std::chrono::duration_cast<std::chrono::steady_clock::duration>(relative_time);
    if(std::ratio_greater<std::chrono::steady_clock::period, Period>())
    {
        ++rtime;
    }
    return try_wait_until(std::chrono::steady_clock::now() + rtime);
}

template<typename Duration>
bool Semaphore::try_wait_until(const std::chrono::time_point<std::chrono::high_resolution_clock, Duration>& absolute_time)
{
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::seconds> sec = std::chrono::time_point_cast<std::chrono::seconds>(absolute_time);
    std::chrono::nanoseconds nano = std::chrono::duration_cast<std::chrono::nanoseconds>(absolute_time - sec);
    
    struct timespec ts =
    {
        static_cast<std::time_t>(sec.time_since_epoch().count()),
        static_cast<long>(nano.count())
    };
    
    int res = pthread_mutex_lock(&info->mutex);
    
    if (!res && res != EBUSY && res != EDEADLK)
    {
        while (info->count == 0)
        {
            int res2 = pthread_cond_timedwait(&info->condition, &info->mutex, &ts);
            if (res2 == ETIMEDOUT || res2 == EINVAL)
            {
                pthread_mutex_unlock(&info->mutex);
                return false;
            }
            
            --info->count;
            pthread_mutex_unlock(&info->mutex);
            return true;
        }
    }
    return false;
}

template<typename Clock, typename Duration>
bool Semaphore::try_wait_until(const std::chrono::time_point<Clock, Duration>& absolute_time)
{
    return try_wait_until(std::chrono::high_resolution_clock::now() + (absolute_time - Clock::now()));
}

bool Semaphore::timed_wait(unsigned long milliseconds)
{
    if(!milliseconds)
    {
        return wait();
    }
    
    #if defined(_WIN32) || defined(_WIN64)
    struct timespec ts;
    struct timeval now;
    gettimeofday(&now, nullptr);
    ts.tv_sec = now.tv_sec + milliseconds / 1000;
    ts.tv_nsec = now.tv_usec * 1000 + (milliseconds % 1000) * 1000000;
    ts.tv_sec += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;
    #else
    struct timespec ts;
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    ts.tv_sec = now.tv_sec + milliseconds / 1000;
    ts.tv_nsec = now.tv_nsec + (milliseconds % 1000) * 1000000;
    ts.tv_sec += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;
    #endif
    
    int res = pthread_mutex_lock(&info->mutex);
    
    if (!res && res != EBUSY && res != EDEADLK)
    {
        while (info->count == 0)
        {
            int res2 = pthread_cond_timedwait(&info->condition, &info->mutex, &ts);
            if (res2 == ETIMEDOUT || res2 == EINVAL)
            {
                pthread_mutex_unlock(&info->mutex);
                return false;
            }
            
            --info->count;
            pthread_mutex_unlock(&info->mutex);
            return true;
        }
    }
    return false;
}

bool Semaphore::signal()
{
    int res = pthread_mutex_lock(&info->mutex);
    if (!res && res != EBUSY && res != EDEADLK)
    {
        ++info->count;
        pthread_cond_signal(&info->condition);
        pthread_mutex_unlock(&info->mutex);
        return true;
    }
    return false;
}

bool Semaphore::signal_all()
{
    int res = pthread_mutex_lock(&info->mutex);
    if (!res && res != EBUSY && res != EDEADLK)
    {
        info->count = 0;
        pthread_cond_broadcast(&info->condition);
        pthread_mutex_unlock(&info->mutex);
        return true;
    }
    return false;
}
