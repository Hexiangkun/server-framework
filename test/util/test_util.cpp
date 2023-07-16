#include "util.h"
#include <iostream>

int main()
{
    uint64_t tmp =0;
    uint64_t begin = hxk::GetCurrentMS();

    for(int i=0;i<655360;i++){
        tmp+=hxk::GetCurrentMS();
        tmp%=655360;
    }

    uint64_t end = hxk::GetCurrentMS();
    std::cout << end - begin <<std::endl;

    std::cout << hxk::BackTraceToString() << std::endl;
    return 0;
}
