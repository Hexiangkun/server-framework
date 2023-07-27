#include "log.h"

//LogLevel
namespace hxk
{

std::string LogLevel::levelToString(LogLevel::Level level)
{
    switch (level)
    {
    case UNKNOWN:
        return "UNKONWN";
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO ";
    case WARN:
        return "WARN ";
    case ERROR:
        return "ERROR";
    case FATAL:
        return "FATAL";
    default:
        return "UNKONWN";
        break;
    }
}

LogLevel::Level LogLevel::fromStringToLevel(std::string str) {
    switch(str)
    {
    case "UNKONWN":
        return LogLevel::Level::UNKNOWN;
    case "DEBUG":
        return Level::DEBUG;
    case "INFO":
        return Level::INFO;
    case "WARN":
        return Level::WARN;
    case "ERROR":
        return Level::ERROR;
    case "FATAL":
        return Level::FATAL;
    default:
        return Level::UNKNOWN;
    }
}
}

//LogEvent
namespace hxk
{

LogEvent::LogEvent(const std::string& filename, uint32_t line, uint32_t threadid, uint32_t fiberid, 
                    time_t time, const std::string& content, LogLevel::Level level)
        :m_level(level), m_filename(filename), m_line(line), 
        m_thread_id(threadid), m_fiber_id(fiberid), m_time(time),
        m_content(content){

        }

LogLevel::Level LogEvent::getLevel() const 
{ 
    return m_level;
}

void LogEvent::setLevel(LogLevel::Level level) 
{
    m_level = level;
}

const std::string& LogEvent::getFilename() const 
{ 
    return m_filename;
}

uint32_t LogEvent::getLine() const 
{
    return m_line;
}

uint32_t LogEvent::getThreadId() const 
{ 
    return m_thread_id;
}

uint32_t LogEvent::getFiberId() const 
{ 
    return m_fiber_id; 
}

time_t LogEvent::getTime() const 
{ 
    return m_time; 
}

const std::string& LogEvent::getContent() const 
{ 
    return m_content;
}
}
/**
 * FormatItem
 * %p 输出日志等级
 * %f 输出文件名
 * %l 输出行号
 * %d 输出日志时间
 * %t 输出线程号
 * %F 输出协程号
 * %m 输出日志消息
 * %n 输出换行
 * %% 输出百分号
 * %T 输出制表符
 * */
namespace hxk
{

/**
 * @Author: hxk
 * @brief: 输出字符串
 * @return {*}
 */
class PlainFormatItem : public FormatItem
{
public:
    explicit PlainFormatItem(const std::string& str) : m_str(str){}
    void format(std::ostream& out, LogEvent::_ptr ev) override
    {
        out << m_str;
    }
private:
    std::string m_str;
};

/**
 * @Author: hxk
 * @brief: 输出日志等级
 * @return {*}
 */
class LevelFormatItem : public FormatItem
{
public: 
    void format(std::ostream& out, LogEvent::_ptr ev)
    {
        out << LogLevel::levelToString(ev->getLevel());
    }
};

/**
 * @Author: hxk
 * @brief: 输出文件名
 * @return {*}
 */
class FileNameFormatItem : public FormatItem
{
public: 
    void format(std::ostream& out, LogEvent::_ptr ev)
    {
        out << ev->getFilename();
    }
};

/**
 * @Author: hxk
 * @brief: 输出行号
 * @return {*}
 */
class LineFormatItem : public FormatItem
{
public: 
    void format(std::ostream& out, LogEvent::_ptr ev)
    {
        out << ev->getLine();
    }
};

/**
 * @Author: hxk
 * @brief: 输出线程id
 * @return {*}
 */
class ThreadIdFormatItem : public FormatItem
{
public: 
    void format(std::ostream& out, LogEvent::_ptr ev)
    {
        out << ev->getThreadId();
    }
};

/**
 * @Author: hxk
 * @brief: 输出协程id
 * @return {*}
 */
class FiberIdFormatItem : public FormatItem
{
public: 
    void format(std::ostream& out, LogEvent::_ptr ev)
    {
        out << ev->getFiberId();
    }
};

/**
 * @Author: hxk
 * @brief: 输出时间
 * @return {*}
 */
class TimeFormatItem : public FormatItem
{
public: 
    explicit TimeFormatItem(const std::string &str = "%Y-%m-%d %H:%M:%S"):m_time_pattern(str)
    {
        if(m_time_pattern.empty()){
            m_time_pattern = "%Y-%m-%d %H:%M:%S";
        }
    }
    void format(std::ostream& out, LogEvent::_ptr ev)
    {
        struct tm t;
        time_t tt = ev->getTime();
        localtime_r(&tt,&t);
        char time_buf[64]={0};
        strftime(time_buf, sizeof(time_buf), m_time_pattern.c_str(),&t);
        out << time_buf;
    }
private:
    std::string m_time_pattern;
};

/**
 * @Author: hxk
 * @brief: 输出内容
 * @return {*}
 */
class ContentFormatItem : public FormatItem
{
public: 
    void format(std::ostream& out, LogEvent::_ptr ev)
    {
        out << ev->getContent();
    }
};

/**
 * @Author: hxk
 * @brief: 空行
 * @return {*}
 */
class NewLineFormatItem : public FormatItem
{
public: 
    void format(std::ostream& out, LogEvent::_ptr ev)
    {
        out << std::endl;
    }
};

/**
 * @Author: hxk
 * @brief: 百分号 %
 * @return {*}
 */
class PercentSignFormatItem : public FormatItem
{
public: 
    void format(std::ostream& out, LogEvent::_ptr ev)
    {
        out << "%";
    }
};

/**
 * @Author: hxk
 * @brief: Tab制表符
 * @return {*}
 */
class TabFormatItem : public FormatItem
{
public: 
    void format(std::ostream& out, LogEvent::_ptr ev)
    {
        out << '\t';
    }
};


/*
有且只有 thread_local 关键字修饰的变量具有线程（thread）周期，
这些变量在线程开始的时候被生成，在线程结束的时候被销毁，
并且每一个线程都拥有一个独立的变量实例。
*/                                 
thread_local static std::map<char, FormatItem::_ptr> format_item_map{
#define FN(CH, ITEM_NAME)                   \
    {                                       \
        CH, std::make_shared<ITEM_NAME>()   \
    } 
    FN('p', LevelFormatItem),
    FN('f', FileNameFormatItem),
    FN('l', LineFormatItem),
    FN('d', TimeFormatItem),
    FN('F', FiberIdFormatItem),
    FN('t', ThreadIdFormatItem),
    FN('m', ContentFormatItem),
    FN('n', NewLineFormatItem),
    FN('%', PercentSignFormatItem),
    FN('T', TabFormatItem),
#undef FN
};
}

//LogFormatter
namespace hxk
{
LogFormatter::LogFormatter(const std::string& pattern):m_format_pattern(pattern)
{
    init();
}

std::string LogFormatter::format(LogEvent::_ptr ev)
{
    std::stringstream ss;
    for(auto& item : m_formatItem_list){
        item->format(ss, ev);
    }
    return ss.str();
}

void LogFormatter::init()
{
    enum PARSE_STATUS{
        SCAN_STATUS,    //扫描到普通字符
        CREATE_STATUS   //扫描到%，处理占位字符
    };
    PARSE_STATUS status = SCAN_STATUS;
    size_t str_begin = 0;
    size_t str_end = 0;
    //[%d] [%p] [T:%t F:%F]%T%m%n
    for(size_t i=0; i<m_format_pattern.length();i++){
        switch(status)
        {
            case SCAN_STATUS:
                str_begin = i;
                for(str_end = i; str_end < m_format_pattern.length();str_end++){
                    if(m_format_pattern[str_end] == '%'){
                        status = CREATE_STATUS;
                        break;
                    }
                }
                i=str_end;
                m_formatItem_list.push_back(std::make_shared<PlainFormatItem>(
                    m_format_pattern.substr(str_begin, str_end-str_begin)));
                break;
            case CREATE_STATUS:
                assert(!format_item_map.empty() && "format_item_map don't init correctly");
                auto iter = format_item_map.find(m_format_pattern[i]);  //因为上一个case会让i加1
                if(iter == format_item_map.end()){
                    m_formatItem_list.push_back(std::make_shared<PlainFormatItem>("<error fromat>"));
                }
                else{
                    m_formatItem_list.push_back(iter->second);
                }
                status = SCAN_STATUS;
                break;
        }
    }
}
}

//LogAppender && StdoutLogAppender && FileLogAppender
namespace hxk
{
//LogAppender
LogAppender::LogAppender(LogLevel::Level level):m_level(level)
{

}

LogFormatter::_ptr LogAppender::getFormatter()
{
    ScopedLock lock(&m_mutex);
    return m_formatter;
}

void LogAppender::setFormatter(LogFormatter::_ptr formatter)
{
    ScopedLock lock(&m_mutex);
    m_formatter = std::move(formatter);
}

//StdoutLogAppender
StdoutLogAppender::StdoutLogAppender(LogLevel::Level level) : LogAppender(level)
{

}

void StdoutLogAppender::log(LogLevel::Level level, LogEvent::_ptr ev)
{
    if(level < m_level)
    {
        return;
    }
    ScopedLock lock(&m_mutex);
    std::cout << m_formatter->format(ev);
    std::cout.flush();
}

//FileLogAppender
FileLogAppender::FileLogAppender(const std::string& filename, LogLevel::Level level):LogAppender(level), m_filename(filename)
{
    reopen();
}

bool FileLogAppender::reopen()
{
    ScopedLock lock(&m_mutex);
    /*
    使用 fstream::operator!() 操作符来检查文件流对象 m_file_stream 是否有效
    true if an error has occurred, false otherwise.
    */
    if(!m_file_stream){     
        m_file_stream.close();
    }
    m_file_stream.open(m_filename, std::ios_base::out | std::ios_base::app);
    return !!m_file_stream;
}

void FileLogAppender::log(LogLevel::Level level, LogEvent::_ptr ev)
{
    if(level < m_level){
        return ;
    }
    ScopedLock lock(&m_mutex);
    m_file_stream << m_formatter->format(ev);
    m_file_stream.flush();
}
}

//Logger
namespace hxk
{
Logger::Logger():m_name("default"), m_level(LogLevel::DEBUG), 
                    m_formatter_pattern("[%d] [%p] [T:%t F:%F]%T%m%n")
{
    m_formatter.reset(new LogFormatter(m_formatter_pattern));
}

Logger::Logger(const std::string& name, LogLevel::Level level, const std::string& pattern)
                :m_name(name), m_level(level), m_formatter_pattern(pattern)
{
    m_formatter.reset(new LogFormatter(m_formatter_pattern));
}

void Logger::addAppender(LogAppender::_ptr appender)
{
    ScopedLock lock(&m_mutex);
    if(!appender->getFormatter()){
        appender->setFormatter(m_formatter);
    }
    m_appender_list.push_back(appender);
}

void Logger::delAppender(LogAppender::_ptr appender)
{
    ScopedLock lock(&m_mutex);
    auto iter = std::find(m_appender_list.begin(), m_appender_list.end(), appender);
    if(iter != m_appender_list.end()){
        m_appender_list.erase(iter);
    }
}

LogLevel::Level Logger::getLevel() const 
{ 
    return m_level;
}

void Logger::setLevel(LogLevel::Level level) 
{ 
    m_level = level;
}

std::string Logger::getName() const 
{ 
    return m_name; 
}

void Logger::log(LogEvent::_ptr ev)
{
    if(ev->getLevel() < m_level){
        return;
    }
    ScopedLock lock(&m_mutex);
    for(auto& iter:m_appender_list){
        iter->log(ev->getLevel(), ev);
    }
}

}

//__LoggerManager
namespace hxk
{
__LoggerManager::__LoggerManager()
{
    init();
}

void __LoggerManager::init(std::vector<LogConfig> config_log_list)
{
    ScopedLock lock(&m_mutex);
    auto config = Config::lookUp<std::vector<LogConfig>>("logs");
    
    //const auto& config_log_list = config->getValue();
    // std::cout << "init" << config_log_list.size() << std::endl;
    for(const auto& config_log : config_log_list) {
        m_logger_map.erase(config_log.name);    //删除同名的logger
        auto logger = std::make_shared<Logger>(config_log.name,config_log.level,config_log.formatter);

        for(const auto& config_appender : config_log.appender) {
            LogAppender::_ptr appender;
            switch (config_appender.output_type)
            {
                case LogAppenderConfig::STDOUT:
                    appender = std::make_shared<StdoutLogAppender>(config_appender.level);
                    break;
                case LogAppenderConfig::FILE:
                    appender = std::make_shared<FileLogAppender>(config_appender.file_path, config_appender.level);
                default:
                    std::cerr << "__LoggerManager::init exception invalid appender' values, appender.type=" << config_appender.output_type << std::endl;
                    break;
            }
            //如果定义了appender的日志格式，个性化formatter，否则跟随logger
            if(!config_appender.formatter.empty()) {
                appender->setFormatter(std::make_shared<LogFormatter>(config_appender.formatter));
            }
            logger->addAppender(std::move(appender));
        }
        std::cout << "create Logger successfully " << config_log.name << std::endl;
        m_logger_map.insert(std::make_pair(config_log.name, std::move(logger)));
    }
    ensureGlobalLoggerExists();
}

template<class T>
void __LoggerManager::update_init(const T& config_log_list) 
{
    std::cout << "update_init" << config_log_list.size() << std::endl;
    for(const auto& config_log : config_log_list) {
        m_logger_map.erase(config_log.name);    //删除同名的logger
        auto logger = std::make_shared<Logger>(config_log.name,config_log.level,config_log.formatter);

        for(const auto& config_appender : config_log.appender) {
            LogAppender::_ptr appender;
            switch (config_appender.output_type)
            {
                case LogAppenderConfig::STDOUT:
                    appender = std::make_shared<StdoutLogAppender>(config_appender.level);
                    break;
                case LogAppenderConfig::FILE:
                    appender = std::make_shared<FileLogAppender>(config_appender.file_path, config_appender.level);
                default:
                    std::cerr << "__LoggerManager::init exception invalid appender' values, appender.type=" << config_appender.output_type << std::endl;
                    break;
            }
            //如果定义了appender的日志格式，个性化formatter，否则跟随logger
            if(!config_appender.formatter.empty()) {
                appender->setFormatter(std::make_shared<LogFormatter>(config_appender.formatter));
            }
            logger->addAppender(std::move(appender));
        }
        std::cout << "create Logger successfully " << config_log.name << std::endl;
        m_logger_map.insert(std::make_pair(config_log.name, std::move(logger)));
    }
}

void __LoggerManager::ensureGlobalLoggerExists()
{
    auto iter = m_logger_map.find("global");
    if(iter == m_logger_map.end()){
        auto global_logger = std::make_shared<Logger>();
        global_logger->addAppender(std::make_shared<StdoutLogAppender>());
        m_logger_map.insert(std::make_pair("global", std::move(global_logger)));
    }
    else if(!iter->second){
        iter->second = std::make_shared<Logger>();
        iter->second->addAppender(std::make_shared<StdoutLogAppender>());
    }
}

Logger::_ptr __LoggerManager::getLogger(const std::string& name)
{
    ScopedLock lock(&m_mutex);
    auto iter = m_logger_map.find(name);
    if(iter == m_logger_map.end()){
        return m_logger_map["global"];
    }
    else{
        return iter->second;
    }
}

Logger::_ptr __LoggerManager::getGlobal()
{
    return m_logger_map["global"];
}
}



