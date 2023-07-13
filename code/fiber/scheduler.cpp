#include "scheduler.h"

namespace hxk
{
static Logger::_ptr g_logger = GET_LOGGER("system");

//当前线程的协程调度器
static thread_local Scheduler* t_scheduler = nullptr;
//协程调度器的调度工作协程
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t thread_size, bool use_caller, std::string name):m_name(name)
{
    assert(thread_size > 0);
    if(use_caller) {
        Fiber::getThis();   //实例化master fiber
        --thread_size;      //线程池需要的线程数减1

        assert(getThis() == nullptr);   //确保该线程下只有一个调度器
        t_scheduler = this;

        m_root_fiber = std::make_shared<Fiber>(std::bind(&Scheduler::run), this);

        Thread::setThisThreadName(m_name);

        t_scheduler_fiber = m_root_fiber.get();
        m_root_thread_id = GetThreadID();
        m_thread_id_list.push_back(m_root_thread_id);
    }
    else
    {
        m_root_thread_id = -1;

    }
    m_thread_count = thread_size;
}

Scheduler::~Scheduler()
{
    LOG_DEBUG(g_logger,"调用 Scheduler::~Scheduler()");
    assert(m_auto_stopped);
    if(getThis() ==  this) {
        t_scheduler = nullptr;
    }
}

void Scheduler::start()
{
    LOG_DEBUG(g_logger, "调用 Scheduler::start()");
    {
        ScopedLock loc(&m_mutex);
        if(!m_stopped) {
            return;     //调度器已经开始工作
        }
        m_stopped = false;
        assert(m_thread_list.empty());
        m_thread_list.resize(m_thread_count);
        for(size_t i = 0; i < m_thread_count; i++) {
            m_thread_list[i] = std::make_shared<Thread>(std::bind(&Scheduler::run, this), m_name + "_" + std::to_string(i));
            m_thread_id_list.push_back(m_thread_list[i]->getID());
        }
    }
}

void Scheduler::stop()
{
    LOG_DEBUG(g_logger, "调用 Scheduler::stop()");
    m_auto_stopped = true;

    for(size_t i=0; i< m_thread_count; i++) {
        tickle();
    }
    if(m_root_fiber) {
        tickle();
        if(!isStop()){
            m_root_fiber->call();
        }
    }
    {
        for(auto &t : m_thread_list) {
            t->join();
        }
        m_thread_list.clear();
    }
    if(onStop()) {
        return;
    }
}


bool Scheduler::hasFreeThread()
{
    return m_free_thread_count > 0;
}

bool Scheduler::isStop()
{
    return m_auto_stopped && m_task_list.empty() && m_active_thread_count == 0;
}

Scheduler* Scheduler::getThis()
{
    return t_scheduler;
}

Fiber* Scheduler::getMainFiber()
{   
    return t_scheduler_fiber;
}


void Scheduler::run()
{
    LOG_DEBUG(g_logger, "调用 Scheduler::run()");
    t_scheduler = this;

    
}

void Scheduler::tickle()
{

}

bool Scheduler::onStop()
{

}

void Scheduler::onFree()
{

}

}