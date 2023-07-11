#include "fiber.h"

namespace hxk
{
static Logger::_ptr g_logger = GET_LOGGER("system");

//master fiber
Fiber::Fiber(): m_id(0), m_stack_size(0),m_state(EXEC), m_ucontext(), m_stack(nullptr),m_callback()
{
    setThis(this);

    //获取上下文对象，将当前上下文保存到m_ucontext中
    if(getcontext(&m_ucontext)) {
        throw Exception(std::string(strerror(errno)));
    }

    ++FiberInfo::s_fiber_count; //存在协程数量增加

    LOG_FORMAT_DEBUG(g_logger, "use Fiber::Fiber() to create master fiber, thread id = %ld, fiber id = %ld.",GetThreadID(), m_id);
}

Fiber::Fiber(FiberFunc callback, size_t stack_size):m_id(++FiberInfo::s_fiber_id),
                                                    m_stack_size(stack_size),
                                                    m_state(INIT),
                                                    m_ucontext(),
                                                    m_stack(nullptr),
                                                    m_callback(callback)
{   
    //如果stack_size为0，使用config文件的fiber.stack_size
    if(stack_size == 0) {
        m_stack_size = FiberInfo::g_fiber_stack_size->getValue();
    }
    //获取当前协程的上下文信息
    if(getcontext(&m_ucontext)){
        throw Exception(std::string(strerror(errno)));
    }
    m_stack = stackAllocator::Alloc(m_stack_size);
    m_ucontext.uc_link = nullptr;
    m_ucontext.uc_stack.ss_sp = m_stack;
    m_ucontext.uc_stack.ss_size = m_stack_size;

    makecontext(&m_ucontext, &Fiber::mainFunc, 0);
    ++FiberInfo::s_fiber_count;
}

Fiber::~Fiber()
{
    if(m_stack){    //存在栈，说明是子协程
        assert(m_state == INIT || m_state == TERM || m_state == EXCEPTION) ;
        stackAllocator::Delalloc(m_stack, m_stack_size);
    }
    else{   //为master fiber
        assert(!m_callback);
        assert(m_state == EXEC);
        if(FiberInfo::t_fiber == this) {
            setThis(nullptr);
        }
    }
}

void Fiber::reset(FiberFunc callback)
{
    assert(m_stack);
    assert(m_state == INIT || m_state == TERM || m_state == EXCEPTION);
    m_callback = std::move(callback);
    if(getcontext(&m_ucontext)){
        throw Exception(std::string(strerror(errno)));
    }
    m_ucontext.uc_link = nullptr;
    m_ucontext.uc_stack.ss_sp = m_stack;
    m_ucontext.uc_stack.ss_size = m_stack_size;
    makecontext(&m_ucontext, &Fiber::mainFunc, 0);
    m_state = INIT;
}

void Fiber::swapIn()
{
    //只有协程是等待执行的状态才能被换入
    assert(m_state == INIT || m_state == READY || m_state == HOLD);
    setThis(this);
    m_state = EXEC;
    //assert(Scheduler) ??????
    
}

void Fiber::setThis(Fiber* fiber)   //设置正在执行的协程
{
    FiberInfo::t_fiber = fiber;
}

}