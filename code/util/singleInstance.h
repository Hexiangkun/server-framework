#pragma once

#include <memory>


/*
    *单例模式
        *构造函数和析构函数为私有类型，目的是禁止外部构造和析构。
        *拷贝构造函数和赋值构造函数是私有类型，目的是禁止外部拷贝和赋值，确保实例的唯一性。
        *类中有一个获取实例的静态方法，可以全局访问。
*/
namespace hxk{

template<typename T>
class SingleInstance final  //该类禁止被继承
{
public:
    static T* GetInstance()
    {
        static T ins;
        return &ins;
    }
private:
    SingleInstance() = default;
    ~SingleInstance() = default;

    SingleInstance(const SingleInstance& );
    const SingleInstance operator=(const SingleInstance&);
};

template<typename T>
class SingleInstancePtr final
{
public:
    static std::shared_ptr<T> GetInstance()
    {
        static auto ins = std::make_shared<T>();
        return ins;
    }

private:
    SingleInstancePtr()=default;
    ~SingleInstancePtr()=default;
    SingleInstancePtr(const SingleInstancePtr&);
    const SingleInstancePtr operator=(const SingleInstancePtr&);
};

}