// SPDX-License-Identifier: MIT

#ifndef STREAM_H
#define STREAM_H

#include "audio_buffer.h"
#include "audio_format.h"
#include "file_desc.h"
#include <FLAC/stream_decoder.h>

namespace plac {

struct Stream {
  Stream(AudioBuffer<228000> &audio_buffer);
  Stream(Stream &) = delete;
  Stream(Stream &&) = delete;
  Stream &operator=(Stream &) = delete;
  Stream &operator=(Stream &&) = delete;
  ~Stream() noexcept {
    FLAC__stream_decoder_finish(decoder_);
    FLAC__stream_decoder_delete(decoder_);
  }

  bool Reset(const char *name) {
    if (name == nullptr) {
      LOG_ERROR("nullptr provided");
      return false;
    }

    FLAC__stream_decoder_reset(decoder_);
    desc_ = FileDesc{name};
    if (!desc_.IsValid()) {
      LOG_ERROR("invalid file: {}", name);
      return false;
    }
    const FLAC__bool ret{FLAC__stream_decoder_process_until_end_of_metadata(decoder_)};
    return ret != 0;
  }

  void Decode();
  void FillBuffer();

  FLAC__StreamDecoder *decoder_;
  FileDesc desc_;
  AudioFormat format_;
  AudioBuffer<228000> &audio_buffer_;
};

} // namespace plac

#endif
