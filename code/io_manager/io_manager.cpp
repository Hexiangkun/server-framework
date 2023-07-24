#include "io_manager.h"
#include "exception.h"

#include <sys/epoll.h>

namespace hxk
{

static Logger::_ptr g_logger = GET_LOGGER("system");

void FDContent::resetEventHandler(EventHandler& handler)
{
    handler.m_fiber.reset();
    handler.m_callback = nullptr;
    handler.m_scheduler = nullptr;
}


void FDContent::triggerEvent(FDEventType type)
{
    assert(m_event_type & type);
    m_event_type = static_cast<FDEventType>(m_event_type & ~type);
    auto &handler = getEventHandler(type);
    assert(handler.m_scheduler);
    if(handler.m_fiber) {
        handler.m_scheduler->schedule(std::move(handler.m_fiber));
    }
    else if(handler.m_callback) {
        handler.m_scheduler->schedule(std::move(handler.m_callback));
    }
    handler.m_scheduler = nullptr;
}

EventHandler& FDContent::getEventHandler(FDEventType type)
{
    switch (type)
    {
    case FDEventType::READ:
        return m_read_handler;
    case FDEventType::WRITE:
        return m_write_handler;
    default:
        assert(0);
    }
}

IOManager::IOManager(size_t thread_size, bool use_caller, std::string name):Scheduler(thread_size, use_caller, name)
{
    LOG_DEBUG(g_logger, "调用IOManager::IOManager()");
    m_epoll_fd = epoll_create(0xffff);
    if(m_epoll_fd == -1) {
        THROW_EXCEPTION_WITH_ERRNO;
    }
    if(pipe(m_tickle_fds) == -1) {  //创建管道
        THROW_EXCEPTION_WITH_ERRNO;
    }
    epoll_event event;
    event.data.fd = m_tickle_fds[0];
    event.events = EPOLLIN | EPOLLET;   //监听读事件|开启边缘触发

    if(fcntl(m_tickle_fds[0], F_SETFL, O_NONBLOCK)){//将管道读端设置为非阻塞模式
        THROW_EXCEPTION_WITH_ERRNO;
    }
    if(epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_tickle_fds[0], &event) == -1) {
        THROW_EXCEPTION_WITH_ERRNO;
    }
    contentListResize(64);
    start();    //启动调度器
}

IOManager::~IOManager()
{
    stop(); 

    close(m_epoll_fd);
    close(m_tickle_fds[0]);
    close(m_tickle_fds[1]);
}

IOManager* IOManager::getThis()
{
    return dynamic_cast<IOManager*>(Scheduler::getThis());
}

void IOManager::contentListResize(size_t size)
{
    m_fd_content_list.resize(size);
    for(size_t i = 0; i<m_fd_content_list.size(); i++) {
        if(!m_fd_content_list[i]) {
            m_fd_content_list[i] = std::make_unique<FDContent>();
            m_fd_content_list[i]->m_fd = i;
        }
    }
}

int IOManager::addEventListener(int fd, FDEventType event_type, std::function<void()> cb)
{
    FDContent* fd_ctx = nullptr;
    ReadScopedLock lock(&m_lock);
    //取对应fd的文件描述符对象
    if(m_fd_content_list.size() > static_cast<size_t>(fd)) {
        fd_ctx = m_fd_content_list[fd].get();
        lock.unlock();
    }
    else {
        lock.unlock();  
        WriteScopedLock lock2(&m_lock);     //扩容，取对应的对象
        contentListResize(m_fd_content_list.size()*2);
        fd_ctx = m_fd_content_list[fd].get();
    }
    ScopedLock lock3(&fd_ctx->m_mutex);
    if(fd_ctx->m_event_type & event_type) { //检查要监听的事件是否存在
        LOG_FORMAT_ERROR(g_logger, "IOManager::addEventListener 重复添加相同的事件，fd = %d, event_type = %d", fd, event_type);
        assert(!(fd_ctx->m_event_type & event_type));
    }
    int op = fd_ctx->m_event_type == FDEventType::NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->m_event_type | event_type;
    epevent.data.ptr = fd_ctx;

    if(epoll_ctl(m_epoll_fd, op, fd, &epevent) == -1) {
        LOG_FORMAT_ERROR(g_logger, "epoll_ctl 调用失败，fd = %d", m_epoll_fd);
        return -1;
    }

    ++m_pending_event_count;

    fd_ctx->m_event_type = static_cast<FDEventType>(fd_ctx->m_event_type | event_type);
    EventHandler& event_handler = fd_ctx->getEventHandler(event_type);
    assert(event_handler.m_scheduler == nullptr && !event_handler.m_fiber && !event_handler.m_callback);

    event_handler.m_scheduler = this;
    if(cb) {
        event_handler.m_callback.swap(cb);
    }
    else{
        //当callback时nullptr时，将当前上下文转换为协程，并作为时间回调使用
        event_handler.m_fiber = Fiber::getThis();
    }
    return 0;
}

int IOManager::removeEventListener(int fd, FDEventType event_type)
{
    FDContent* fd_ctx = nullptr;
    {
        ReadScopedLock lock(&m_lock);
        if(m_fd_content_list.size() <= static_cast<size_t>(fd)) {
            return false;
        }
        fd_ctx = m_fd_content_list[fd].get();
    }
    ScopedLock lock2(&(fd_ctx->m_mutex));
    if(!(fd_ctx->m_event_type & event_type)) {  //要移除的事件不存在
        return false;       
    }
    //从epoll上移除该事件的监听
    auto new_event_type = static_cast<FDEventType>(fd_ctx->m_event_type & ~event_type);
    //如果new_event为0，从epoll中移除对该fd的监听，否则修改监听事件
    int op = new_event_type == FDEventType::NONE ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
    epoll_event epevent;
    epevent.events = EPOLLET | new_event_type;
    epevent.data.ptr = fd_ctx;
    if(epoll_ctl(m_epoll_fd, op, fd, &epevent) == -1) {
        LOG_FORMAT_ERROR(g_logger, "removeEventListener epoll_ctl error, epfd = %d", m_epoll_fd);
        THROW_EXCEPTION_WITH_ERRNO;
    }
    fd_ctx->m_event_type = new_event_type;
    auto& event_handler = fd_ctx->getEventHandler(event_type);
    fd_ctx->resetEventHandler(event_handler);
    --m_pending_event_count;
    return true;
}

bool IOManager::cancelEventListener(int fd, FDEventType event_type)
{
    FDContent* fd_ctx = nullptr;
    {
        ReadScopedLock lock(&m_lock);
        if(m_fd_content_list.size() <=  static_cast<size_t>(fd)) {
            return false;
        }
        fd_ctx = m_fd_content_list[fd].get();
    }
    ScopedLock lock2(&(fd_ctx->m_mutex));
    if(!(fd_ctx->m_event_type & event_type)) {
        return false;
    }

    auto new_event_type = static_cast<FDEventType>(fd_ctx->m_event_type & ~event_type);
    int op = new_event_type == FDEventType::NONE ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
    epoll_event epevent;
    epevent.events = EPOLLET | new_event_type;
    epevent.data.ptr = fd_ctx;
    if(epoll_ctl(m_epoll_fd, op, fd, &epevent) == -1) {
        LOG_FORMAT_ERROR(g_logger, "cancelEventListener epoll_ctl error, epfd = %d", m_epoll_fd);
        THROW_EXCEPTION_WITH_ERRNO;
    }
    fd_ctx->m_event_type = new_event_type;
    fd_ctx->triggerEvent(event_type);
    --m_pending_event_count;
    return true;
}

bool IOManager::cancelAll(int fd)
{
    FDContent* fd_ctx = nullptr;
    {
        ReadScopedLock lock(&m_lock);
        if(m_fd_content_list.size() <= static_cast<size_t>(fd)) {
            return false;
        }
        fd_ctx = m_fd_content_list[fd].get();
    }
    ScopedLock lock2(&(fd_ctx->m_mutex));
    if(!(fd_ctx->m_event_type)) {
        return false;
    }
    if(epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
        LOG_FORMAT_ERROR(g_logger, "cancelAll epoll_ctl error, epfd = %d", m_epoll_fd);
        THROW_EXCEPTION_WITH_ERRNO;
    }
    if(fd_ctx->m_event_type & FDEventType::READ) {
        fd_ctx->triggerEvent(FDEventType::READ);
        --m_pending_event_count;
    }
    if(fd_ctx->m_event_type & FDEventType::WRITE) {
        fd_ctx->triggerEvent(FDEventType::WRITE);
        --m_pending_event_count;
    }
    fd_ctx->m_event_type = FDEventType::NONE;
    return true;
}

void IOManager::tickle()
{
    if(hasFreeThread()) {
        return;
    }
    if(write(m_tickle_fds[1], "T", 1) == -1) {
        throw SystemException("send message to child thread error");
    }
}

bool IOManager::isStop()
{
    uint64_t timeout;
    return isStop(timeout);
}

bool IOManager::isStop(uint64_t& timeout)
{
    timeout = getNextTimer();
    return timeout == ~0ull && m_pending_event_count == 0 && Scheduler::isStop();
}

void IOManager::onFree()
{
    LOG_DEBUG(g_logger, "调用 IOManager::onFree()");
    auto event_list = std::make_unique<epoll_event[]>(64);

    while(true)
    {
        uint64_t next_timeout = 0;
        if(isStop(next_timeout)) {
            if(next_timeout == ~0ull) {
                LOG_FORMAT_DEBUG(g_logger, "调度器 %s 已经停止执行", m_name.c_str());
                break;
            }
        }
        int result = 0;
        while(true) 
        {
            static const int MAX_TIMEOUT = 1000;
            if(next_timeout != ~0ull) {
                next_timeout = static_cast<int>(next_timeout) > MAX_TIMEOUT ? MAX_TIMEOUT:next_timeout; 
            }
            else {
                next_timeout = MAX_TIMEOUT;
            }
            result = epoll_wait(m_epoll_fd, event_list.get(), 64, static_cast<int>(next_timeout));
            if(result < 0) {

            }
            if(result >= 0) {
                break;
            }
        }

        std::vector<std::function<void()>> fns;
        listExpiredCallback(fns);
        if(!fns.empty()) {
            schedule(fns.begin(), fns.end());
        }

        for(int i = 0; i < result; i++) {
            epoll_event& ev = event_list[i];
            if(event.data.fd == m_tickle_fds[0]) {
                char dummy;
                while(true) {
                    int status = read(ev.data.fd, &dummy, 1);
                    if(status == 0 || status == -1) {
                        break;
                    }
                }
                continue;
            }

            auto fd_ctx = static_cast<FDContent*>(ev.data.ptr);
            ScopedLock lock(&(fd_ctx->m_mutex));
            if(ev.events & (EPOLLERR | EPOLLHUP)) {
                ev.events |= EPOLLIN | EPOLLOUT;
            }
            uint32_t real_event = FDEventType::NONE;
            if(ev.events & EPOLLIN) {
                real_event |= FDEventType::READ;
            }
            if(ev.events & EPOLLOUT) {
                real_event != FDEventType::WRITE;
            }

            if((fd_ctx->m_event_type & real_event) == FDEventType::NONE) {
                continue;
            }

            uint32_t left_events = (fd_ctx->m_event_type & ~real_event);
            int op = left_events == 0 ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
            ev.events = EPOLLET | left_events;
            if(epoll_ctl(m_epoll_fd, op, fd_ctx->m_fd, &ev) == -1) {
                LOG_FORMAT_ERROR(g_logger, "epoll_ctl(%d, %d, %d, %ul) :  errno = %d, %s",
                    m_epoll_fd, op, fd_ctx->m_fd, ev.events, errno, strerror(errno));
            }
            if(real_event & FDEventType::READ) {
                fd_ctx->triggerEvent(FDEventType::READ);
                --m_pending_event_count;
            }
            if(real_event & FDEventType::WRITE) {
                fd_ctx->triggerEvent(FDEventType::WRITE);
                --m_pending_event_count;
            }
        }
        Fiber::_ptr current_fiber = Fiber::getThis();
        auto raw_ptr = current_fiber.get();
        current_fiber.reset();
        raw_ptr->swapOut();
    }
}

void IOManager::onTimerInsertedAtFirst()
{
    tickle();
}

}