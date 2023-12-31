#pragma once

#include <memory>
#include <functional>
#include <ucontext.h>
#include <atomic>

#include "noncopyable.h"
#include "config.h"



namespace hxk
{
class Scheduler;

/**
 * @Author: hxk
 * @brief: 协程
 * @return {*}
 */
class Fiber : public std::enable_shared_from_this<Fiber>, public noncopyable
{
    friend class Scheduler;

public:
    using _ptr = std::shared_ptr<Fiber>;
    using _uptr = std::unique_ptr<Fiber>;
    using FiberFunc = std::function<void()> ;

    enum STATE{
        INIT,       //初始化
        READY,      //就绪
        HOLD,       //挂起
        EXEC,       //执行
        TERM,       //结束
        EXCEPTION   //异常
    };

public:
    /**
     * @Author: hxk
     * @brief: 创建新的协程
     * @param {FiberFunc} callback 协程执行函数
     * @param {size_t} stack_size   协程栈空间大小
     * @param {bool}   use_caller   是否在main fiber上调度
     * @return {*}
     */
    explicit Fiber(FiberFunc callback, size_t stack_size = 0,bool user_caller = false);
    ~Fiber();

    /**
     * @Author: hxk
     * @brief: 更换协程执行函数
     * @pre   m_state 为ITIT TERM EXCEPTION
     * @post  m_state 为 INIT
     * @param {FiberFunc} callback
     * @return {*}
     */
    void reset(FiberFunc callback); 

    /**
     * @Author: hxk
     * @brief: 将当前协程切换到运行状态
     * @pre  m_state != EXEC
     * @post m_state = EXEC
     * @return {*}
     */
    void swapIn();      //换入协程，通常由master fiber调用

    /**
     * @Author: hxk
     * @brief: 将当前协程切换到后台
     * @return {*}
     */
    void swapOut();     //挂起协程，通常由master fiber调用

    /**
     * @Author: hxk
     * @brief: 
     * @pre 执行的为当前线程的主协程
     * @return {*}
     */
    void call();        //换入协程，将调用时的上下文挂起，保存到线程局部变量中
    /**
     * @Author: hxk
     * @brief: 
     * @pre 执行的为该协程
     * @post 返回到线程的主协程
     * @return {*}
     */
    void back();        //挂起协程，保存当前上下文到协程对象中，从线程局部变量恢复上下文

    void swapIn(Fiber::_ptr fiber_ptr);     //换入协程，通常由调度器调用
    void swapOut(Fiber::_ptr fiber_ptr);    //挂起协程，通常由调度器调用

    uint64_t getId();   //获取协程id
    STATE getState();   //获取协程状态

    bool finish() const noexcept;   //判断协程是否执行结束

private:    
    Fiber();    //用于创建master fiber

public:
    static Fiber::_ptr getThis();   //获取当前正在执行的fiber指针
                                    //如果不存在，在当前线程上创建master fiber
    
    static void setThis(Fiber* fiber);  //设置当前线程运行的协程fiber

    static void yieldToReady();    //挂起当前协程，转换为READY状态，等待下一次调用
    static void yieldToHold();  //挂起当前协程，转为HOLD状态

    static uint64_t getTotalFiberCount();   //获取存在协程数量

    static uint64_t getThisFiberId();       //获取当前协程的id

    /**
     * @Author: hxk
     * @brief: 协程执行函数
     * @post    执行完返回到线程主协程
     * @return {*}
     */
    static void mainFunc();     //协程入口函数

    /**
     * @Author: hxk
     * @brief: 协程执行函数
     * @post    执行完返回到线程调度协程
     * @return {*}
     */
    static void callerMainFunc();

private:
    uint64_t m_id;          //协程id

    uint64_t m_stack_size;  //协程栈大小

    STATE m_state;          //协程状态

    ucontext_t m_ucontext;   //协程上下文

    void* m_stack;  //协程栈空间指针

    FiberFunc m_callback;   //执行函数  
};

/**
 * @Author: hxk
 * @brief: 对malloc/free简单封装
 * @return {*}
 */
class MallocStackAllocator
{
public:
    static void* Alloc(uint64_t size)
    {
        return malloc(size);
    }

    static void Delalloc(void *ptr, uint64_t size)
    {
        free(ptr);
    }
};


/// @brief 协程栈空间分配器
using stackAllocator = MallocStackAllocator;

namespace FiberInfo
{
static std::atomic_uint64_t s_fiber_id = {0};   //最后一个协程id

static std::atomic_int64_t s_fiber_count = {0}; //协程数量

static thread_local Fiber* t_fiber = nullptr;   //当前线程正在执行的协程

static thread_local Fiber::_ptr t_master_fiber; //当前线程的主协程

static ConfigVar<uint64_t>::_ptr g_fiber_stack_size = Config::lookUp<uint64_t>("fiber.stack_size", 1024*1024, "fiber stack size");
}

}