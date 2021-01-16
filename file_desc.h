// SPDX-License-Identifier: MIT

#ifndef FILE_DESC_H
#define FILE_DESC_H

#include "conditions.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>

namespace plac {

struct FileDesc {
  FileDesc() = default;
  FileDesc(const char *name) : fd_{open(name, O_RDONLY, 0)} {}
  FileDesc(FileDesc &) = delete;
  FileDesc(FileDesc &&other) noexcept : fd_{other.fd_} { other.fd_ = -1; }
  FileDesc &operator=(FileDesc &) = delete;
  FileDesc &operator=(FileDesc &&other) noexcept {
    if (fd_ != other.fd_) {
      std::swap(fd_, other.fd_);
    }
    return *this;
  }
  ~FileDesc() noexcept {
    if (IsValid()) {
      EXPECTS(::close(fd_) == 0, "cannot close file descriptor");
    }
  }
  bool IsValid() const { return fd_ >= 0; }

  int fd_{-1};
};

} // namespace plac

#endif
