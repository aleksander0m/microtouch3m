#include "Utils.hpp"

#include <iostream>

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

        if (!it->ifa_name | !(it->ifa_flags & IFF_BROADCAST))
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
