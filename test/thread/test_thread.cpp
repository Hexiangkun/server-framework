#include "thread.h"
#include "log.h"
#include "lock.h"

auto logger  = GET_ROOT_LOGGER();

static uint64_t count = 0;
hxk::RWLock m_rwlock;
hxk::Mutex m_mutex;


void fn_1()
{
    LOG_FORMAT_DEBUG(logger, "当前线程id=%ld/%d, 当前线程名=%s", hxk::GetThreadID(),
                hxk::Thread::getThisThreadID(), hxk::Thread::getThisThreadName().c_str());
}

void fn_2()
{
    hxk::WriteScopedLock wsl(&m_rwlock);
    for(int i=0;i<1000000;i++){
        count++;
    }
}

void Test_createThreadJoin()
{
    LOG_DEBUG(logger, "create test_thread");
    std::vector<hxk::Thread::_ptr> thread_vec;

    for(size_t i=0;i<5;i++){
        thread_vec.push_back(std::make_shared<hxk::Thread>(&fn_1, "thread_"+std::to_string(i)));
    }

    LOG_DEBUG(logger, "join()");
    for(auto thread:thread_vec){
        thread->join();
    }
}

void Test_createThreadDetach()
{
    LOG_DEBUG(logger, "detach()");
    for(size_t i=0;i<5;i++){
        std::make_unique<hxk::Thread>(&fn_1, "detach_thread_"+std::to_string(i));
    }
}

void Test_RWLock()
{
    LOG_DEBUG(logger,"RWLock Test");
    std::vector<hxk::Thread::_uptr> thread_vec;
    for(int i=0;i<10;i++) {
        thread_vec.push_back(std::make_unique<hxk::Thread>(&fn_2, "thread_"+std::to_string(i)));
    }

    for(auto& thread:thread_vec){
        thread->join();
    }

    LOG_FORMAT_DEBUG(logger, "count = %ld", count);
}

int main()
{
    //Test_createThreadJoin();
    //Test_createThreadDetach();
    Test_RWLock();
    return 0;
}