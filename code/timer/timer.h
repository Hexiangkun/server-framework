#pragma once

#include <memory>
#include <functional>
#include <set>

#include "lock.h"
#include "util.h"

namespace hxk
{

class TimerManager;

class Timer : public std::enable_shared_from_this<Timer>
{
friend class TimerManager;

public:
    typedef std::shared_ptr<Timer> _ptr;

    /// @brief 取消定时器
    /// @return 
    bool cancel();

    /// @brief  重新设置事件间隔
    /// @param ms   延迟
    /// @param from_now 是否立即开始倒计时
    /// @return 
    bool reset(uint64_t ms, bool from_now);

    /// @brief 重新计时
    /// @return 
    bool refresh();

    ~Timer() = default;
private:
    /**
     * @Author: hxk
     * @brief: 
     * @param {uint64_t} ms         延迟时间
     * @param {function<void()>} fn 回调函数
     * @param {bool} cyclic         是否重复执行
     * @param {TimerManager*} manager   执行环境
     * @return {*}
     */
    Timer(uint64_t ms, std::function<void()> fn, bool cyclic, TimerManager* manager);

    /**
     * @Author: hxk
     * @brief: 用于创建只有时间信息的定时器，基本用于查找超时的定时器，无其他作用
     * @param {uint64_t} next
     * @return {*}
     */
    Timer(uint64_t next);

private:
    bool m_cyclic ;     //是否重复执行
    uint64_t m_ms;      //执行周期
    uint64_t m_next;    //执行的绝对时间戳
    std::function<void()> m_cb;
    TimerManager* m_manager;

private:
    struct Compare 
    {
        bool operator()(const Timer::_ptr& lhs, const Timer::_ptr& rhs) const;
    };
};


class TimerManager
{
friend class Timer;
public:
    TimerManager();
    virtual ~TimerManager();

    /**
     * @Author: hxk
     * @brief: 新增一个定时器
     * @param {uint64_t} ms 延迟毫秒数
     * @param {function<void()>} fn 回调函数
     * @param {bool} cyclic     是否重复执行
     * @return {*}
     */
    Timer::_ptr addTimer(uint64_t ms, std::function<void()> fn, bool cyclic = false);

    /**
     * @Author: hxk
     * @brief: 新增一个条件定时器。当到达执行时间时，提供的条件变量依旧有效，则执行，否则不执行
     * @param {uint64_t} ms
     * @param {function<void()>} fn
     * @param {weak_ptr<void>} weak_cond 条件变量，利用智能指针是否有效作为判断条件
     * @param {bool} cyclic
     * @return {*}
     */
    Timer::_ptr addConditionTimer(uint64_t ms, std::function<void()> fn, std::weak_ptr<void> weak_cond, bool cyclic = false);


    /**
     * @Author: hxk
     * @brief: 获取下一个定时器的等待时间
     * @return 
     */
    uint64_t getNextTimer();

    /**
     * @Author: hxk
     * @brief: 获取所有等待超时的定时器的回调函数对象，并将定时器从队列中移除
     * @return {*}
     */
    void listExpiredCallback(std::vector<std::function<void()>>& fns);

    /**
     * @Author: hxk
     * @brief: 检查是否有等待执行的定时器
     * @return {*}
     */
    bool hasTimer();

protected:

    /**
     * @Author: hxk
     * @brief: 当创建了延迟时间最短的定时任务时，会调用此函数
     * @return {*}
     */
    virtual void onTimerInsertedAtFirst() = 0;

    /**
     * @Author: hxk
     * @brief: 添加已有的定时器对象
     * @param {_ptr} timer
     * @param {WriteScopedLock&} lock
     * @return {*}
     */
    void addTimer(Timer::_ptr timer, WriteScopedLock& lock);

private:
    bool detectClockRollover(uint64_t now_ms);

private:
    RWLock m_lock;
    std::set<Timer::_ptr, Timer::Compare> m_timers;
    uint64_t m_previous_time = 0;
};
}