#include "fiber.h"

namespace hxk
{
static Logger::_ptr g_logger = GET_LOGGER("system");

Fiber::Fiber(): m_id(0), m_stack_size(0),m_state(EXEC), m_context(), m_stack(nullptr),m_callback()
{
    setThis(this);

    //
    if(getcontext(&m_context)) {
        throw std::exception();
    }

    ++FiberInfo::s_fiber_count; //存在协程数量增加

}

void Fiber::setThis(Fiber* fiber)
{
    FiberInfo::t_fiber = fiber;
}

}