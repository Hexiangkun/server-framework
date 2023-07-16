#include "fiber.h"

class Job
{
public:
    Job() {
        std::cout << "构造函数 Job" << std::endl;
    }
    ~Job() {
        std::cout << "析构函数 Job" << std::endl;
    }
};
int fib = 0;
void func()
{
    std::array<Job, 3> list;
    std::cout << "func() start" << std::endl;

    int a=0,b=1;
    while(a < 20) {
        fib = a+b;
        a=b;
        b=fib;
        hxk::Fiber::yieldToReady();
    }

    std::cout << "func() end" <<std::endl;
}

void test(char c) 
{
    for(int i=0;i<10;i++) {
        std::cout << c << std::endl;
        hxk::Fiber::yieldToHold();
    }
}


int main()
{
    hxk::Fiber::getThis();
    {
        auto fiber = std::make_shared<hxk::Fiber>(func);
        std::cout << "换入协程，打印斐波那契数列" << std::endl;
        fiber->call();
        while (fib < 100 && !fiber->finish())
        {
            std::cout << fib << " ";
            fiber->call();
        }
    }
    std::cout << "end" << std::endl;
    return 0;
}