#pragma once

#include "scheduler.h"
#include "fiber.h"
#include "lock.h"
#include "timer.h"
#include "log.h"

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


class IOManager final : public Scheduler, public TimerManager
{
public:
    typedef std::shared_ptr<IOManager> _ptr;

public:
    explicit IOManager(size_t thread_size, bool use_caller, std::string name = "");
    ~IOManager();

    /**
     * @Author: hxk
     * @brief: 给指定的fd增加监听事件，当cb为nullptr时，将当前上下文转换为协程，并作为事件回调使用
     * @param {int} fd
     * @param {FDEventType} event_type
     * @param {function<void()>} cb
     * @return {*}
     */
    int addEventListener(int fd, FDEventType event_type, std::function<void()> cb = nullptr);
    int removeEventListener(int fd, FDEventType event_type);
    bool cancelEventListener(int fd, FDEventType event_type);   //立即触发fd指定的事件，然后移除该事件
    bool cancelAll(int fd); //触发fd所有事件，然后移除所有事件

public:
    static IOManager* getThis();


protected:
    void tickle() override;
    void onFree() override;
    bool isStop() override;
    bool isStop(uint64_t& timeout);
    void contentListResize(size_t size);
    void onTimerInsertedAtFirst() override;

private:
    RWLock m_lock;
    int m_epoll_fd = 0;
    int m_tickle_fds[2] = {0};  //主线程给子线程发消息用的管道
    std::atomic_size_t m_pending_event_count = 0;   //等待执行的事件的数量
    std::vector<std::unique_ptr<FDContent>> m_fd_content_list;
};
}