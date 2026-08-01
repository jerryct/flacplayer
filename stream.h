// SPDX-License-Identifier: MIT

#ifndef STREAM_H
#define STREAM_H

#include "alsa_audio_device.h"
#include "audio_format.h"
#include "file_desc.h"
#include "ring.h"
#include <FLAC/stream_decoder.h>

namespace plac {

struct Stream {
  Stream(const AlsaAudioDevice::Output out);
  Stream(const Stream &) = delete;
  Stream(Stream &&) = delete;
  Stream &operator=(const Stream &) = delete;
  Stream &operator=(Stream &&) = delete;
  ~Stream() noexcept;

  bool Reset(const char *name);
  void Decode();

  FLAC__StreamDecoder *decoder_;
  FileDesc desc_;
  AudioFormat format_;

  ::plac::AlsaAudioDevice device_;

  Ring ring;
};

} // namespace plac

#endif
