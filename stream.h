// SPDX-License-Identifier: MIT

#ifndef STREAM_H
#define STREAM_H

#include "audio_buffer.h"
#include "audio_format.h"
#include "file_desc.h"
#include "flow_control.h"
#include <FLAC/stream_decoder.h>

namespace plac {

struct Stream {
  Stream(AudioBuffer<228000> &audio_buffer, FlowControl &flow);
  Stream(Stream &) = delete;
  Stream(Stream &&) = delete;
  Stream &operator=(Stream &) = delete;
  Stream &operator=(Stream &&) = delete;
  ~Stream() noexcept;

  bool Reset(const char *name);
  void Decode();
  void FillBuffer();

  FLAC__StreamDecoder *decoder_;
  FileDesc desc_;
  AudioFormat format_;
  AudioBuffer<228000> &audio_buffer_;
  FlowControl &flow_;
};

} // namespace plac

#endif
