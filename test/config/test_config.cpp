#include "yaml-cpp/yaml.h"
#include "/home/hxk/C++Project/framework/code/util/config.h"
#include <iostream>


auto config_system_port = hxk::Config::lookUp<int>("system.port", 6666);

auto config_test_list = hxk::Config::lookUp<std::vector<std::string>>(
        "test_list", std::vector<std::string>{"vector","string"});

auto config_test_linklist = hxk::Config::lookUp<std::list<std::string>>(
    "test_linklist", std::list<std::string>{"list", "string"});
auto config_test_map = hxk::Config::lookUp<std::map<std::string, std::string>>(
    "test_map", std::map<std::string, std::string>{
                    std::make_pair("map1", "srting"),
                    std::make_pair("map2", "srting"),
                    std::make_pair("map3", "srting")});
auto config_test_set = hxk::Config::lookUp<std::set<int>>(
    "test_set", std::set<int>{10, 20, 30});


/*
test_list: - 1
- 2
- 3
test_linklist: - asdasd
- asdasd
- asdasd
- asdasd
- asdasd
- asdasd
- asdasd
- asdasd
- asdasd
test_map: map_1: 1000
map_2: 2000
map_3: 3000
test_set: [1000, 2000, 3000, 1000, 2000, 3000]
*/

//自定义类的解析
struct Goods
{
public:
    std::string name;
    double price;
public:
    std::string toString() const
    {
        std::stringstream ss;
        ss << "**" << name << "** $" <<price;
        return ss.str();
    }

    bool operator==(const Goods& rhs) const
    {
        return name == rhs.name &&  price == rhs.price;
    }


};

std::ostream& operator<<(std::ostream& out, const Goods& goods)
{
    out << goods.toString();
    return out;
}

namespace hxk
{
template<>
class LexicalCast<std::string, Goods>
{
public:
    Goods operator()(const std::string& source)
    {
        auto node = YAML::Load(source);
        Goods g;
        if(node.IsMap()){
            g.name = node["name"].as<std::string>();
            g.price = node["price"].as<double>();
        }
        return g;
    }
};

template<>
class LexicalCast<Goods,std::string>
{
public:
    std::string operator()(const Goods& g)
    {
        YAML::Node node;
        node["name"] = g.name;
        node["price"] = g.price;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};
}

auto config_test_user_type = hxk::Config::lookUp<Goods>("user.goods",Goods{});
auto config_test_user_type_list  = hxk::Config::lookUp<std::vector<Goods>>("user.goods_list",std::vector<Goods>{});


void test_loadConfig(const std::string& path)
{
    YAML::Node config;
    try
    {
        config = YAML::LoadFile(path);
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    std::cout << config << std::endl;
    hxk::Config::loadFromYAML(config);
}

void test_ConfigVarToString()
{
    std::cout << *config_system_port << std::endl;
    std::cout << *config_test_list << std::endl;
    std::cout << *config_test_linklist << std::endl;
    std::cout << *config_test_map << std::endl;
    std::cout << *config_test_set << std::endl;
    std::cout << *config_test_user_type << std::endl;
    std::cout << *config_test_user_type_list << std::endl;
}

// 测试获取并使用配置的值
void TEST_GetConfigVarValue()
{
// 遍历线性容器的宏
#define TSEQ(config_var)                                             \
    std::cout << "name = " << config_var->getName() << "; value = "; \
    for (const auto& item : config_var->getValue())                  \
    {                                                                \
        std::cout << item << ", ";                                   \
    }                                                                \
    std::cout << std::endl;

    TSEQ(config_test_list);
    TSEQ(config_test_linklist);
    TSEQ(config_test_set);
    TSEQ(config_test_user_type_list);
#undef TSEQ
// 遍历映射容器的宏
#define TMAP(config_var)                                                \
    std::cout << "name = " << config_var->getName() << "; value = ";    \
    for (const auto& pair : config_var->getValue())                     \
    {                                                                   \
        std::cout << "<" << pair.first << ", " << pair.second << ">, "; \
    }                                                                   \
    std::cout << std::endl;

    TMAP(config_test_map);
#undef TMAP
}


// 测试获取不存在的配置项
void TEST_nonexistentConfig()
{
    auto log_name = hxk::Config::lookUp("nonexistent");
    if (!log_name)
    {
        std::cout << "no value" << std::endl;
    }
}


int main()
{
    test_loadConfig("/home/hxk/C++Project/framework/test/test_config.yaml");
    std::cout <<std::endl;
    test_ConfigVarToString();
    std::cout <<std::endl;
    TEST_GetConfigVarValue();
    std::cout << std::endl;
    TEST_nonexistentConfig();
    return 0;
}