#include "address.h"
#include <sstream>
#include <cstring>

namespace hxk
{

int Address::getFamily() const
{
    return getAddr()->sa_family;
}


std::string Address::toString()
{
    thread_local static std::stringstream ss;
    insert(ss);
    return ss.str();
}

bool Address::operator<(const Address& rhs) const
{
    socklen_t min_len = std::min(getAddrLen(), rhs.getAddrLen());
    int result = memcmp(getAddr(), rhs.getAddr(), min_len);
    if(result != 0) {
        return result < 0;
    }
    else {
        return getAddrLen() < rhs.getAddrLen();
    }
}

bool Address::operator==(const Address& rhs) const
{
    return getAddrLen() == rhs.getAddrLen() && memcmp(getAddr(), rhs.getAddr(), getAddrLen()) == 0;
}

bool Address::operator!=(const Address& rhs) const
{
    return !(*this == rhs);
}
}