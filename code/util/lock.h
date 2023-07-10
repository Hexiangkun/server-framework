#pragma once

#include "noncopyable.h"
#include <semaphore.h>
#include <stdint.h>
#include <pthread.h>
#include <stdexcept>

namespace hxk
{

/**
 * @Author: hxk
 * @brief: 对semaphore信号量简单封装
 */
class Semaphore : public noncopyable
{
public:
    explicit Semaphore(uint32_t count = 0)
    {
        if(sem_init(&m_semaphore, 0, count)){
            //LOG
            throw std::logic_error("sem_init error");
        }
    }
    ~Semaphore()
    {
        sem_destroy(&m_semaphore);
    }

    void wait()    //-1操作，值为0时阻塞
    {
        if(sem_wait(&m_semaphore)){ //返回值为0代表成功，-1代表失败
            //LOG
            throw std::logic_error("sem_wait error");
        }
    }
    void notify()  //+1操作
    {
        if(sem_post(&m_semaphore)){

            throw std::logic_error("sem_post error");
        }
    }
private:
    sem_t m_semaphore;
};

/**
 * @Author: hxk
 * @brief: 作用域线程锁包装器
 * @return {*}
 */
template<typename T>
class ScopedLockImpl
{
public:
    explicit ScopedLockImpl(T* mutex) : m_mutex(mutex)
    {
        m_mutex->lock();
        m_locked = true;
    }
    ~ScopedLockImpl()
    {
        unlock();
    }

    void lock()
    {
        if(!m_locked){
            m_mutex->lock();
            m_locked = true;
        }
    }
    void unlock()
    {
        if(m_locked){
            m_locked = false;
            m_mutex->unlock();
        }
    }
private:
    T* m_mutex;
    bool m_locked;
};

/**
 * @Author: hxk
 * @brief: 作用域读写锁包装器
 * @return {*}
 */
template<typename T>
class ReadScopedLockImpl
{
public:
    explicit ReadScopedLockImpl(T* mutex) : m_mutex(mutex)
    {
        m_mutex->readLock();
        m_locked = true;
    }
    ~ReadScopedLockImpl()
    {
        unlock();
    }
    void lock()
    {
        if(!m_locked){
            m_mutex->readLock();
            m_locked = true;
        }
    }
    void unlock()
    {
        if(m_locked){
            m_locked = false;
            m_mutex->unlock();
        }
    }

private:
    T* m_mutex;
    bool m_locked;
};


template<typename T>
class WriteScopedLockImpl
{
public:
    explicit WriteScopedLockImpl(T* mutex) :m_mutex(mutex)
    {
        m_mutex->writeLock();
        m_locked = true;
    }
    ~WriteScopedLockImpl()
    {
        unlock();
    }
    void lock()
    {
        if(!m_locked){
            m_mutex->writeLock();
            m_locked = true;
        }
    }
    void unlock()
    {
        if(m_locked){
            m_locked = false;
            m_mutex->unlock();
        }
    }

private:
    T* m_mutex;
    bool m_locked;
};


/**
 * @Author: hxk
 * @brief: pthread互斥量封装
 * @return {*}
 */
class Mutex
{
public:
    Mutex()
    {
        pthread_mutex_init(&m_mutex, nullptr);
    }
    ~Mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    int lock()
    {
        return pthread_mutex_lock(&m_mutex);
    }
    int unlock()
    {
        return pthread_mutex_unlock(&m_mutex);
    }

private:
    pthread_mutex_t m_mutex{};
};

/**
 * @Author: hxk
 * @brief: pthread 读写锁封装
 * @return {*}
 */
class RWLock
{
public:
    RWLock()
    {
        pthread_rwlock_init(&m_lock, nullptr);
    }
    ~RWLock()
    {
        pthread_rwlock_destroy(&m_lock);
    }

    int readLock()
    {
        return pthread_rwlock_rdlock(&m_lock);
    }
    int writeLock()
    {
        return pthread_rwlock_wrlock(&m_lock);
    }
    int unlock()
    {
        return pthread_rwlock_unlock(&m_lock);
    }
private:
    pthread_rwlock_t m_lock{};
};


/**
 * @Author: hxk
 * @brief: Mutex的RAII
 * @return {*}
 */
using ScopedLock = ScopedLockImpl<Mutex>;

/**
 * @Author: hxk
 * @brief: 读写锁针对读操作的RAII实现
 * @return {*}
 */
using ReadScopedLock = ReadScopedLockImpl<RWLock>;

/**
 * @Author: hxk
 * @brief: 读写锁针对写操作的RAII实现
 * @return {*}
 */
using WriteScopedLock = WriteScopedLockImpl<RWLock>;

}