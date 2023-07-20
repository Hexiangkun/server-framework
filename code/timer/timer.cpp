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