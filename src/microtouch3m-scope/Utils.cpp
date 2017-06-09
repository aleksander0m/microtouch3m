#include "Utils.hpp"

#include <iostream>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <iomanip>

#if !defined _GNU_SOURCE
# define _GNU_SOURCE     /* To get defns of NI_MAXSERV and NI_MAXHOST */
#endif

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <string.h>
#include <linux/if_link.h>
#include <unistd.h>
#include <net/if.h>
#include <netpacket/packet.h>

std::string Utils::ipv4_string()
{
    std::string ret;

    struct ifaddrs *ifaddr;

    if (getifaddrs(&ifaddr) == -1) {
        std::cerr << "Error retrieving interface addresses" << std::endl;
        goto exit;
    }

    for (struct ifaddrs *it = ifaddr; it != 0; it = it->ifa_next) {
        if (!it->ifa_addr || it->ifa_addr->sa_family != AF_INET)
            continue;

        if (!it->ifa_name || !(it->ifa_flags & IFF_BROADCAST))
            continue;

        char host[NI_MAXHOST];
        int s;

        s = getnameinfo(it->ifa_addr,
                        sizeof(struct sockaddr_in),
                        host, NI_MAXHOST,
                        0, 0, NI_NUMERICHOST);

        if (s != 0)
        {
            std::cerr << "Retrieving interface " << it->ifa_name << " info failed: "
                      << gai_strerror(s) << std::endl;
            goto exit;
        }

        ret = host;
        goto exit;
    }

exit:
    freeifaddrs(ifaddr);

    return ret;
}

std::string Utils::mac()
{
    std::string ret;

    struct ifaddrs *ifaddr;

    if (getifaddrs(&ifaddr) == -1) {
        std::cerr << "Error retrieving interface addresses" << std::endl;
        goto exit;
    }

    for (struct ifaddrs *it = ifaddr; it != 0; it = it->ifa_next) {
        if (!it->ifa_addr || it->ifa_addr->sa_family != AF_PACKET)
            continue;

        if (!it->ifa_name || !(it->ifa_flags & IFF_BROADCAST))
            continue;

        std::ostringstream oss;

        oss << std::hex << std::setfill('0');

        struct sockaddr_ll *s = (struct sockaddr_ll *) it->ifa_addr;

        for (int i = 0; i < s->sll_halen; ++i)
        {
            oss << std::setw(2) << (int) s->sll_addr[i];

            if (i != s->sll_halen - 1)
                oss << ':';
        }

        ret = oss.str();
        goto exit;
    }

exit:
    freeifaddrs(ifaddr);

    return ret;
}

int32_t Utils::closest_factor(int32_t of_number, int32_t to_number)
{
    int32_t dist = to_number;
    int32_t closest = 1;

    for (int32_t i = 1; i <= of_number; ++i)
    {
        if (of_number % i == 0)
        {
            int32_t d = (int32_t) std::fabs(to_number - i);

            if (d < dist)
            {
                closest = i;
                dist = d;

                if (d == 0 || i > to_number) break;
            }
        }
    }

    return closest;
}

unsigned int Utils::gcd(unsigned int u, unsigned int v)
{
    int shift;

    /* GCD(0,v) == v; GCD(u,0) == u, GCD(0,0) == 0 */
    if (u == 0) return v;
    if (v == 0) return u;

    /* Let shift := lg K, where K is the greatest power of 2
          dividing both u and v. */
    for (shift = 0; ((u | v) & 1) == 0; ++shift) {
        u >>= 1;
        v >>= 1;
    }

    while ((u & 1) == 0)
        u >>= 1;

    /* From here on, u is always odd. */
    do {
        /* remove all factors of 2 in v -- they are not common */
        /*   note: v is not zero, so while will terminate */
        while ((v & 1) == 0)  /* Loop X */
            v >>= 1;

        /* Now u and v are both odd. Swap if necessary so u <= v,
           then set v = v - u (which is even). For bignums, the
           swapping is just pointer movement, and the subtraction
           can be done in-place. */
        if (u > v) {
            unsigned int t = v; v = u; u = t;}  // Swap u and v.
        v = v - u;                       // Here v >= u.
    } while (v != 0);

    /* restore common factors of 2 */
    return u << shift;
}
