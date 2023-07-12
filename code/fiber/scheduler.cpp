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
        Fiber::getThis();
    }
}

}