#pragma once

#include <memory>
#include <functional>
#include <vector>

#include "noncopyable.h"
#include "fiber.h"
#include "lock.h"
#include "thread.h"
#include "hook.h"

namespace hxk
{
struct Task
{
    typedef std::shared_ptr<Task> _ptr;
    typedef std::unique_ptr<Task> _uptr;
    typedef std::function<void()> TaskFunc;

    Fiber::_ptr m_fiber;
    TaskFunc m_callback;
    long m_thread_id; // 任务要绑定执行线程的id

    Task() : m_thread_id(-1) {}
    Task(const Task &lhs) = default;
    Task(Fiber::_ptr f, long id) : m_fiber(std::move(f)), m_thread_id(id) {}
    Task(const TaskFunc &cb, long id) : m_callback(cb), m_thread_id(id) {}
    Task(TaskFunc &&cb, long id) : m_callback(std::move(cb)), m_thread_id(id) {}
    Task &operator=(const Task &lhs) = default;

    void reset()
    {
        m_fiber = nullptr;
        m_callback = nullptr;
        m_thread_id = -1;
    }
};

class Scheduler : public noncopyable
{
private:
   

public:
    friend class Fiber;
    typedef std::shared_ptr<Scheduler> _ptr;
    typedef std::unique_ptr<Scheduler> _uptr;

public:
    /**
     * @Author: hxk
     * @brief: 构造函数
     * @param {size_t} thread_size  线程池线程数量
     * @param {bool} use_caller     是否将Scheduler实例化的线程作为master fiber
     * @param {string} name         调度器的名称
     * @return {*}
     */
    explicit Scheduler(size_t thread_size, bool use_caller = true, std::string name="");
    virtual ~Scheduler();

    void start();           //启动协程调度器
    void stop();            //停止协程调度器
    bool hasFreeThread();
    virtual bool isStop();

public:
    static Scheduler* getThis();    //获取当前协程调度器
    static Fiber* getMainFiber();   //获取当前协程调度器的调度工作协程


protected:
    void run();
    virtual void tickle();
    virtual bool onStop();  //调度器停止时的回调函数，返回调度器当前是否处于停止工作的状态
    virtual void onFree();  //调度器空闲时的回调函数

public:
    
    /**
     * @Author: hxk
     * @brief: 添加任务
     * @param {Executable&&} exec 模板类型必须是std::unique_ptr<Fiber> 或 std::function
     * @param {long} thread_id  任务要绑定执行线程的id
     * @param {bool} instant    是否优先调度
     * @return {*}
     */
    template<typename Executable>
    void schedule(Executable&& exec, long thread_id = -1, bool instant = false)
    {
        bool need_tickle = false;
        {
            ScopedLock lock(&m_mutex);
            need_tickle = scheduleNonBlock(std::forward<Executable>(exec), thread_id);
        }
        if(need_tickle) {
            tickle();
        }
    }

    template<typename InputIterator>
    void schedule(InputIterator begin, InputIterator end)
    {
        bool need_tickle = false;
        {
            ScopedLock lock(&m_mutex);
            while(begin != end) {
                need_tickle = scheduleNonBlock(*begin) || need_tickle;
                ++begin;
            }
        }
        if(need_tickle)
        {
            tickle();
        }
    }

private:
    /**
     * @Author: hxk
     * @brief: 添加任务
     * @param {Executable&&} exec 模板类为std::unique_ptr<Fiber> 或 std::function
     * @param {long} thread_id  任务要绑定的线程id
     * @param {bool} instant    任务是否要优先调度
     * @return {*}  是否是空闲状态下的第一个新任务
     */
    template<typename Executable>
    bool scheduleNonBlock(Executable&& exec, long thread_id = -1, bool instant = false)
    {
        bool need_tickle = m_task_list.empty();
        auto task = std::make_unique<Task>(std::forward<Executable>(exec), thread_id);
        if(task->m_fiber || task->m_callback) {
            if(instant) {
                m_task_list.push_front(std::move(task));
            }
            else {
                m_task_list.push_back(std::move(task));
            }
        }
        return need_tickle;
    }

protected:
    const std::string m_name;   //调度器名称
    long m_root_thread_id;  //主线程id
    std::vector<long> m_thread_id_list; //线程id列表
    size_t m_thread_count;  //线程数量
    std::atomic_uint64_t m_active_thread_count; //活跃线程数量
    std::atomic_uint64_t m_free_thread_count;   //空闲线程数量
    bool m_stopped;         //执行停止状态
    bool m_auto_stopped;    //是否自动停止

private:
    mutable Mutex m_mutex;
    Fiber::_ptr m_root_fiber;   //负责调度的协程，仅在类实例化参数中use_call为true有效
    std::vector<Thread::_ptr> m_thread_list;    //线程列表
    std::list<Task::_ptr>   m_task_list;    //任务集合

};
}