// SPDX-License-Identifier: MIT

#ifndef ALSA_AUDIO_DEVICE_H
#define ALSA_AUDIO_DEVICE_H

#include "audio_buffer.h"
#include "audio_format.h"
#include "conditions.h"
#include "flow_control.h"
#include <alsa/asoundlib.h>

namespace plac {

struct Params {
  // wakeup interval in # frames
  //
  //  - The latency is defined by the buffer size.
  //  - The wakeup interval is defined by the fragment size.
  //
  //  The buffer fill level will oscillate between 'full buffer' and 'full buffer minus 1x fragment size minus OS
  //  scheduling latency'. Setting smaller fragment sizes will increase the CPU load and decrease battery time since
  //  you force the CPU to wake up more often. OTOH it increases drop out safety, since you fill up playback buffer
  //  earlier. Choosing the fragment size is hence something which you should do balancing out your needs between
  //  power consumption and drop-out safety. With modern processors and a good OS scheduler like the Linux one setting
  //  the fragment size to anything other than half the buffer size does not make much sense.
  //
  //  ... (Oh, ALSA uses the term 'period' for what I call 'fragment' above. It's synonymous)
  //
  // From: https://stackoverflow.com/questions/24040672/the-meaning-of-period-in-alsa/24049739#24049739
  snd_pcm_uframes_t period_size;
  // buffer size in # frames
  snd_pcm_uframes_t buffer_size;
};

enum class Status : int { run, drain };

struct AlsaAudioDevice {
  AlsaAudioDevice(AudioBuffer<228000> &audio_buffer, FlowControl &flow)
      : handle_{nullptr}, format_{}, params_{}, audio_buffer_{audio_buffer}, flow_{flow} {
    int open_mode = 0;
    open_mode |= SND_PCM_NO_AUTO_RESAMPLE;
    // open_mode |= SND_PCM_NO_AUTO_CHANNELS;
    // open_mode |= SND_PCM_NO_AUTO_FORMAT;
    open_mode |= SND_PCM_NO_SOFTVOL;
    // open_mode |= SND_PCM_NONBLOCK;

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
  FlowControl &flow_;
};

} // namespace plac

#endif
