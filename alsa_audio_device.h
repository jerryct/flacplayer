// SPDX-License-Identifier: MIT

#ifndef ALSA_AUDIO_DEVICE_H
#define ALSA_AUDIO_DEVICE_H

#include "audio_format.h"
#include <alsa/asoundlib.h>

namespace plac {

struct AlsaAudioDevice {
  struct Params {
    // wakeup interval in # frames
    //
    //  - The latency is defined by the buffer size.
    //  - The wakeup interval is defined by the fragment size.
    //
    //  The buffer fill level will oscillate between 'full buffer' and 'full
    //  buffer minus 1x fragment size minus OS scheduling latency'. Setting
    //  smaller fragment sizes will increase the CPU load and decrease battery
    //  time since you force the CPU to wake up more often. OTOH it increases
    //  drop out safety, since you fill up playback buffer earlier. Choosing the
    //  fragment size is hence something which you should do balancing out your
    //  needs between power consumption and drop-out safety. With modern
    //  processors and a good OS scheduler like the Linux one setting the
    //  fragment size to anything other than half the buffer size does not make
    //  much sense.
    //
    //  ... (Oh, ALSA uses the term 'period' for what I call 'fragment' above.
    //  It's synonymous)
    //
    // From:
    // https://stackoverflow.com/questions/24040672/the-meaning-of-period-in-alsa/24049739#24049739
    snd_pcm_uframes_t period_size;
    // buffer size in # frames
    snd_pcm_uframes_t buffer_size;
  };

  enum class LogLevel { verbose, non_verbose };
  enum class Output { file, uln2 };

  AlsaAudioDevice(const Output out);
  AlsaAudioDevice(const AlsaAudioDevice &) = delete;
  AlsaAudioDevice(AlsaAudioDevice &&) = delete;
  AlsaAudioDevice &operator=(const AlsaAudioDevice &) = delete;
  AlsaAudioDevice &operator=(AlsaAudioDevice &&) = delete;
  ~AlsaAudioDevice() noexcept;

  void Init(const AudioFormat format, const LogLevel log_level);
  void Play(const int *left, size_t length, const int *right, AudioFormat format);
  void Drain();

  snd_pcm_t *handle_;
  AudioFormat format_;
  Params params_;

  timespec timer_;
};

} // namespace plac

#endif
