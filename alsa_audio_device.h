// SPDX-License-Identifier: MIT

#ifndef ALSA_AUDIO_DEVICE_H
#define ALSA_AUDIO_DEVICE_H

#include "audio_buffer.h"
#include "audio_format.h"
#include "conditions.h"
#include <alsa/asoundlib.h>

namespace plac {

struct Params {
  // distance between interrupts is # frames
  snd_pcm_uframes_t period_frames;
  // buffer duration is # frames
  snd_pcm_uframes_t buffer_frames;
};

enum class Status : int { run, drain };

struct AlsaAudioDevice {
  AlsaAudioDevice(AudioBuffer<228000> &audio_buffer)
      : handle_{nullptr}, format_{}, params_{}, audio_buffer_{audio_buffer} {
    int open_mode = 0;
    open_mode |= SND_PCM_NO_AUTO_RESAMPLE;
    // open_mode |= SND_PCM_NO_AUTO_CHANNELS;
    // open_mode |= SND_PCM_NO_AUTO_FORMAT;
    open_mode |= SND_PCM_NO_SOFTVOL;
    // open_mode |= SND_PCM_NONBLOCK;

    const int nonblock = 0;
    if (nonblock) {
      const int err = snd_pcm_nonblock(handle_, 1);
      ENSURES(err >= 0, "nonblock setting error: {}", snd_strerror(err));
    }

    const char *name{"plug_uln2"};
    const int err = snd_pcm_open(&handle_, name, SND_PCM_STREAM_PLAYBACK, open_mode);
    ENSURES(err >= 0, "audio open error: {}", snd_strerror(err));
  }
  AlsaAudioDevice(AlsaAudioDevice &) = delete;
  AlsaAudioDevice(AlsaAudioDevice &&) = delete;
  AlsaAudioDevice &operator=(AlsaAudioDevice &) = delete;
  AlsaAudioDevice &operator=(AlsaAudioDevice &&) = delete;
  ~AlsaAudioDevice() noexcept { snd_pcm_close(handle_); }

  void Init(const AudioFormat format, const int verbose);
  void Playback(std::atomic<Status> &status);

  snd_pcm_t *handle_;
  AudioFormat format_;
  Params params_;
  AudioBuffer<228000> &audio_buffer_;
};

} // namespace plac

#endif
