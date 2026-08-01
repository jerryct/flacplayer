// SPDX-License-Identifier: MIT

#ifndef RING_H
#define RING_H

#include "liburing.h"
#include <array>

namespace plac {

struct Ring
{
    Ring();
    Ring(const Ring &other) = delete;
    Ring &operator =(const Ring &other) = delete;
    Ring(Ring &&other) = delete;
    Ring &operator =(Ring &&other) = delete;
    ~Ring() noexcept;

    void ReadFixed(int fd, char *buf, std::size_t size);

    io_uring_cqe *Wait();

    io_uring ring;
    __u64 offset{};

    std::array<char, 8192> buf;
};

} // namespace plac

#endif
