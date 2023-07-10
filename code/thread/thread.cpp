#include "thread.h"

namespace hxk
{

static thread_local pid_t t_tid = 0;
// 记录当前线程的线程名
static thread_local std::string t_thread_name = "UNKNOWN";

static Logger::_ptr system_logger = GET_LOGGER("system");

struct ThreadData
{
public:
    typedef Thread::ThreadFunc ThreadFunc;
    ThreadFunc m_callback;
    std::string m_name;   
    pid_t* m_id;
    Semaphore* m_semaphore; 

    ThreadData(ThreadFunc func, const std::string& name, pid_t* id, Semaphore* semaphore)
            :m_callback(std::move(func)),m_name(name), m_id(id), m_semaphore(semaphore)
            {

            }
    
    void runThread()
    {
        *m_id = GetThreadID();  //获取系统线程id
        m_id = nullptr;

        m_semaphore->notify();  //信号量+1，通知主线程，子线程启动成功
        m_semaphore = nullptr;
        
        t_tid = GetThreadID();
        t_thread_name = m_name.empty() ? "UNKNOWN" : m_name;

        pthread_setname_np(pthread_self(), m_name.substr(0,15).c_str());

        try
        {
            m_callback();
        }
        catch(const std::exception& e)
        {
            LOG_FORMAT_FATAL(system_logger,"thread excu error, name: %s, reason:%s",m_name.c_str(), e.what());
            std::cerr << e.what() << '\n';
        }
    }
};


Thread::Thread(ThreadFunc callback, const std::string& name)
                    : m_id(-1),
                    m_name(name),
                    m_thread(0),
                    m_callback(callback),
                    m_semaphore(0),
                    m_started(true),
                    m_joined(false)
{
    ThreadData* data = new ThreadData(m_callback, m_name, &m_id, &m_semaphore);

    int result = pthread_create(&m_thread, nullptr, &Thread::run, data);

    if(result) {
        m_started = false;
        delete data;
        LOG_FORMAT_FATAL(system_logger, "pthread_create() error, name: %s, error code: %d", m_name.c_str(), result);
        throw std::system_error();
    }
    else {
        m_semaphore.wait();
        assert(m_id > 0);
    }
}
Thread::~Thread()
{
    if(m_started && !m_joined) {
        pthread_detach(m_thread);
    }
}

pid_t Thread::getID() const
{
    return m_id;
}

const std::string Thread::getName() const
{
    return m_name;
}

void Thread::setName(const std::string& name)
{
    if(name.empty()){
        return;
    }
    m_name = name;
}

int Thread::join()
{
    assert(m_started);
    assert(!m_joined);
    m_joined = true;
    // if(m_thread){
    //     if(pthread_join(m_thread, nullptr)){

    //     }
    // }
    return pthread_join(m_thread, nullptr);
}

pid_t Thread::getThisThreadID()
{
    return t_tid;
}

const std::string Thread::getThisThreadName()
{
    return t_thread_name;
}

void Thread::setThisThreadName(const std::string& name)
{
    if(name.empty()){
        return;
    }
    t_thread_name = name;
}

void* Thread::run(void* arg)
{
    std::unique_ptr<ThreadData> data((ThreadData*)arg);
    data->runThread();
    return 0;
}

}