#pragma once

#include "scheduler.h"
#include "fiber.h"
#include "lock.h"

namespace hxk
{
enum FDEventType
{
    NONE = 0x0,
    READ = 0x1,
    WRITE = 0x4
};

struct EventHandler
{
    Scheduler* m_scheduler;         //指定处理该事件的调度器
    Fiber::_ptr m_fiber;            //要跑的协程
    Fiber::FiberFunc m_callback;    //要跑的函数，协程和函数存在一个即可
};

struct FDContent
{
    hxk::Mutex m_mutex;
    EventHandler m_read_handler;
    EventHandler m_write_handler;
    int m_fd;
    FDEventType m_event_type = FDEventType::NONE;

    EventHandler& getEventHandler(FDEventType type);    //获取指定事件的处理器
    void resetEventHandler(EventHandler& handler);      //清除指定的事件处理器
    void triggerEvent(FDEventType type);                //触发事件，然后删除
};


class IOManager final : public Scheduler
{

};
}