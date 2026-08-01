// SPDX-License-Identifier: MIT

#include "ring.h"
#include "conditions.h"

#include <cctype>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace plac {

Ring::Ring()
{
    int ret = io_uring_queue_init(1, &ring, 0);
    if (ret != 0) {
        fprintf(stderr, "Unable to setup io_uring: %s\n", strerror(-ret));
    }

    iovec iov[1];
    iov[0].iov_base = buf.data();
    iov[0].iov_len = buf.size();
    int ret2 = io_uring_register_buffers(&ring, iov, 1);
    if (ret2 != 0) {
        fprintf(stderr, "Error registering buffers: %s", strerror(-ret));
    }
}

Ring::~Ring() noexcept
{
    io_uring_unregister_buffers(&ring);
    io_uring_queue_exit(&ring);
}

void Ring::ReadFixed(int fd, char *buf, std::size_t size)
{
    io_uring_sqe *sqe = io_uring_get_sqe(&ring);
    if (sqe == nullptr) {
        fprintf(stderr, "Could not get SQE.\n");
    }
    EXPECTS(sqe != nullptr, "Could not get SQE");
    //io_uring_prep_read(sqe, 0, buf, size, offset);
    io_uring_prep_read_fixed(sqe, 0, buf, size, offset, 0);
    io_uring_sqe_set_flags(sqe, IOSQE_FIXED_FILE);

    int ret = io_uring_submit(&ring);
    if (ret != 1) { // ret is number of submitted requests
        fprintf(stderr, "Unable to submit: %s\n", strerror(-ret));
    }

    //io_uring_sqe_set_data(sqe, sqe);
}

io_uring_cqe *Ring::Wait()
{
    struct io_uring_cqe *cqe;
    if (io_uring_peek_cqe(&ring, &cqe) != 0) {
        perror("io_uring_wait_cqe");
        return nullptr;
    }

    if (cqe->res < 0) {
        fprintf(stderr, "read failed: %d\n", cqe->res);
    } else {
        //printf("read %d bytes\n", cqe->res);
    }

    //io_uring_cqe_seen(&ring, cqe);

    return cqe;
}

} // namespace plac
