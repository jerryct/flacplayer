// SPDX-License-Identifier: MIT

#include "core_audio_device.h"
#include "flow_control.h"
#include "stream.h"

int main(int argc, char *argv[]) {
  EXPECTS(argc > 1, "no file provided");

  ::plac::FlowControl flow{};
  ::plac::AudioBuffer<228000> audio_buffer{};
  ::plac::Stream stream{audio_buffer, flow};
  ::plac::CoreAudioDevice device{audio_buffer, flow};

  bool first{true};
  for (int i{1}; i < argc; ++i) {
    if (!stream.Reset(argv[i])) {
      continue;
    }

    if (first) {
      first = false;
      device.Init(stream.format_);
      stream.FillBuffer();
      device.Playback();
    }
    if (device.format_ != stream.format_) {
      LOG_ERROR("audio format mismatch");
      break;
    }

    stream.Decode();
  }

  return 0;
}
