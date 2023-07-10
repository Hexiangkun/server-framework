#pragma once

#include <memory>
#include <string>
#include <algorithm>
#include <iostream>
#include "lexicalcast.h"
#include "lock.h"

namespace hxk
{

class ConfigVarBase
{
public:
    typedef std::shared_ptr<ConfigVarBase> _ptr;

    ConfigVarBase(const std::string &name, const std::string &description) : m_name(name), m_description(description)
    {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }
    virtual ~ConfigVarBase() = default;

    const std::string& getName() const { return m_name;}
    const std::string& getDescription() const { return m_description;}

    virtual std::string toString() const = 0;   //将对应的配置项的值转化为字符串
    virtual bool fromString(const std::string& val) = 0;    //通过字符串来获取配置项的值

protected:
    std::string m_name;         //配置项的名称
    std::string m_description;  //配置项的备注
};

/**
 * @Author: hxk
 * @brief: 通用型配置项的模板类
 *      T   配置项的值的类型
 *      ToStringFN  将配置项的值转为string
 *      FromStringFN 将string转换为配置项的值
 * @return {*}
 */
template<class T, class ToStringFN = LexicalCast<T, std::string>, class FromStringFN = LexicalCast<std::string, T>>
class ConfigVar : public ConfigVarBase
{
public:
    typedef std::shared_ptr<ConfigVar> _ptr;
    typedef std::function<void(const T& old_val, const T& new_val)> onChangeCB; //回调函数

    ConfigVar(const std::string& name, const std::string& description, const T& value) : ConfigVarBase(name, description),m_value(value)
    {

    }

    T getValue() const //获取配置项的值
    {
        ReadScopedLock lock(&m_mutex);
        return m_value;
    }

    void setValue(const T value)
    {
        {
            ReadScopedLock lock(&m_mutex);//上读锁
            if(value == m_value){
                return ;
            }
            T old_val = m_value;

            for(const auto& pair : m_callback_map){
                pair.second(old_val, value);
            }
        }
        WriteScopedLock lock(&m_mutex);
        m_value = value;
        std::cout << "setvalue" << value.size() << std::endl;
    }

    std::string toString() const    //返回配置项的值的字符串
    {
        try
        {
            return ToStringFN()(getValue());    //默认调用了boost::lexical_cast进行类型转换
        }
        catch(const std::exception& e)
        {
            std::cerr << "ConfigVar::tostring exception"
                      << e.what() 
                      << " convert: "
                      << typeid(m_value).name()
                      << " to string"
                      << std::endl;
        }
        return "<error>";
    }

    bool fromString(const std::string& val) override    //将yaml文本转化为配置项的值
    {   
        try
        {
            setValue(FromStringFN()(val));
            return true;
        }
        catch(const std::exception& e)
        {
            std::cerr << "ConfigVar::fromstring exception"
                      << e.what() 
                      << " convert: "
                      << " string to"
                      << typeid(m_value).name()
                      << std::endl;
        }
        return false;
    }

    uint64_t addListener(onChangeCB cb)     //增加配置项变更事件处理器，返回处理器的唯一编号
    {
        static uint64_t s_cb_id = 0;
        WriteScopedLock lock(&m_mutex);

        m_callback_map[s_cb_id] = cb;
        return s_cb_id;
    }    

    void delListener(uint64_t key)          //删除配置项变更事件处理器
    {
        WriteScopedLock lock(&m_mutex);
        m_callback_map.erase(key);
    }

    onChangeCB getListener(uint64_t key) 
    {
        ReadScopedLock lock(&m_mutex);
        if(m_callback_map.count(key)){
            return m_callback_map[key];
        }
        else{
            return nullptr;
        }
    }

    void clearListener()
    {
        WriteScopedLock lock(&m_mutex);
        m_callback_map.clear();
    }
    

private:
    T m_value;
    std::map<uint64_t, onChangeCB> m_callback_map;
    mutable RWLock m_mutex;
};


class Config
{
public:
    typedef std::map<std::string, ConfigVarBase::_ptr> ConfigVarMap;

    static ConfigVarBase::_ptr lookUp(const std::string& name)  //查找配置项，返回ConfigVarBase::_ptr指针
    {
        ReadScopedLock lock(&getRWLock());
        ConfigVarMap& m_data = getConfigVarMapData();
        if(m_data.count(name)){
            return m_data[name];
        }
        else{
            return nullptr;
        }
    }

    template<class T>
    static typename ConfigVar<T>::_ptr lookUp(const std::string& name) //查找配置项，返回ConfigVar::_ptr指针;typename的作用是把ConfigVar<T>::_ptr当作类型看待
    {
        auto base_ptr = lookUp(name);
        if(!base_ptr){
            return nullptr;
        }
        
        auto ptr = std::dynamic_pointer_cast<ConfigVar<T>>(base_ptr);
        if(!ptr){
            std::cerr << "Config::lookUp<T> exception, can't convert to ConfigVar<T>" << std::endl;
            throw std::bad_cast();
        }
        return ptr;
    }

    //创建或更新配置项          todo更新配置项
    template<class T>
    static typename ConfigVar<T>::_ptr lookUp(const std::string& name, const T& val, const std::string& description = "")
    {
        auto tmp = lookUp<T>(name);
        if(tmp){    //存在同名配置项，
            return tmp;
        }
        if(name.find_first_not_of("qwertyuiopasdfghjklzxcvbnm0123456789._") != std::string::npos){
            std::cerr << "Config::lookUp<T> exception, Parameters can only start with an alphanumeric dot or underscore." << std::endl;
            throw std::invalid_argument(name);
        }

        auto ptr = std::make_shared<ConfigVar<T>>(name,description,val);
        WriteScopedLock lock(&getRWLock());
        getConfigVarMapData()[name] = ptr;
        return ptr;
    }

    //从YAML::Node中载入配置
    static void loadFromYAML(const YAML::Node& root)
    {
        std::vector<std::pair<std::string, YAML::Node>> node_list;
        traversalNode(root, "", node_list);
        for(const auto& node : node_list) {
            std::string key = node.first;
            if(key.empty()){
                continue;
            }
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);

            //根据配置项名称获取配置项
            auto var = lookUp(key);

            if(var){
                std::stringstream ss;
                ss << node.second;
                // std::cout << key << ": " << ss.str() << std::endl;
                var->fromString(ss.str());
            }
        }
    }

    static ConfigVarMap& getConfigVarMapData()
    {
        static ConfigVarMap m_data;
        return m_data;
    }

private:
    //遍历YAML::Node对象，将遍历结果扁平化存到列表里返回
    static void traversalNode(const YAML::Node& node, const std::string& name, std::vector<std::pair<std::string, YAML::Node>>& output)
    {
        auto output_iter = std::find_if(output.begin(), output.end(), 
                            [&name](const std::pair<std::string, YAML::Node>& item){
                                return item.first == name;
                            });
        if(output_iter != output.end()){
            output_iter->second = node;
        }
        else{
            output.push_back(std::make_pair(name, node));
        }

        if(node.IsMap()){
            for(auto iter : node){
                traversalNode(iter.second, name.empty() ? iter.first.Scalar() : name + "." + iter.first.Scalar(), output);
            }
        }

        if(node.IsSequence()){
            for(size_t i=0; i<node.size(); i++){
                traversalNode(node[i], name + "." + std::to_string(i), output);
            }
        }
    }



private:
    

    static RWLock& getRWLock()
    {
        static RWLock m_lock;
        return m_lock;
    }
};

extern std::ostream& operator<<(std::ostream& out, const ConfigVarBase& cvb);

} // namespace hxk
