#ifndef NONCOPYABLE_H
#define NONCOPYABLE_H

/*
禁止类的拷贝
*/
namespace hxk
{

class noncopyable
{
protected:
    noncopyable(/* args */) = default;
    ~noncopyable() = default;

public:
    noncopyable(const noncopyable&) = delete;
    void operator=(const noncopyable&) = delete;
};
}

#endif

