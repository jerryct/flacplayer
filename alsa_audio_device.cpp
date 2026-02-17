// SPDX-License-Identifier: MIT

#include "alsa_audio_device.h"
#include "conditions.h"
#include <cstdint>

namespace plac {

namespace {

void show_available_sample_formats(snd_pcm_t *handle_, snd_pcm_hw_params_t *params)
{
    fprintf(stderr, "Available formats:\n");
    for (int format = 0; format <= SND_PCM_FORMAT_LAST; format++) {
        if (snd_pcm_hw_params_test_format(handle_, params, static_cast<snd_pcm_format_t>(format))
            == 0) {
            fprintf(stderr, "- %s\n", snd_pcm_format_name(static_cast<snd_pcm_format_t>(format)));
        }
    }
}

struct Logger {
  Logger() : log{} {
    const int err = snd_output_stdio_attach(&log, stderr, 0);
    ENSURES(err >= 0, "cannot attach stdio: {}", snd_strerror(err));
  }
  Logger(Logger &) = delete;
  Logger(Logger &&other) = delete;
  Logger &operator=(Logger &) = delete;
  Logger &operator=(Logger &&other) = delete;
  ~Logger() { snd_output_close(log); }

  snd_output_t *log;
};

void Sleep(timespec &t) {
    t.tv_sec += 1;
    ::clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, nullptr);
}

void CopyAudio16Bit(const int *const left, const int *const right,
                    snd_pcm_uframes_t frames, uint8_t *data) {
    for (int i{0}; i < frames; ++i) {
        std::uint32_t interleaved{static_cast<std::uint32_t>(right[i])};
        interleaved <<= 16;
        interleaved |= static_cast<std::uint32_t>(left[i]) & 0xFFFF;
        __builtin_memcpy(data, &interleaved, 4);
        data += 4;
    }
}

void CopyAudio24Bit(const int *const left, const int *const right,
                    snd_pcm_uframes_t frames, uint8_t *data) {
    for (int i{0}; i < frames; ++i) {
        std::uint64_t interleaved{static_cast<std::uint32_t>(right[i])};
        interleaved <<= 24;
        interleaved |= static_cast<std::uint64_t>(left[i]) & 0xFFFFFF;
        __builtin_memcpy(data, &interleaved, 6);
        data += 6;
    }
}

// implements
// https://github.com/alsa-project/alsa-lib/blob/5f524e300409f0a7b54c5fe9ee194bad1fc39516/test/pcm.c#L600
// recovery is factored out to avoid repeating it in this function. instead
// function retruns
ssize_t Copy(snd_pcm_t *handle_, const AudioFormat format,
             const int *const left, const int *const right, const size_t count,
             timespec &timer) {
    const snd_pcm_sframes_t avail{snd_pcm_avail_update(handle_)};
    if (avail < 0) {
        return avail;
    }

    if (static_cast<size_t>(avail) < count) {
        if (timer.tv_sec == 0) {
            const int r{snd_pcm_start(handle_)};
            ENSURES(r == 0, "cannot start stream: {}", snd_strerror(r));
            ::clock_gettime(CLOCK_MONOTONIC, &timer);
            return 0L;
        } else {
            Sleep(timer);
            const snd_pcm_sframes_t r{snd_pcm_avail(handle_)};
            if (r < format.rate) {
                LOG_ERROR("low hardware buffer {}\n", r);
            }
            return std::min(r, 0L);
        }
    }

    const snd_pcm_channel_area_t *areas{nullptr};
    snd_pcm_uframes_t offset{};
    snd_pcm_uframes_t frames{count};
    const auto r = snd_pcm_mmap_begin(handle_, &areas, &offset, &frames);
    if (r != 0) {
        return r;
    }

    ENSURES(areas[0].first == 0, "");
    ENSURES(areas[0].step == format.bits * format.channels, "mismatch in step size");
    uint8_t *data = static_cast<uint8_t *>(areas[0].addr) + AsBytes(format, offset);

    if (format.bits == 16) {
        CopyAudio16Bit(left, right, frames, data);
    } else if (format.bits == 24) {
        CopyAudio24Bit(left, right, frames, data);
    }

    return snd_pcm_mmap_commit(handle_, offset, frames);
}

} // namespace

AlsaAudioDevice::AlsaAudioDevice(const Output out)
    : handle_{nullptr}, format_{}, params_{}, timer_{} {
    int open_mode = 0;
    open_mode |= SND_PCM_NO_AUTO_RESAMPLE;
    // open_mode |= SND_PCM_NO_AUTO_CHANNELS;
    // open_mode |= SND_PCM_NO_AUTO_FORMAT;
    open_mode |= SND_PCM_NO_SOFTVOL;
    // open_mode |= SND_PCM_NONBLOCK;

    const char *name{};
    switch (out) {
    case Output::file:
        name = {"write_file"};
        break;
    case Output::uln2:
        name = {"plug_uln2"};
        break;
    default:
        ENSURES(false, "unknown output");
        break;
    }
    const int err = snd_pcm_open(&handle_, name, SND_PCM_STREAM_PLAYBACK, open_mode);
    ENSURES(err >= 0, "audio open error: {}", snd_strerror(err));
}

AlsaAudioDevice::~AlsaAudioDevice() noexcept { snd_pcm_close(handle_); }

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

  err = snd_pcm_hw_params_set_access(handle_, params, SND_PCM_ACCESS_MMAP_INTERLEAVED);
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
  p.buffer_size = f.rate * 2; // 2s buffer
  const unsigned buffer_period_ratio{2};
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
  // not needed. api for read/write but not mmap
  err = snd_pcm_sw_params_set_stop_threshold(handle_, swparams, p.buffer_size);
  ENSURES(err >= 0, "cannot set stop threshold");
  // not needed. api for read/write but not mmap
  err = snd_pcm_sw_params_set_start_threshold(handle_, swparams, p.buffer_size);
  ENSURES(err >= 0, "cannot set start threshold");
  err = snd_pcm_sw_params_set_tstamp_mode(handle_, swparams, SND_PCM_TSTAMP_NONE);
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

void AlsaAudioDevice::Play(const int *left, size_t length, const int *right, AudioFormat format)
{
    while (length != 0) {
        ssize_t n = Copy(handle_, format, left, right, length, timer_);
        if (n < 0) {
            n = snd_pcm_recover(handle_, n, 0);
            ENSURES(n == 0, "write error: {}", snd_strerror(n));
            timer_ = {};
        }

        length -= n;
        left += n;
        right += n;
    }
}

void AlsaAudioDevice::Drain() {
    snd_pcm_nonblock(handle_, /* block= */ 0);
    snd_pcm_drain(handle_);
}

} // namespace plac
