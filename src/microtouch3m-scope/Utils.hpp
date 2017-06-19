/*
 * microtouch3m-scope - Graphical tool for monitoring MicroTouch 3M touchscreen scope
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA.
 *
 * Copyright (C) 2017 Zodiac Inflight Innovations
 * Copyright (C) 2017 Sergey Zhuravlevich
 */

#ifndef MICROTOUCH3M_SCOPE_UTILS_HPP
#define MICROTOUCH3M_SCOPE_UTILS_HPP

#include <string>

#include <time.h>
#include <stdint.h>

namespace Utils {
    inline timespec timespec_diff(const timespec &start, const timespec &stop)
    {
        timespec ret;

        if ((stop.tv_nsec - start.tv_nsec) < 0)
        {
            ret.tv_sec = stop.tv_sec - start.tv_sec - 1;
            ret.tv_nsec = stop.tv_nsec - start.tv_nsec + 1000000000;
        }
        else
        {
            ret.tv_sec = stop.tv_sec - start.tv_sec;
            ret.tv_nsec = stop.tv_nsec - start.tv_nsec;
        }

        return ret;
    }

    std::string ipv4_string();
    std::string mac();
    int32_t closest_factor(int32_t of_number, int32_t to_number);
    unsigned int gcd(unsigned int u, unsigned int v);
};

#endif // MICROTOUCH3M_SCOPE_UTILS_HPP
