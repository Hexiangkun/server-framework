#include "log.h"

#include "scheduler.h"

void fun1()
{
    for(int i=0;i<3;i++) {
        std::cout << "-------hello---------" << std::endl;
        hxk::Fiber::yieldToHold();
    }
}

void fun2()
{
    for(int i=0;i<3;i++) {
        std::cout << "-------world---------" << std::endl;
        hxk::Fiber::yieldToHold();
    }
}

int main()
{
    hxk::Scheduler sc(2, true, "test");
    sc.start();

    int i=0;
    for(i=0;i<3;i++){
        sc.schedule([&i](){
            std::cout << ">>>>" <<std::endl;
        });
    }
    sc.stop();
}