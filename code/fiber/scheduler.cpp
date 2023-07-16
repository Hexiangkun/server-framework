#include "scheduler.h"

namespace hxk
{
static Logger::_ptr g_logger = GET_LOGGER("system");

//当前线程的协程调度器
static thread_local Scheduler* t_scheduler = nullptr;
//协程调度器的调度工作协程
static thread_local Fiber* t_scheduler_fiber = nullptr;

Scheduler::Scheduler(size_t thread_size, bool use_caller, std::string name):m_name(name),
                                                                        m_active_thread_count(0),
                                                                        m_free_thread_count(0),
                                                                        m_stopped(true),
                                                                        m_auto_stopped(false),
                                                                        m_root_thread_id(0)
{
    assert(thread_size > 0);
    if(use_caller) {
        Fiber::getThis();   //实例化此类的线程作为master fiber
        --thread_size;      //线程池需要的线程数减1

        assert(getThis() == nullptr);   //确保该线程下只有一个调度器
        t_scheduler = this;

        //use_caller为真时有效，调度协程
        m_root_fiber = std::make_shared<Fiber>(std::bind(&Scheduler::run,this));

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

void Scheduler::start() //创建线程池
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

    //use_caller为true，并且指定线程数量为1时，说明只有一条主线程在运行，简单等待执行结束即可
    if(m_root_fiber && m_thread_count == 0 && (m_root_fiber->finish() || m_root_fiber->getState() == Fiber::INIT)) {
        m_stopped = true;
        if(onStop()) {      //用户自定义的回调函数来结束停止过程
            return;
        }
    }

    m_stopped = true;
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
    //任务列表没有新任务，也没有正在执行的任务，说明调度器已经停止工作
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

    setHookEnable(true);

    if(GetThreadID() != m_root_thread_id) { //判断执行run函数的线程是否为线程池中的线程
        t_scheduler_fiber = Fiber::getThis().get(); //当前线程不存在master fiber，创建一个,与fiber类中的master fiber共享一个fiber
    }

    //线程空闲时执行的协程
    auto free_fiber = std::make_shared<Fiber>(std::bind(&Scheduler::onFree,this));

    Task task;  //开始调度
    while(true) {
        task.reset();

        bool tickle_me = false;
        //查找等待调度的task
        {
            ScopedLock lock(&m_mutex);
            auto iter = m_task_list.begin();
            while(iter != m_task_list.end()) {

                //任务需要在指定线程运行，但是不是当前线程
                if((*iter)->m_thread_id != -1 && (*iter)->m_thread_id != GetThreadID()) {
                    ++iter;
                    tickle_me = true;
                    continue;
                }
                assert((*iter)->m_fiber || (*iter)->m_callback);
                //任务为fiber，但是正在执行
                if((*iter)->m_fiber && (*iter)->m_fiber->getState() == Fiber::EXEC) {
                    ++iter;
                    continue;
                }

                task = **iter;  //找到可执行的任务，拷贝一份
                ++m_active_thread_count;
                m_task_list.erase(iter);
                break;
            }
        }
        if(tickle_me) {//存在需要唤醒的任务
            tickle();
        }
        if(task.m_callback) {   //为callback任务，为其创建task
            task.m_fiber = std::make_shared<Fiber>(std::move(task.m_callback));
            task.m_callback = nullptr;
        }
        if(task.m_fiber && !task.m_fiber->finish()) {
            //m_root_thread_id等于当前线程id，说明use_caller为true
            //使用m_root_fiber作为master fiber
            if(GetThreadID() == m_root_thread_id) {
                task.m_fiber->swapIn();
            }
            else {
                task.m_fiber->swapIn();
            }

            --m_active_thread_count;

            Fiber::STATE fiber_status = task.m_fiber->getState();
            if(fiber_status == Fiber::READY) {
                schedule(std::move(task.m_fiber), task.m_thread_id);
            }
            else if(fiber_status != Fiber::EXCEPTION && fiber_status != Fiber::TERM) {
                task.m_fiber->m_state = Fiber::HOLD;
            }
            task.reset();
        }
        else {
            if(free_fiber->finish()) {
                break;
            }
            ++m_free_thread_count;
            free_fiber->swapIn();
            --m_free_thread_count;
            if(free_fiber->getState() != Fiber::TERM && free_fiber->getState() != Fiber::EXCEPTION) {
                free_fiber->m_state = Fiber::HOLD;
            }
        }
    }   
    LOG_DEBUG(g_logger, "Scheduler::run() end");
}

void Scheduler::tickle()
{
    LOG_DEBUG(g_logger, "tickle");
}

bool Scheduler::onStop()
{
    return isStop();
}

void Scheduler::onFree()
{
    while (!isStop())
    {
        Fiber::yieldToHold();
    }
    return;
}

}