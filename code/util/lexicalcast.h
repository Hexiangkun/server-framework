#pragma once

#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>

namespace hxk
{

/**
 * @Author: hxk
 * @brief: 
 * boost::lexical_cast使用std::stringstream实现类型转换
 */

template<typename Source, typename Target>
class LexicalCast
{
public:
    Target operator()(const Source& source)
    {
        return boost::lexical_cast<Target>(source);
    }
};

/**
 * @Author: hxk
 * @brief: YAML格式Sequnce到std::vector<T>的转换
 * 针对std::string  到 std::vector<T>的转换
 * @return {*}
 */
template<typename T>
class LexicalCast<std::string, std::vector<T>>
{
public:
    std::vector<T> operator()(const std::string& source)
    {
        YAML::Node node;
        node = YAML::Load(source);  //调用YAML::Load解析传入字符串，解析失败会抛出异常
        std::vector<T> config_vec;

        if(node.IsSequence())   //检查解析后的node是否为一个序列型
        {
            std::stringstream ss;
            for(const auto& item : node){
                ss.str("");
                ss << item;// 利用 YAML::Node 实现的 operator<<() 将 node 转换为字符串
                config_vec.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
        }

        return config_vec;
    }
};

/**
 * @brief YAML格式字符串到其他类型的转换仿函数
 * LexicalCast 的偏特化，针对 std::vector<T> 到 std::string 的转换
*/
template<typename T>
class LexicalCast<std::vector<T>, std::string>
{
public:
    std::string operator()(const std::vector<T>& source)
    {
        YAML::Node node;
        for(auto &item : source){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(item)));
        }
        std::stringstream ss;
        ss << node; //通过 std::stringstream 与调用 yaml-cpp 库实现的 operator<<() 将 node 转换为字符串
        return ss.str();
    }
};


/**
 * @brief YAML格式字符串到其他类型的转换仿函数
 * LexicalCast 的偏特化，针对 std::string 到 std::list<T> 的转换，
*/
template<typename T>
class LexicalCast<std::string, std::list<T>>
{
public:
    std::list<T>  operator()(const std::string& source)
    {
        YAML::Node node;
        node = YAML::Load(source);
        std::list<T> config_list;
        if(node.IsSequence()){
            std::stringstream ss;
            for(const auto& item : node){
                ss.str("");
                ss << item;
                config_list.push_back(LexicalCast<std::string, T>()(ss.str()));
            }
        }
        else{
            //LOG
        }
        return config_list;
    }
};

/**
 * @brief YAML格式字符串到其他类型的转换仿函数
 * LexicalCast 的偏特化，针对 std::list<T> 到 std::string 的转换，
*/
template<typename T>
class LexicalCast<std::list<T>, std::string>
{
public:
    std::string operator()(const std::list<T>& source)
    {
        YAML::Node node;
        for(const auto& item : source){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(item)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

/**
 * @brief YAML格式字符串到其他类型的转换仿函数
 * LexicalCast 的偏特化，针对 std::string 到 std::map<std::string, T> 的转换，
*/
template<typename T>
class LexicalCast<std::string, std::map<std::string, T>>
{
public:
    std::map<std::string, T> operator()(const std::string& source)
    {
        YAML::Node node;
        node = YAML::Load(source);
        std::map<std::string, T> config_map;
        if(node.IsMap()){
            std::stringstream ss;
            for(const auto& item : node){
                ss.str("");
                ss << item.second;
                config_map.insert(std::make_pair(item.first.as<std::string>(), LexicalCast<std::string, T>()(ss.str())));
            }
        }
        else{
            //LOG
        }
        return config_map;
    }
};

/**
 * @brief YAML格式字符串到其他类型的转换仿函数
 * LexicalCast 的偏特化，针对 std::map<std::string, T> 到 std::string 的转换，
*/
template<typename T>
class LexicalCast<std::map<std::string, T>, std::string>
{
public:
    std::string operator()(const std::map<std::string, T>& source)
    {
        YAML::Node node;
        for(const auto& item : source){
            node[item.first] = YAML::Load(LexicalCast<T, std::string>()(item.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


/**
 * @brief YAML格式字符串到其他类型的转换仿函数
 * LexicalCast 的偏特化，针对 std::string 到 std::set<T> 的转换，
*/
template<typename T>
class LexicalCast<std::string, std::set<T>>
{
public:
    std::set<T>  operator()(const std::string& source)
    {
        YAML::Node node;
        node = YAML::Load(source);
        std::set<T> config_set;
        if(node.IsSequence()){
            std::stringstream ss;
            for(const auto& item : node){
                ss.str("");
                ss << item;
                config_set.insert(LexicalCast<std::string, T>()(ss.str()));
            }
        }
        else{
            //LOG
        }
        return config_set;
    }
};

/**
 * @brief YAML格式字符串到其他类型的转换仿函数
 * LexicalCast 的偏特化，针对 std::list<T> 到 std::string 的转换，
*/
template<typename T>
class LexicalCast<std::set<T>, std::string>
{
public:
    std::string operator()(const std::set<T>& source)
    {
        YAML::Node node;
        for(const auto& item : source){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(item)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


} // namespace hxk
