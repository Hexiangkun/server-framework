#pragma once

#include <memory>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string>
#include <iostream>

namespace hxk
{

class Address
{
public:
    using _ptr = std::shared_ptr<Address>;
    // Address() = default;
    virtual ~Address() = default;

    int getFamily() const ; //获取协议族

    virtual const sockaddr* getAddr() const = 0;

    virtual socklen_t getAddrLen() const = 0;

    std::string toString();
    
    bool operator<(const Address& rhs) const;
    bool operator==(const Address& rhs) const;
    bool operator!=(const Address& rhs) const;

protected:
    virtual std::ostream& insert(std::ostream& os) const;
};

class IPAddress : public Address
{
public:
    using _ptr = std::shared_ptr<IPAddress>;
    //获取广播地址
    virtual IPAddress::_ptr broadcastAddress(uint32_t prefix_len) = 0;
    //获取网络地址
    virtual IPAddress::_ptr networkAddress(uint32_t prefix_len) = 0;
    //获取子网掩码
    virtual IPAddress::_ptr subnetAddress(uint32_t prefix_len) = 0;
    //获取端口号
    virtual uint32_t getPort() const = 0;
    //设置端口号
    virtual void setPort(uint16_t port) = 0;
};

class IPv4Address : public IPAddress
{
public:
    using _ptr = std::shared_ptr<IPv4Address>;

    IPv4Address(uint32_t address = INADDR_ANY, uint16_t port = 0);

    //获取socket地址
    const sockaddr* getAddr() const override;
    //获取地址长度
    socklen_t getAddrLen() const override;
    //获取广播地址
    IPAddress::_ptr broadcastAddress(uint32_t prefix_len) override;
    //获取网络地址
    IPAddress::_ptr networkAddress(uint32_t prefix_len) override;
    //获取子网掩码
    IPAddress::_ptr subnetAddress(uint32_t prefix_len) override;

    uint32_t getPort() const override;

    void setPort(uint16_t port) override;

protected:
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr_in m_addr;
};

class IPv6Address : public IPAddress
{
public:
    using _ptr = std::shared_ptr<IPv6Address>;

    IPv6Address();
    IPv6Address(const char* address = "", uint16_t port = 0);

    //获取socket地址
    const sockaddr* getAddr() const override;
    //获取地址长度
    socklen_t getAddrLen() const override;
    //获取广播地址
    IPAddress::_ptr broadcastAddress(uint32_t prefix_len) override;
    //获取网络地址
    IPAddress::_ptr networkAddress(uint32_t prefix_len) override;
    //获取子网掩码
    IPAddress::_ptr subnetAddress(uint32_t prefix_len) override;

    uint32_t getPort() const override;

    void setPort(uint16_t port) override;

protected:
    std::ostream& insert(std::ostream& os) const override;

private:
    sockaddr_in6 m_addr;
};

// class UnixAddress : public Address
// {
// public:
//     using _ptr = std::shared_ptr<UnixAddress>;

//     UnixAddress(const std::string& path);

//     const sockaddr* getAddr() const override;

//     socklen_t getAddrLen() const override;

// protected:
//     std::ostream& insert(std::ostream& os) const override;

// private:
//     sockaddr_in m_addr;
//     socklen_t m_addr_len;
// };

// class UnknowAddress : public Address
// {
// public:
//     using _ptr = std::shared_ptr<UnknowAddress>;

//     UnknowAddress(const std::string& path);

//     const sockaddr* getAddr() const override;

//     socklen_t getAddrLen() const override;

// protected:
//     std::ostream& insert(std::ostream& os) const override;

// private:
//     sockaddr_in m_addr;
//     socklen_t m_addr_len;
// };

}