#include "fiber.h"
#include "scheduler.h"
#include "exception.h"
#include "log.h"
namespace hxk
{
static Logger::_ptr g_logger = GET_LOGGER("system");


//master fiber
Fiber::Fiber(): m_id(0), m_stack_size(0),m_state(EXEC), m_ucontext(), m_stack(nullptr),m_callback()
{
    setThis(this);  //设置当前线程正在执行的协程

    //获取上下文对象，将当前上下文保存到m_ucontext中
    if(getcontext(&m_ucontext)) {
        throw Exception(std::string(strerror(errno)));
    }

    ++FiberInfo::s_fiber_count; //存在协程数量增加

    LOG_FORMAT_DEBUG(g_logger, "use Fiber::Fiber() to create master fiber, thread id = %ld, fiber id = %ld.",GetThreadID(), m_id);
}

Fiber::Fiber(FiberFunc callback, size_t stack_size, bool use_caller):m_id(++FiberInfo::s_fiber_id),
                                                    m_stack_size(stack_size),
                                                    m_state(INIT),
                                                    m_ucontext(),
                                                    m_stack(nullptr),
                                                    m_callback(std::move(callback))
{   
    //如果stack_size为0，使用config文件的fiber.stack_size
    if(stack_size == 0) {
        m_stack_size = FiberInfo::g_fiber_stack_size->getValue();
    }
    //获取当前协程的上下文信息
    if(getcontext(&m_ucontext)){
        throw Exception(std::string(strerror(errno)));
    }
    m_stack = stackAllocator::Alloc(m_stack_size);  //为当前协程分配栈空间
    m_ucontext.uc_link = nullptr;                   //nullptr代表当前协程执行完毕，线程就退出
    m_ucontext.uc_stack.ss_sp = m_stack;
    m_ucontext.uc_stack.ss_size = m_stack_size;

    if(!use_caller){
        makecontext(&m_ucontext, &Fiber::mainFunc, 0);  //给上下文绑定入口函数Fiber::mainFunc
    }
    else{
        makecontext(&m_ucontext, &Fiber::callerMainFunc, 0);
    }
    ++FiberInfo::s_fiber_count;

    LOG_FORMAT_DEBUG(g_logger,"调用Fiber::Fiber 创建新的协程，线程 = %ld,协程 = %ld",
            GetThreadID(), m_id);
}

Fiber::~Fiber()
{
    --FiberInfo::s_fiber_count;
    if(m_stack){    //存在栈，说明是子协程
        //只有子协程未运行或者异常，才能被销毁
        assert(m_state == INIT || m_state == TERM || m_state == EXCEPTION) ;
        stackAllocator::Delalloc(m_stack, m_stack_size);
            // LOG_FORMAT_DEBUG(g_logger,
            //      "调用 Fiber::~Fiber 析构协程，thread_id = %ld, fiber_id = %ld",
            //      GetThreadID(), m_id);
    }
    else{   //为master fiber
        assert(!m_callback);
        assert(m_state == EXEC);
        // LOG_FORMAT_DEBUG(g_logger,
        //          "调用 master Fiber::~Fiber 析构协程，thread_id = %ld, fiber_id = %ld",
        //          GetThreadID(), m_id);
        if(FiberInfo::t_fiber == this) {
            setThis(nullptr);
        }
    }
}

void Fiber::reset(FiberFunc callback)   //重新设置协程的执行函数
{
    assert(m_stack);    //当前协程必须存在栈空间
    //协程不在运行状态
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

    //挂起master fiber，切换到当前fiber
    assert(Scheduler::getMainFiber() && "请勿手动调用该函数");
    //执行m_ucontext的上下文，如果m-ucontext.uc_link为空，则线程结束
    if(swapcontext(&(Scheduler::getMainFiber()->m_ucontext), &m_ucontext)){
        throw Exception(std::string(strerror(errno)));
    }
}

void Fiber::swapOut()   //和scheduler的master fiber切换
{
    assert(m_stack);
    setThis(FiberInfo::t_master_fiber.get());       // ?????????????

    //挂起当前fiber，切换到master fiber
    assert(Scheduler::getMainFiber() && "请勿手动调用该函数");
    if(swapcontext(&m_ucontext, &(Scheduler::getMainFiber()->m_ucontext))){
        throw Exception(std::string(strerror(errno)));
    }
}

void Fiber::call()  //从master fiber切换到当前的协程
{
    assert(FiberInfo::t_master_fiber && "当前线程不存在主协程");
    assert(m_state == INIT || m_state == READY || m_state == HOLD);
    setThis(this);
    m_state = EXEC;
    if(swapcontext(&(FiberInfo::t_master_fiber->m_ucontext), &m_ucontext)){
        throw Exception(std::string(strerror(errno)));
    }
}

void Fiber::back()//从当前协程切换回主协程master fiber
{
    assert(FiberInfo::t_master_fiber && "当前线程不存在主线程");
    assert(m_stack);
    setThis(FiberInfo::t_master_fiber.get());
    if(swapcontext(&m_ucontext, &(FiberInfo::t_master_fiber->m_ucontext))){
        throw Exception(std::string(strerror(errno)));
    }
}

void Fiber::swapIn(Fiber::_ptr fiber_ptr)
{
    assert(m_state == INIT || m_state == READY || m_state == HOLD);
    setThis(this);
    m_state = EXEC;
    if(swapcontext(&(fiber_ptr->m_ucontext), &m_ucontext)){
        throw Exception(std::string(strerror(errno)));
    }
}

void Fiber::swapOut(Fiber::_ptr fiber_ptr)
{
    assert(m_state);
    setThis(fiber_ptr.get());
    m_state = EXEC;
    if(swapcontext(&m_ucontext, &(fiber_ptr->m_ucontext))) {
        throw Exception(std::string(strerror(errno)));
    }
}

uint64_t Fiber::getId()
{
    return m_id;
}

Fiber::STATE Fiber::getState() 
{
    return m_state;
}

bool Fiber::finish() const noexcept
{
    return (m_state == TERM || m_state == EXCEPTION);
}

Fiber::_ptr Fiber::getThis()
{
    if(FiberInfo::t_fiber != nullptr) {
        return FiberInfo::t_fiber->shared_from_this();
    }
    //为空说明不存在master fiber，则初始化master fiber
    FiberInfo::t_master_fiber.reset(new Fiber());
    return FiberInfo::t_master_fiber->shared_from_this();
}

void Fiber::setThis(Fiber* fiber)   //设置正在执行的协程
{
    FiberInfo::t_fiber = fiber;
}

void Fiber::yieldToReady()
{
    Fiber::_ptr current_fiber = getThis();
    current_fiber->m_state = READY;
    current_fiber->back();  //回到主协程            ?????
}

void Fiber::yieldToHold()
{
    Fiber::_ptr current_fiber = getThis();
    current_fiber->m_state = HOLD;
    current_fiber->swapOut();
}


uint64_t Fiber::getTotalFiberCount()
{
    return FiberInfo::s_fiber_count;
}

uint64_t Fiber::getThisFiberId()
{
    if(FiberInfo::t_fiber != nullptr){
        return FiberInfo::t_fiber->getId();
    }
    return 0;
}

void Fiber::callerMainFunc()
{

}

void Fiber::mainFunc()
{
    Fiber::_ptr current_fiber = getThis();
    try
    {
        current_fiber->m_callback();
        current_fiber->m_callback = nullptr;
        current_fiber->m_state = TERM;        
    }
    catch(Exception& e)
    {
        current_fiber->m_state = EXCEPTION;
        LOG_FORMAT_ERROR(g_logger, "Fiber exception: %s, call stack:\n%s", e.what(), e.stackTrace());
    }
    catch(std::exception& e)
    {
        current_fiber->m_state = EXCEPTION;
        LOG_FORMAT_ERROR(g_logger, "Fiber exception: %s", e.what());
    }
    catch(...)
    {
        current_fiber->m_state = EXCEPTION;
        LOG_ERROR(g_logger, "Fiber exception");
    }
    //执行结束后，切回主协程
    Fiber* current_fiber_ptr = current_fiber.get();
    current_fiber.reset();      //释放shared_ptr的所有权
    if(Scheduler::getThis() &&
        Scheduler::getThis()->m_root_thread_id == GetThreadID() &&
        Scheduler::getThis()->m_root_fiber.get() != current_fiber_ptr)
    {
        current_fiber_ptr->swapOut();
    }
    else{
        current_fiber_ptr->back();
    }

}
}