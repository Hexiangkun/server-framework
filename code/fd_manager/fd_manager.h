#pragma once

#include <memory>
#include "io_manager.h"
#include "lock.h"
#include "singleInstance.h"

namespace hxk
{

class FileDescriptor : public std::enable_shared_from_this<FileDescriptor>
{
public:
    using _ptr = std::shared_ptr<FileDescriptor>;

    FileDescriptor(int fd);
    ~FileDescriptor();

    bool init();
    bool close();

    bool isInit() const;
    bool isSocket() const;
    bool isClosed() const;

    void setUserNonBlock(bool v);
    bool getUserNonBlock() const;

    void setSystemNonBlock(bool v);
    bool getSystemNonBlock() const;

    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);

private:
    bool m_is_init;
    bool m_is_socket;
    bool m_system_non_block;
    bool m_user_non_block;
    bool m_is_closed;

    int m_fd;

    uint64_t m_recv_timeout;
    uint64_t m_send_timeout;

    hxk::IOManager* m_iomanager;
};


class FileDescriptorManagerImpl
{
public:
    FileDescriptorManagerImpl();

    FileDescriptor::_ptr get(int fd, bool auto_create = false);

    void remove(int fd);

private:
    RWLock m_lock;
    std::vector<FileDescriptor::_ptr> m_data;
};

using FileDescriptorManager = SingleInstancePtr<FileDescriptorManagerImpl>;

}