// SPDX-License-Identifier: MIT

#include "alsa_audio_device.h"

namespace plac {

namespace {

void show_available_sample_formats(snd_pcm_t *handle_, snd_pcm_hw_params_t *params) {
  fprintf(stderr, "Available formats:\n");
  for (int format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
    if (snd_pcm_hw_params_test_format(handle_, params, static_cast<snd_pcm_format_t>(format)) == 0)
      fprintf(stderr, "- %s\n", snd_pcm_format_name(static_cast<snd_pcm_format_t>(format)));
  }
}

struct Logger {
  Logger() {
    int err = snd_output_stdio_attach(&log, stderr, 0);
    ENSURES(err >= 0, "cannot attach stdio: {}", snd_strerror(err));
  }
  Logger(Logger &) = delete;
  Logger(Logger &&other) = delete;
  Logger &operator=(Logger &) = delete;
  Logger &operator=(Logger &&other) = delete;
  ~Logger() { snd_output_close(log); }

  snd_output_t *log;
};

} // namespace

void AlsaAudioDevice::Init(const AudioFormat f, const int verbose) {
  Params p;
  p.buffer_frames = AsFrames(f, 228000);
  unsigned buffer_period_ratio{4};
  p.period_frames = p.buffer_frames / buffer_period_ratio;
  AudioFormat info = f;
  Logger log;

  int err;

  snd_pcm_hw_params_t *params;
  snd_pcm_hw_params_alloca(&params);
  err = snd_pcm_hw_params_any(handle_, params);
  ENSURES(err >= 0, "broken configuration for this PCM: no configurations available");

  if (verbose) {
    fprintf(stderr, "HW Params of device \"%s\":\n", snd_pcm_name(handle_));
    fprintf(stderr, "--------------------\n");
    snd_pcm_hw_params_dump(params, log.log);
    fprintf(stderr, "--------------------\n");
  }

  err = snd_pcm_hw_params_set_access(handle_, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  ENSURES(err >= 0, "access type not available");

  snd_pcm_format_t format;
  if (info.bits == 16) {
    format = SND_PCM_FORMAT_S16_LE;
  } else if (info.bits == 24) {
    format = SND_PCM_FORMAT_S24_3LE;
  } else {
    ENSURES(false, "only 16bit and 24bit supported");
  }
  err = snd_pcm_hw_params_set_format(handle_, params, format);
  if (err < 0) {
    LOG_ERROR("sample format not available");
    show_available_sample_formats(handle_, params);
    exit(EXIT_FAILURE);
  }
  err = snd_pcm_hw_params_set_channels(handle_, params, info.channels);
  ENSURES(err >= 0, "channels count not available");

  unsigned int rate = info.rate;
  err = snd_pcm_hw_params_set_rate_resample(handle_, params, 0);
  ENSURES(err == 0, "cannot set resample rate");
  err = snd_pcm_hw_params_set_rate_near(handle_, params, &info.rate, nullptr);
  ENSURES(err >= 0, "cannot set rate near");
  ENSURES(rate == info.rate, "rate modified");

  err = snd_pcm_hw_params_set_buffer_size(handle_, params, p.buffer_frames);
  ENSURES(err >= 0, "cannot set buffer size");
  err = snd_pcm_hw_params_set_period_size(handle_, params, p.period_frames, 0);
  ENSURES(err >= 0, "cannot set period size");
  err = snd_pcm_hw_params_set_periods_near(handle_, params, &buffer_period_ratio, 0);
  ENSURES(err >= 0, "cannot set buffer/period ratio");

  err = snd_pcm_hw_params(handle_, params);
  if (err < 0) {
    LOG_ERROR("unable to install hw params:");
    snd_pcm_hw_params_dump(params, log.log);
    exit(EXIT_FAILURE);
  }

  snd_pcm_sw_params_t *swparams;
  snd_pcm_sw_params_alloca(&swparams);
  err = snd_pcm_sw_params_current(handle_, swparams);
  ENSURES(err >= 0, "unable to get current sw params");

  err = snd_pcm_sw_params_set_avail_min(handle_, swparams, p.period_frames);
  ENSURES(err >= 0, "cannot set avail min");
  // stop threshold not disabled to stop playback in case of underrun
  err = snd_pcm_sw_params_set_stop_threshold(handle_, swparams, p.buffer_frames);
  ENSURES(err >= 0, "cannot set stop threshold");
  // start threshold disabled to avoid automatic start
  err = snd_pcm_sw_params_set_start_threshold(handle_, swparams, std::numeric_limits<snd_pcm_uframes_t>::max());
  ENSURES(err >= 0, "cannot set start threshold");
  err = snd_pcm_sw_params_set_tstamp_mode(handle_, swparams, SND_PCM_TSTAMP_ENABLE);
  ENSURES(err >= 0, "cannot set tstamp mode");
  err = snd_pcm_sw_params_set_tstamp_type(handle_, swparams, SND_PCM_TSTAMP_TYPE_MONOTONIC_RAW);
  ENSURES(err >= 0, "cannot set tstamp mode");

  if (snd_pcm_sw_params(handle_, swparams) < 0) {
    LOG_ERROR("unable to install sw params:");
    snd_pcm_sw_params_dump(swparams, log.log);
    exit(EXIT_FAILURE);
  }

  if (verbose) {
    snd_pcm_dump(handle_, log.log);
  }

  format_.bits = snd_pcm_format_physical_width(format);
  snd_pcm_hw_params_get_channels(params, &format_.channels);
  snd_pcm_hw_params_get_rate(params, &format_.rate, nullptr);

  EXPECTS(info == f, "");
  EXPECTS(format_ == f, "");

  snd_pcm_hw_params_get_period_size(params, &params_.period_frames, 0);
  EXPECTS(p.period_frames == params_.period_frames, "");
  snd_pcm_hw_params_get_buffer_size(params, &params_.buffer_frames);
  EXPECTS(p.buffer_frames == params_.buffer_frames, "");
}

namespace {
// https://git.alsa-project.org/?p=alsa-lib.git;a=blob;f=src/pcm/pcm.c;h=bc18954b92da124bafd3a67913bd3c8900dd012f;hb=HEAD#l7864
ssize_t pcm_write_flac(snd_pcm_t *handle, AudioBuffer<228000> &audio_buffer, AudioFormat format, size_t count) {
  auto writer = [handle](AudioFormat format, u_char *data, size_t count) {
    auto n = snd_pcm_writei(handle, data, count);
    if ((n == -EAGAIN) || ((n >= 0) && (static_cast<size_t>(n) < count))) {
      snd_pcm_wait(handle, 100);
    } else if (n < 0) {
      n = snd_pcm_recover(handle, n, 0);
      ENSURES(n >= 0, "write error: {}", snd_strerror(n));
    }

    return n;
  };

  ssize_t result = 0;
  while (count > 0) {
    const ssize_t n = audio_buffer.Read(format, count, writer);
    if (n >= 0) {
      result += n;
      count -= n;
    }
  }

  return result;
}
} // namespace

void AlsaAudioDevice::Playback(std::atomic<Status> &status) {
  pcm_write_flac(handle_, audio_buffer_, format_, params_.buffer_frames);

  while (status == Status::run) {
    pcm_write_flac(handle_, audio_buffer_, format_, params_.period_frames);
  }

  if (status == Status::drain) {
    auto writer = [this](AudioFormat, u_char *data, size_t count) { return snd_pcm_writei(handle_, data, count); };
    audio_buffer_.Drain(format_, writer);
    snd_pcm_nonblock(handle_, 0);
    snd_pcm_drain(handle_);
    // snd_pcm_nonblock(handle_, nonblock);
  } else {
    snd_pcm_drop(handle_);
  }
}

} // namespace plac
