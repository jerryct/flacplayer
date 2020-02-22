// SPDX-License-Identifier: MIT

#include "alsa_audio_device.h"
#include "stream.h"
#include <thread>

int main(int argc, char *argv[]) {
  EXPECTS(argc > 1, "no file provided");

  ::plac::AudioBuffer<228000> audio_buffer;
  ::plac::AlsaAudioDevice device{audio_buffer};
  ::plac::Stream stream{audio_buffer};
  std::atomic<::plac::Status> status{::plac::Status::run};
  std::thread audio{};

  bool first{true};
  while (optind <= (argc - 1)) {
    if (!stream.Reset(argv[optind++])) {
      continue;
    }
    if (first) {
      first = false;
      device.Init(stream.format_, 0);
      stream.FillBuffer();
      audio = std::thread{&::plac::AlsaAudioDevice::Playback, std::ref(device), std::ref(status)};
    }
    if (device.format_ != stream.format_) {
      LOG_ERROR("audio format mismatch");
      break;
    }

    stream.Decode();
  }

  if (audio.joinable()) {
    status = ::plac::Status::drain;
    audio.join();
  }

  return 0;
}
