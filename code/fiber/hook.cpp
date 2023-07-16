#include "hook.h"

namespace hxk
{
static thread_local bool t_hook_enabled = false;


bool isHookEnabled()
{
    return t_hook_enabled;
}

void setHookEnable(bool flag)
{
    t_hook_enabled = flag;
}

} // namespace hxk
