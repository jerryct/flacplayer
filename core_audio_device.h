// SPDX-License-Identifier: MIT

#ifndef CORE_AUDIO_DEVICE_H
#define CORE_AUDIO_DEVICE_H

#include "audio_buffer.h"
#include "audio_format.h"
#include <CoreAudio/AudioHardware.h>

namespace plac {

struct CoreAudioDevice {
  CoreAudioDevice(AudioBuffer<228000> &audio_buffer);
  CoreAudioDevice(CoreAudioDevice &) = delete;
  CoreAudioDevice(CoreAudioDevice &&) = delete;
  CoreAudioDevice &operator=(CoreAudioDevice &) = delete;
  CoreAudioDevice &operator=(CoreAudioDevice &&) = delete;
  ~CoreAudioDevice();

  void Init(const AudioFormat format);
  void Playback();

  AudioObjectID uln2_;
  AudioDeviceIOProcID proc_id_;
  AudioFormat format_;
  AudioBuffer<228000> &audio_buffer_;
};

} // namespace plac

#endif
