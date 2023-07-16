#include "util.h"

namespace hxk
{

long GetThreadID()
{
    return ::syscall(SYS_gettid);
}

uint64_t GetFiberID()
{
    return Fiber::getThisFiberId();
}

void BackTrace(std::vector<std::string>& out, int size, int skip)
{
    void** void_ptr_list = (void**)malloc(sizeof(void*) *size);
    int call_stack_count = ::backtrace(void_ptr_list, size);
    char** string_list = ::backtrace_symbols(void_ptr_list,call_stack_count);
    if(string_list == NULL){
        std::cerr << "Backtrace() exception!" << std::endl;
        return;
    }

    for(int i = skip; string_list && i < call_stack_count; i++){
        std::stringstream ss;
        char* str = string_list[i];
        char* bracket_pos = nullptr;
        char* plus_pos = nullptr;
        //找到左括号位置
        for(bracket_pos = str; *bracket_pos != '(' && *bracket_pos; bracket_pos++){

        }
        assert(*bracket_pos == '(');
        *bracket_pos = '\0';    //把左括号之前的字符串塞进字符串流中
        ss << string_list[i] << '(' ;
        *bracket_pos = '(';

        //找到+号位置
        for(plus_pos = bracket_pos; *plus_pos != '+' && *plus_pos; plus_pos++){

        }

        char* type = nullptr;
        if(*bracket_pos + 1 != *plus_pos){
            *plus_pos = '\0';
            int status;
            type = abi::__cxa_demangle(bracket_pos + 1, nullptr, nullptr, &status);
            assert(status == 0 || status == -2);
            ss << (status == 0) ? type : bracket_pos +1;    //status=-2，代表字符串解析错误
            *plus_pos = '+';
        }

        ss << plus_pos; //把剩下的字符串塞进去
        out.push_back(ss.str());
        free(type);
    }
    free(string_list);
    free(void_ptr_list);
}

std::string BackTraceToString(int size, int skip)
{
    std::vector<std::string> call_stack;
    BackTrace(call_stack, size, skip);
    std::stringstream ss;
    for(const auto& item : call_stack){
        ss << item << std::endl;
    }
    return ss.str();
}

uint64_t GetCurrentMS()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

uint64_t GetCurrentUS()
{
    timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec * 1000ul * 1000ul +tv.tv_usec;
}



}