#include "log.h"
#include <iostream>
#include <memory>

void TEST_macroDefaultLogger()
{
    std::cout << ">>>>>> Call TEST_macroLogger 测试日志器的宏函数 <<<<<<" << std::endl;
    auto logger = GET_ROOT_LOGGER();
    LOG_DEBUG(logger, "消息消息 " + std::to_string(time(nullptr)));
    LOG_INFO(logger, "消息消息 " + std::to_string(time(nullptr)));
    LOG_WARN(logger, "消息消息 " + std::to_string(time(nullptr)));
    LOG_ERROR(logger, "消息消息 " + std::to_string(time(nullptr)));
    LOG_FATAL(logger, "消息消息 " + std::to_string(time(nullptr)));
    LOG_FORMAT_DEBUG(logger, "消息消息 %s", "debug");
    LOG_FORMAT_INFO(logger, "消息消息 %s", "info");
    LOG_FORMAT_WARN(logger, "消息消息 %s", "warn");
    LOG_FORMAT_ERROR(logger, "消息消息 %s", "error");
    LOG_FORMAT_FATAL(logger, "消息消息 %s", "fatal");
}

void TEST_getNonexistentLogger()
{
    std::cout << ">>>>>> Call TEST_getNonexistentLogger 测试获取不存在的日志器 <<<<<<" << std::endl;
    try
    {
        auto logger = GET_LOGGER("noexist");   //不存在noexist，则获取全局logger
        if(logger){
            std::cout << logger->getName() << std::endl;
        }
        else{
            std::cout << "can not get noexist logger" << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
}

void TEST_loggerConfig()
{
    std::cout << ">>>>>> Call TEST_loggerConfig 测试日志器的配置文件加载 <<<<<<" << std::endl;
    auto config = hxk::Config::lookUp<std::vector<hxk::LogConfig>>("logs");
    std::cout << config->getName() <<std::endl;
    LOG_DEBUG(GET_ROOT_LOGGER(), config->toString().c_str());
    auto yaml_node = YAML::LoadFile("/home/hxk/C++Project/framework/test/config.yaml");
    hxk::Config::loadFromYAML(yaml_node);
    LOG_DEBUG(GET_ROOT_LOGGER(), config->toString().c_str());
    for(auto iter : config->getValue()) {
        std::cout << iter.name << " " << iter.formatter<< std::endl;
        std::cout << iter.appender.size() << std::endl;
    }
}

void TEST_createLoggerByYAMLFile()
{
    std::cout << ">>>>>> Call TEST_createAndUsedLogger 测试配置功能 <<<<<<" << std::endl;
    auto yaml_node = YAML::LoadFile("/home/hxk/C++Project/framework/test/config.yaml");
    hxk::Config::loadFromYAML(yaml_node);
    auto global_logger = hxk::LoggerManager_Ptr::GetInstance()->getGlobal();
    auto system_logger = hxk::LoggerManager_Ptr::GetInstance()->getLogger("system");

    LOG_DEBUG(global_logger, "输出一条 debug 日志到全局日志器");
    LOG_INFO(global_logger, "输出一条 info 日志到全局日志器");
    LOG_ERROR(global_logger, "输出一条 error 日志到全局日志器");

    LOG_DEBUG(system_logger, "输出一条 debug 日志到 system 日志器");
    LOG_INFO(system_logger, "输出一条 info 日志到 system 日志器");
    LOG_ERROR(system_logger, "输出一条 error 日志到 system 日志器");
    // auto event = MAKE_LOG_EVENT(zjl::LogLevel::DEBUG, "输出一条 debug 日志");
    // global_logger->log(event);
    // system_logger->log(event);
}


int main()
{
    TEST_macroDefaultLogger();
    //TEST_getNonexistentLogger();
    //TEST_loggerConfig();
    //TEST_createLoggerByYAMLFile();
}