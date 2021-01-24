// SPDX-License-Identifier: MIT

#include "alsa_audio_device.h"

namespace plac {

namespace {

void show_available_sample_formats(snd_pcm_t *handle_, snd_pcm_hw_params_t *params) {
  fprintf(stderr, "Available formats:\n");
  for (int format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
    if (snd_pcm_hw_params_test_format(handle_, params, static_cast<snd_pcm_format_t>(format)) == 0) {
      fprintf(stderr, "- %s\n", snd_pcm_format_name(static_cast<snd_pcm_format_t>(format)));
    }
  }
}

struct Logger {
  Logger() : log{} {
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

void AlsaAudioDevice::Init(const AudioFormat f, const LogLevel log_level) {
  AudioFormat info = f;
  Logger log;

  int err{};

  snd_pcm_hw_params_t *params;
  snd_pcm_hw_params_alloca(&params);
  err = snd_pcm_hw_params_any(handle_, params);
  ENSURES(err >= 0, "broken configuration for this PCM: no configurations available");

  if (LogLevel::verbose == log_level) {
    fprintf(stderr, "HW Params of device \"%s\":\n", snd_pcm_name(handle_));
    fprintf(stderr, "--------------------\n");
    snd_pcm_hw_params_dump(params, log.log);
    fprintf(stderr, "--------------------\n");
  }

  err = snd_pcm_hw_params_set_access(handle_, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  ENSURES(err >= 0, "access type not available");

  snd_pcm_format_t format{};
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

  Params p{};
  p.buffer_size = AsFrames(f, 228000);
  const unsigned buffer_period_ratio{4};
  p.period_size = p.buffer_size / buffer_period_ratio;

  err = snd_pcm_hw_params_set_buffer_size(handle_, params, p.buffer_size);
  ENSURES(err >= 0, "cannot set buffer size");
  err = snd_pcm_hw_params_set_period_size(handle_, params, p.period_size, 0);
  ENSURES(err >= 0, "cannot set period size");
  err = snd_pcm_hw_params_set_periods(handle_, params, buffer_period_ratio, 0);
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

  err = snd_pcm_sw_params_set_avail_min(handle_, swparams, p.period_size);
  ENSURES(err >= 0, "cannot set avail min");
  err = snd_pcm_sw_params_set_stop_threshold(handle_, swparams, p.buffer_size);
  ENSURES(err >= 0, "cannot set stop threshold");
  err = snd_pcm_sw_params_set_start_threshold(handle_, swparams, p.buffer_size);
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

  if (LogLevel::verbose == log_level) {
    snd_pcm_dump(handle_, log.log);
  }

  format_.bits = snd_pcm_format_physical_width(format);
  snd_pcm_hw_params_get_channels(params, &format_.channels);
  snd_pcm_hw_params_get_rate(params, &format_.rate, nullptr);

  EXPECTS(info == f, "");
  EXPECTS(format_ == f, "");

  snd_pcm_hw_params_get_period_size(params, &params_.period_size, nullptr);
  EXPECTS(p.period_size == params_.period_size, "");
  snd_pcm_hw_params_get_buffer_size(params, &params_.buffer_size);
  EXPECTS(p.buffer_size == params_.buffer_size, "");
}

void AlsaAudioDevice::Playback(std::atomic<Status> &status) {
  while (status == Status::run) {
    auto writer = [this](AudioFormat, const u_char *const data, const size_t count) {
      auto n = snd_pcm_writei(handle_, data, count);
      if ((n == -EAGAIN) || ((n >= 0) && (static_cast<size_t>(n) < count))) {
        snd_pcm_wait(handle_, 100);
      } else if (n < 0) {
        n = snd_pcm_recover(handle_, n, 0);
        ENSURES(n >= 0, "write error: {}", snd_strerror(n));
      }

      return n;
    };

    audio_buffer_.Read(format_, params_.period_size, writer);
    flow_.Notify();
  }

  if (status == Status::drain) {
    auto writer = [this](AudioFormat, const u_char *const data, const size_t count) -> snd_pcm_sframes_t {
      if (count < params_.period_size) {
        return -EINVAL;
      }
      return snd_pcm_writei(handle_, data, params_.period_size);
    };
    audio_buffer_.Drain(format_, writer);
    snd_pcm_nonblock(handle_, /* block= */ 0);
    snd_pcm_drain(handle_);
  } else {
    snd_pcm_drop(handle_);
  }
}

} // namespace plac
