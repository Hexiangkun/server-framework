#include "exception.h"

namespace hxk
{

Exception::Exception(std::string what):m_message(what), m_stack(BackTraceToString(200))
{

}

const char* Exception::what() const noexcept
{
    return m_message.c_str();
}

const char* Exception::stackTrace() const noexcept
{
    return m_stack.c_str();
}

SystemException::SystemException(std::string what)
            :Exception(what + " : " + std::string(::strerror(errno)))
{

}

}