/*
 * @Author: Helens-dot 374961015@qq.com
 * @Date: 2023-06-04 22:50:02
 * @LastEditors: Helens-dot 374961015@qq.com
 * @LastEditTime: 2023-07-25 21:17:17
 * @FilePath: /C++Project/framework/code/util/util.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#pragma once

#include <stdint.h> //uint64_t
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/syscall.h>    //syscall
#include <execinfo.h>       //backtrace
#include <assert.h>         //assert
#include <cxxabi.h>         //abi::__cxa_demangle
#include <sys/time.h>       //gettimeofday

#include "fiber.h"

namespace hxk
{

//获取linux线程的唯一id
long GetThreadID();

//获取协程id
uint64_t GetFiberID();



/**
 * @Author: hxk
 * @brief: 以vector形式获取调用栈
 * @param {vector} out 获取调用的栈
 * @param {int} size    获取调用栈的最大层数，默认值为200
 * @param {int} skip    省略最近n层调用栈，默认值为1，忽略获取BackTrace本身的调用栈
 * @return {*}
 */
void BackTrace(std::vector<std::string>& out, int size=200,int skip=1);

/**
 * @Author: hxk
 * @brief: 获取调用栈字符串，内部调用Backtrace
 * @param {int} size 获取调用栈最大层数，默认值200
 * @param {int} skip 省略最近n层调用栈，默认值为2， 忽略获取BackTraceToString和BackTrace
 * @return {*}
 */
std::string BackTraceToString(int size = 200, int skip = 2);

/**
 * @Author: hxk
 * @brief: 获取ms时间
 * @return {*}
 */
uint64_t GetCurrentMS();

/**
 * @Author: hxk
 * @brief: 获取us时间
 * @return {*}
 */
uint64_t GetCurrentUS();
}