#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include "singleInstance.h"
#include "lexicalcast.h"
#include "lock.h"
#include "config.h"
#include "util.h"

#define MAKE_LOG_EVENT(level, message)  \
    std::make_shared<hxk::LogEvent>(__FILE__, __LINE__, hxk::GetThreadID(),hxk::GetFiberID(), ::time(nullptr), message, level)

#define LOG_LEVEL(logger, level, message) \
    logger->log(MAKE_LOG_EVENT(level, message));

#define LOG_DEBUG(logger, message) LOG_LEVEL(logger, hxk::LogLevel::DEBUG, message)
#define LOG_INFO(logger, message) LOG_LEVEL(logger, hxk::LogLevel::INFO, message)
#define LOG_WARN(logger, message) LOG_LEVEL(logger, hxk::LogLevel::WARN, message)
#define LOG_ERROR(logger, message) LOG_LEVEL(logger, hxk::LogLevel::ERROR, message)
#define LOG_FATAL(logger, message) LOG_LEVEL(logger, hxk::LogLevel::FATAL, message)
#define LOG(logger, message) LOG_LEVEL(logger, hxk::LogLevel::FATAL, message)

#define LOG_S_EVENT(logger,level)  std::make_shared<hxk::LogEventWrap>(logger, level)->getSS()

#define LOG_S_DEBUG(logger) LOG_S_EVENT(logger, hxk::LogLevel::DEBUG)

#define LOG_FORMAT_LEVEL(logger, level, format, argv...)        \
    {                                                           \
        char* _buf = nullptr;                                   \
        int len = asprintf(&_buf, format, argv);                \
        if(len != -1){                                          \
            LOG_LEVEL(logger, level, std::string(_buf, len));   \
            free(_buf);                                         \
        }                                                       \
    }

#define LOG_FORMAT_DEBUG(logger, format, argv...) LOG_FORMAT_LEVEL(logger, hxk::LogLevel::DEBUG, format, argv)
#define LOG_FORMAT_INFO(logger, format, argv...) LOG_FORMAT_LEVEL(logger, hxk::LogLevel::INFO, format, argv)
#define LOG_FORMAT_WARN(logger, format, argv...) LOG_FORMAT_LEVEL(logger, hxk::LogLevel::WARN, format, argv)
#define LOG_FORMAT_ERROR(logger, format, argv...) LOG_FORMAT_LEVEL(logger, hxk::LogLevel::ERROR, format, argv)
#define LOG_FORMAT_FATAL(logger, format, argv...) LOG_FORMAT_LEVEL(logger, hxk::LogLevel::FATAL, format, argv)

#define GET_ROOT_LOGGER() hxk::LoggerManager_Ptr::GetInstance()->getGlobal()
#define GET_LOGGER(name) hxk::LoggerManager_Ptr::GetInstance()->getLogger(name)

namespace hxk
{

/// @brief 日志级别
class LogLevel
{
public:
    enum Level{
        UNKNOWN = 0,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL
    };
    /// 将日志级别转换为文本
    static std::string levelToString(LogLevel::Level level);
    static LogLevel::Level fromStringToLevel(std::string str);
};


/// @brief 日志器的Appender的配置信息类
struct LogAppenderConfig
{
    enum Type
    {
        STDOUT = 0,
        FILE
    };
    LogAppenderConfig::Type output_type; //输出器的类型
    LogLevel::Level level;  //输出器的日志有效等级
    std::string formatter;  //输出器的日志打印格式
    std::string file_path;  //输出器的目标文件路径

    LogAppenderConfig():output_type(Type::STDOUT), level(LogLevel::UNKNOWN){}

    bool operator==(const LogAppenderConfig& lhs) const
    {
        return output_type == lhs.output_type &&
                level == lhs.level &&
                formatter == lhs.formatter &&
                file_path == lhs.file_path;
    }

};



/// @brief 日志器的配置信息类
struct LogConfig
{
    std::string name;   //日志器的名称
    LogLevel::Level level;
    std::string formatter;
    std::vector<LogAppenderConfig> appender;    //日志器的输出器配置集合

    LogConfig():level(LogLevel::UNKNOWN){}

    bool operator==(const LogConfig& lhs) const
    {
        return name == lhs.name;
    }
};



/**
 * @Author: hxk
 * @brief: 日志消息类
 * @return {*}
 */
class LogEvent
{
public:
    typedef std::shared_ptr<LogEvent> _ptr;
    
    LogEvent(const std::string& filename, uint32_t line, uint32_t threadid, uint32_t fiberid, time_t time, const std::string& content, LogLevel::Level level = LogLevel::DEBUG);

    LogLevel::Level getLevel() const ;
    void setLevel(LogLevel::Level level) ;

    const std::string& getFilename() const;
    uint32_t getLine() const ;
    uint32_t getThreadId() const ;
    uint32_t getFiberId() const ;
    time_t getTime() const ;
    const std::string& getContent() const ;

private:
    LogLevel::Level m_level;    //日志等级
    std::string m_filename;     //文件名
    uint32_t m_line;            //行号
    uint32_t m_thread_id;       //线程id
    uint32_t m_fiber_id;        //携程id

    time_t m_time;              //时间
    std::string m_content;      //内容
};

class Logger;

class LogEventWrap
{
public:
    LogEventWrap(Logger::_ptr& logger, LogLevel::Level level):m_logerr(logger),m_level(level)
    {

    }
    ~LogEventWrap()
    {
        m_logerr->log(MAKE_LOG_EVENT(m_level, ss.str()));
    }

    std::stringstream& getSS()
    {
        return ss;
    }
private:
    Logger::_ptr m_logerr;
    LogLevel::Level m_level;
    std::stringstream ss;
};


class FormatItem
{
public:
    typedef std::shared_ptr<FormatItem> _ptr;
    virtual void format(std::ostream& out, LogEvent::_ptr ev) = 0;
};

/**
 * @Author: hxk
 * @brief: 日志格式化器
 *  构造时传入日志格式化字符串，调用format()传入LogEvent实例，返回格式化后的字符串
 * @return {*}
 */
class LogFormatter
{
public:
    typedef std::shared_ptr<LogFormatter> _ptr;

    explicit LogFormatter(const std::string& pattern);
    std::string format(LogEvent::_ptr ev); 

private:
    void init();

    std::string m_format_pattern;       
    std::vector<FormatItem::_ptr> m_formatItem_list;
};

/**
 * @Author: hxk
 * @brief: 日志输出器基类
 * @return {*}
 */
class LogAppender
{
public:
    typedef std::shared_ptr<LogAppender> _ptr;

    explicit LogAppender(LogLevel::Level level = LogLevel::DEBUG);
    virtual ~LogAppender() = default;

    virtual void log(LogLevel::Level level, LogEvent::_ptr ev) = 0;

    LogFormatter::_ptr getFormatter();  //thread-safe获取格式化器

    void setFormatter(LogFormatter::_ptr formatter);    //thread-safe设置格式化器

protected:
    LogLevel::Level m_level;
    LogFormatter::_ptr m_formatter;
    Mutex m_mutex;
};

/**
 * @Author: hxk
 * @brief: 输出到终端的Appender
 * @return {*}
 */
class StdoutLogAppender : public LogAppender
{
public:
    typedef std::shared_ptr<StdoutLogAppender> _ptr;

    explicit StdoutLogAppender(LogLevel::Level level = LogLevel::DEBUG);

    void log(LogLevel::Level level, LogEvent::_ptr ev) override;
};

/**
 * @Author: hxk
 * @brief: 输出到文件的Appender
 * @return {*}
 */
class FileLogAppender : public LogAppender
{
public:
    typedef std::shared_ptr<FileLogAppender> _ptr;

    explicit FileLogAppender(const std::string& filename, LogLevel::Level level = LogLevel::DEBUG);

    void log(LogLevel::Level level, LogEvent::_ptr ev) override;

    bool reopen();

private:
    std::string m_filename;
    std::ofstream m_file_stream;
};

class Logger
{
public:
    typedef std::shared_ptr<Logger> _ptr;

    Logger();
    Logger(const std::string& name, LogLevel::Level level, const std::string& pattern);

    void log(LogEvent::_ptr ev);

    void addAppender(LogAppender::_ptr appender);
    void delAppender(LogAppender::_ptr appender);

    LogLevel::Level getLevel() const ;
    void setLevel(LogLevel::Level level) ;

    std::string getName() const ;
private:
    LogLevel::Level m_level;
    const std::string m_name;   //日志器名称
    std::string m_formatter_pattern;    //日志输出格式化器的默认pattern
    LogFormatter::_ptr m_formatter;     //日志默认格式化器
    std::list<LogAppender::_ptr> m_appender_list;
    Mutex m_mutex;
};

class __LoggerManager
{
public:
    typedef std::shared_ptr<__LoggerManager> _ptr;
    __LoggerManager();

    Logger::_ptr getLogger(const std::string& name);    //传入日志器名称来获取日志器
    Logger::_ptr getGlobal();   //返回全局日志器

private:
    friend struct LogIniter;

    void init(std::vector<LogConfig> config_log_vec = {});
    void ensureGlobalLoggerExists();

    template<class T>
    void update_init(const T& val);

    std::map<std::string, Logger::_ptr> m_logger_map;
    Mutex m_mutex;
};

/**
 * @Author: hxk
 * @brief: __LoggerManager的单例类
 * @return {*}
 */
typedef SingleInstancePtr<__LoggerManager> LoggerManager_Ptr;

struct LogIniter
{
    LogIniter()
    {
        auto log_config_list = hxk::Config::lookUp<std::vector<LogConfig>>("logs", {}, "日志器的配置项");
        log_config_list->addListener([](const std::vector<LogConfig>& old_val, const std::vector<LogConfig>& new_val){
            std::cout << "日志器配置变动，更新日志器" << std::endl;
            LoggerManager_Ptr::GetInstance()->init(new_val);
            //LoggerManager_Ptr::GetInstance()->update_init<std::vector<LogConfig>>(new_val);
        });
    }
};

static LogIniter __log_initer__;    //静态变量，程序开始一直存在
}

namespace hxk
{
template <>
class LexicalCast<std::string, std::vector<LogConfig>>
{
public:
    std::vector<LogConfig> operator()(const std::string& source)
    {
        auto node = YAML::Load(source);
        std::vector<LogConfig> res;
        if(node.IsSequence()){
            for(const auto& log_config : node){
                LogConfig log_cfg;
                log_cfg.name = log_config["name"] ? log_config["name"].as<std::string>():"";
                log_cfg.level = log_config["level"] ? (LogLevel::Level)(log_config["level"].as<int>()) : LogLevel::UNKNOWN;
                log_cfg.formatter = log_config["formatter"] ? log_config["formatter"].as<std::string>(): "";

                if(log_config["appender"] && log_config["appender"].IsSequence()){
                    for(const auto& app_config : log_config["appender"]){
                        LogAppenderConfig appender_cfg{};
                        appender_cfg.output_type = (LogAppenderConfig::Type)(app_config["type"] ? app_config["type"].as<int>() : 0);
                        appender_cfg.file_path = app_config["file"] ? app_config["file"].as<std::string>() : "";
                        appender_cfg.level = (LogLevel::Level)(app_config["level"] ? app_config["level"].as<int>() : log_cfg.level);
                        appender_cfg.formatter = app_config["formatter"] ? app_config["formatter"].as<std::string>() : log_cfg.formatter;
                        log_cfg.appender.push_back(appender_cfg);
                    }
                }

                res.push_back(log_cfg);
            }
        }
        std::cout << "lex: " << res.size() << std::endl; 
        return res;
    }
};


template<>
class LexicalCast<std::vector<LogConfig>, std::string>
{
public:
    std::string operator()(const std::vector<LogConfig>& source)
    {
        YAML::Node res;
        for(const auto& log_config : source){
            YAML::Node node;
            node["name"] = log_config.name;
            node["level"] = (int)log_config.level;
            node["formatter"] = log_config.formatter;

            YAML::Node app_node_list;
            for(const auto& app_config : log_config.appender){
                YAML::Node app_node;
                app_node["type"] = (int)app_config.output_type;
                app_node["file"] = app_config.file_path;
                app_node["level"] = (int)app_config.level;
                app_node["formatter"] = app_config.formatter;
                app_node_list.push_back(app_node);
            }

            node["appender"] = app_node_list;
            res.push_back(node);
        }

        std::stringstream ss;
        ss << res;
        return ss.str();
    }
};
}