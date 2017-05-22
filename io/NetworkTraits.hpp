#pragma once

namespace io
{
namespace net
{
template<int V, typename SA, typename ADDR>
struct CarrierTraits {
    using type = CarrierTraits<V, SA, ADDR>;
    enum { value = V };
    using sockaddr_type = SA;
    using addr_type = ADDR;
    static constexpr std::size_t address_size() noexcept { return sizeof(SA); }
};

template<int V, int P>
struct ProtocolTraits {
    using type = ProtocolTraits<V, P>;
    enum { value = V, proto = P };
};

using ipv4 = CarrierTraits<PF_INET, struct sockaddr_in, struct in_addr>;
using ipv6 = CarrierTraits<PF_INET6, struct sockaddr_in6, struct in6_addr>;
using tcp = ProtocolTraits<SOCK_STREAM, IPPROTO_TCP>;
using udp = ProtocolTraits<SOCK_DGRAM, IPPROTO_UDP>;
}
}
