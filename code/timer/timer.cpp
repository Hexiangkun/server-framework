#include "timer.h"

namespace hxk
{

bool Timer::Compare::operator()(const Timer::_ptr& lhs, const Timer::_ptr& rhs) const
{
    if(!lhs && !rhs) {
        return false;
    }
    if(lhs) {
        return true;
    }
    if(rhs) {
        return false;
    }
    //按绝对时间戳排序
    if(lhs->m_next < rhs->m_next) {
        return true;
    }
    else if(lhs->m_next > rhs->m_next) {
        return false;
    }
    return lhs.get() < rhs.get();       //时间戳相等，按指针排序
}

Timer::Timer(uint64_t next) :m_cyclic(false), 
                            m_ms(0),
                            m_next(next),
                            m_manager(nullptr)
{

}
Timer::Timer(uint64_t ms, std::function<void()> fn, bool cyclic, TimerManager* manager)
            : m_cyclic(cyclic),
            m_ms(ms),
            m_cb(fn),
            m_manager(manager)
{
    m_next = GetCurrentMS() + m_ms;
}


bool Timer::cancel()
{
    WriteScopedLock lock(&m_manager->m_lock);
    if(m_cb) {
        m_cb = nullptr;
        auto it = m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::reset(uint64_t ms, bool from_now)
{
    if(ms == m_ms && !from_now) {
        return true;
    }
    if(!m_cb) {
        return false;
    }
    WriteScopedLock lock(&m_manager->m_lock);
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start = 0;
    if(from_now) {
        start = GetCurrentMS();
    }
    else {
        start = m_next - m_ms;
    }
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

bool Timer::refresh()
{
    WriteScopedLock lock(&m_manager->m_lock);
    if(!m_cb) {
        return false;
    }
    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end()) {
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next = GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}

}

namespace hxk 
{
static void onTimer(std::weak_ptr<void> weak_cond, std::function<void()> fn)
{
    auto tmp = weak_cond.lock();
    if(tmp) {
        fn();
    }
}
}

namespace hxk
{

TimerManager::TimerManager():m_previous_time(0)
{
    m_previous_time = GetCurrentMS();
}

TimerManager::~TimerManager()
{

}

Timer::_ptr TimerManager::addTimer(uint64_t ms, std::function<void()> fn, bool cyclic)
{
    Timer::_ptr timer(new Timer(ms, fn, cyclic, this));
    WriteScopedLock lock(&m_lock);
    addTimer(timer, lock);
    return timer;
}

void TimerManager::addTimer(Timer::_ptr timer, WriteScopedLock& lock)
{
    auto it = m_timers.insert(timer).first;
    bool at_front = (it == m_timers.begin());
    lock.unlock();
    if(at_front) {
        onTimerInsertedAtFirst();
    }
}

Timer::_ptr TimerManager::addConditionTimer(uint64_t ms, std::function<void()> fn,
                                            std::weak_ptr<void> weak_cond, bool cyclic)
{
    return addTimer(ms, std::bind(&onTimer,weak_cond, fn), cyclic);
}

uint64_t TimerManager::getNextTimer()
{
    ReadScopedLock lock(&m_lock);
    if(m_timers.empty()) {
        return ~0ull;   //没有定时器
    }
    const Timer::_ptr& next = *m_timers.begin();
    uint64_t now_ms = GetCurrentMS();
    if(now_ms >= next->m_next) {
        return 0;   //等待超时
    }
    else {
        return next->m_next - now_ms;   //返回剩余等待时间
    }
}

void TimerManager::listExpiredCallback(std::vector<std::function<void()>>& fns)
{
    uint64_t now_ms = GetCurrentMS();
    std::vector<Timer::_ptr> expired;
    {
        ReadScopedLock lock(&m_lock);
        if(m_timers.empty()) {
            return ;
        }
    }
    WriteScopedLock lock(&m_lock);
    bool rollover = detectClockRollover(now_ms);    //检查系统时间是否被修改
    if(!rollover && (*m_timers.begin())->m_next > now_ms) {
        return;
    }
    Timer::_ptr now_timer(new Timer(now_ms));
    //获取第一个m_next大于或等于now_timer->m_next的定时器的迭代器
    //如果系统时间被修改，则认为所有定时器均超时
    auto it  = rollover ? m_timers.end() : m_timers.lower_bound(now_timer);
    while(it != m_timers.end() && (*it)->m_next == now_timer->m_next) {
        ++it;
    }
    expired.insert(expired.begin(),m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    fns.reserve(expired.size());
    for(auto& timer : expired) {
        fns.emplace_back(timer->m_cb);
        if(timer->m_cyclic) {
            timer->m_next = now_ms + timer->m_ms;
            m_timers.insert(timer);
        }
        else {
            timer->m_cb = nullptr;
        }
    }
}

bool TimerManager::hasTimer()
{
    ReadScopedLock lock(&m_lock);
    return !m_timers.empty();
}

bool TimerManager::detectClockRollover(uint64_t now_ms)
{
    bool rollover = false;
    if(now_ms < m_previous_time && now_ms < (m_previous_time - 60*60*1000)) {
        rollover = true;
    }
    m_previous_time = now_ms;
    return rollover;
}

} // namespace hxk
