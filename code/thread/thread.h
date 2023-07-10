#pragma once

#include "noncopyable.h"
#include "lock.h"
#include "util.h"
#include "log.h"
#include <sys/types.h>
#include <string>
#include <pthread.h>
#include <functional>
#include <memory>

namespace hxk
{

class Thread : public noncopyable
{
public:
    typedef std::function<void()> ThreadFunc;
    typedef std::shared_ptr<Thread> _ptr;
    typedef std::unique_ptr<Thread> _uptr;

    Thread(ThreadFunc callback, const std::string& name);
    ~Thread();

    pid_t getID() const;    //线程ID

    const std::string getName() const;  //获取线程的名称

    void setName(const std::string& name);  //设置线程名称

    int join();    //线程并入主线程，主线程等待子线程完成

public:
    static pid_t getThisThreadID();   //获取当先线程的ID

    static const std::string getThisThreadName();

    static void setThisThreadName(const std::string& name);

    static void* run(void* arg);    

private:
    pid_t m_id; //系统线程id，通过syscall获取

    std::string m_name;     //线程名称 
    
    pthread_t m_thread;     //pthread线程id

    ThreadFunc m_callback;  //线程执行函数

    Semaphore m_semaphore;  //控制线程启动的信号量

    bool m_started;         //线程状态     
    bool m_joined;    
};



}


