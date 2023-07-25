#pragma once

#include <exception>
#include <string>
#include <cstring>
#include <cerrno>


#define THROW_EXCEPTION_WITH_ERRNO  \
    do                              \
    {                               \
        throw Exception(std::string(strerror(errno)));  \ 
    } while (0)
    

namespace hxk
{
class Exception : public std::exception
{
public:
    explicit Exception(std::string what);
    ~Exception() noexcept override = default;

    const char* what() const noexcept override; //获取异常信息

    const char* stackTrace() const noexcept;    //获取函数调用栈
protected:
    std::string m_message;
    std::string m_stack;
};


class SystemException : public Exception
{
public:
    explicit SystemException(std::string what = "");
};
}