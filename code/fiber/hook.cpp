#include "hook.h"

#include "log.h"
#include "config.h"

namespace hxk
{
static Logger::_ptr g_logger  = GET_LOGGER("system");

static thread_local bool t_hook_enabled = false;

static hxk::ConfigVar<int>::_ptr g_tcp_connect_timeout = hxk::Config::lookUp("tcp.connect.timeout", 5000);


bool isHookEnabled()
{
    return t_hook_enabled;
}

void setHookEnable(bool flag)
{
    t_hook_enabled = flag;
}

#define DEAL_FUNC(DO) \
    DO(sleep)           \
    DO(usleep) \
    DO(nanosleep) \
    DO(socket) \
    DO(connect) \
    DO(accept) \
    DO(recv) \
    DO(recvfrom) \
    DO(recvmsg) \
    DO(send) \
    DO(sendto) \
    DO(sendmsg) \
    DO(getsockopt) \
    DO(setsockopt) \
    DO(read) \
    DO(write) \
    DO(close) \
    DO(readv) \
    DO(writev) \
    DO(fcntl) \
    DO(ioctl)

void hook_init()
{
    static bool is_inited = false;
    if(is_inited) {
        return;
    }

    #define TRY_LOAD_HOOK_FUNC(name) name##_f = (name##_func)dlsym(RTLD_NEXT, #name);
        DEAL_FUNC(TRY_LOAD_HOOK_FUNC)
    #undef TRY_LOAD_HOOK_FUNC
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter
{
    _HookIniter()
    {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();
        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_val){
            LOG_FORMAT_INFO(g_logger, "tcp connect timeout change from %d to %d", old_value, new_val);
            s_connect_timeout = new_val;
        });
    }
};

static _HookIniter s_hook_initer;


} // namespace hxk


struct TimerInfo
{
    int cancelled  = 0;
};

template<typename OriginFunc, typename ...Args>
static ssize_t doIO(int fd, OriginFunc func, const char* hook_func_name, uint32_t event, int fd_timeout_type, Args&& ...args)
{
    if(!isHookEnabled()){
        return func(fd, std::forward<Args>(args)...);
    }
}

extern "C"
{
#define DEF_FUNC_NAME(name) name##_func name##_f = nullptr;
    DEAL_FUNC(DEF_FUNC_NAME)
#undef DEF_FUNC_NAME

unsigned int sleep(unsigned int seconds)
{
    if(!hxk::t_hook_enabled) {
        return sleep_f(seconds);
    }
    
}
}