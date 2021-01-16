// SPDX-License-Identifier: MIT

#include "core_audio_device.h"
#include "conditions.h"
#include <iostream>

// Implementation based on:
// https://developer.apple.com/library/archive/technotes/tn2091/_index.html

namespace plac {

namespace {

OSStatus Raw16(const AudioObjectID /*unused*/, const AudioTimeStamp * /*unused*/, const AudioBufferList * /*unused*/,
               const AudioTimeStamp * /*unused*/, AudioBufferList *output, const AudioTimeStamp * /*unused*/,
               void *__nullable client_data) {
  CoreAudioDevice *const dec = static_cast<CoreAudioDevice *>(client_data);
  u_char *out = static_cast<u_char *>(output->mBuffers[0].mData);

  auto writer = [&out](AudioFormat /*unused*/, const u_char *const data, const size_t count) {
    for (size_t i = 0; i < count; ++i) {
      out[1] = data[4 * i + 0];
      out[2] = data[4 * i + 1];
      out[4] = data[4 * i + 2];
      out[5] = data[4 * i + 3];
      out += 12;
    }
    return count;
  };

  AudioFormat physical_format{};
  physical_format.bits = 24;
  physical_format.channels = 4;

  auto count = AsFrames(physical_format, output->mBuffers[0].mDataByteSize);
  while (count > 0) {
    const ssize_t n = dec->audio_buffer_.Read(dec->format_, count, writer);
    if (n >= 0) {
      count -= n;
    }
  }

  dec->flow_.Notify();
  return noErr;
}

OSStatus Raw24(const AudioObjectID /*unused*/, const AudioTimeStamp * /*unused*/, const AudioBufferList * /*unused*/,
               const AudioTimeStamp * /*unused*/, AudioBufferList *output, const AudioTimeStamp * /*unused*/,
               void *__nullable client_data) {
  CoreAudioDevice *const dec = static_cast<CoreAudioDevice *>(client_data);
  u_char *out = static_cast<u_char *>(output->mBuffers[0].mData);

  auto writer = [&out](AudioFormat /*unused*/, const u_char *const data, const size_t count) {
    for (size_t i = 0; i < count; ++i) {
      out[0] = data[6 * i + 0];
      out[1] = data[6 * i + 1];
      out[2] = data[6 * i + 2];
      out[3] = data[6 * i + 3];
      out[4] = data[6 * i + 4];
      out[5] = data[6 * i + 5];
      out += 12;
    }
    return count;
  };

  AudioFormat physical_format{};
  physical_format.bits = 24;
  physical_format.channels = 4;

  auto count = AsFrames(physical_format, output->mBuffers[0].mDataByteSize);
  while (count > 0) {
    const ssize_t n = dec->audio_buffer_.Read(dec->format_, count, writer);
    if (n >= 0) {
      count -= n;
    }
  }

  dec->flow_.Notify();
  return noErr;
}

std::vector<AudioObjectID> GetAudioDevices() {
  OSStatus err = noErr;

  AudioObjectPropertyAddress address{};
  address.mSelector = kAudioHardwarePropertyDevices;
  address.mScope = kAudioObjectPropertyScopeOutput;
  address.mElement = kAudioObjectPropertyElementMaster;

  UInt32 size;
  err = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, nullptr, &size);
  ENSURES(err == noErr, "can't get number of audio devices");

  std::vector<AudioObjectID> ids;
  ids.resize(size / sizeof(AudioObjectID));
  static_assert(std::is_integral<decltype(ids)::value_type>::value, "NO");

  err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size, ids.data());
  ENSURES(err == noErr, "can't get audio device ids");

  return ids;
}

struct StringRef {
  StringRef() : ref{nullptr} {}
  StringRef(CFStringRef _ref) : ref{_ref} {}
  StringRef(StringRef &) = delete;
  StringRef(StringRef &&other) noexcept : ref{other.ref} { other.ref = nullptr; }
  StringRef &operator=(StringRef &) = delete;
  StringRef &operator=(StringRef &&) = delete;
  ~StringRef() noexcept {
    if (ref != nullptr) {
      CFRelease(ref);
    }
  }
  CFStringRef ref;
};

bool operator==(const StringRef &lhs, const StringRef &rhs) {
  return CFStringCompare(lhs.ref, rhs.ref, 0) == kCFCompareEqualTo;
}

StringRef GetDeviceName(const AudioObjectID id) {
  AudioObjectPropertyAddress address{};
  address.mSelector = kAudioObjectPropertyName;
  address.mScope = kAudioObjectPropertyScopeOutput;
  address.mElement = kAudioObjectPropertyElementMaster;

  CFStringRef raw{nullptr};
  UInt32 size{sizeof(raw)};
  static_assert(std::is_pod<decltype(raw)>::value, "NO");
  const OSStatus err = AudioObjectGetPropertyData(id, &address, 0, nullptr, &size, &raw);
  StringRef name{raw};
  ENSURES(err == noErr, "can't get audio device name");

  return name;
}

void SetHogMode(const AudioObjectID id) {
  AudioObjectPropertyAddress address{};
  address.mSelector = kAudioDevicePropertyHogMode;
  address.mScope = kAudioObjectPropertyScopeOutput;
  address.mElement = kAudioObjectPropertyElementMaster;

  const pid_t p = getpid();
  const OSStatus err = AudioObjectSetPropertyData(id, &address, 0, nullptr, sizeof(p), &p);
  ENSURES(err == noErr, "setting hog-mode failed");
}

void SetFrameSize(const AudioObjectID id) {
  AudioObjectPropertyAddress address{};
  address.mSelector = kAudioDevicePropertyBufferFrameSize;
  address.mScope = kAudioObjectPropertyScopeOutput;
  address.mElement = kAudioObjectPropertyElementMaster;

  const UInt32 f = 1024;
  const OSStatus err = AudioObjectSetPropertyData(id, &address, 0, nullptr, sizeof(f), &f);
  ENSURES(err == noErr, "setting buffer frame size failed");
}

std::string PrintAvailableFormats(const AudioObjectID id) {
  OSStatus err = noErr;

  AudioObjectPropertyAddress address{};
  address.mSelector = kAudioStreamPropertyAvailablePhysicalFormats;
  address.mScope = kAudioObjectPropertyScopeOutput;
  address.mElement = kAudioObjectPropertyElementMaster;

  UInt32 size;
  err = AudioObjectGetPropertyDataSize(id, &address, 0, nullptr, &size);
  ENSURES(err == noErr, "can't get number of audio devices");

  std::vector<AudioStreamRangedDescription> des;
  des.resize(size / sizeof(AudioStreamRangedDescription));

  err = AudioObjectGetPropertyData(id, &address, 0, nullptr, &size, des.data());
  ENSURES(err == noErr, "failed");

  for (auto desc : des) {
    std::cout << "Format: " << desc.mFormat.mSampleRate << ", " << desc.mFormat.mFormatID << ", "
              << desc.mFormat.mFormatFlags << ", " << desc.mFormat.mBitsPerChannel << ", "
              << desc.mFormat.mChannelsPerFrame << ", " << desc.mFormat.mBytesPerFrame << ", "
              << desc.mFormat.mFramesPerPacket << ", " << desc.mFormat.mBytesPerPacket << std::endl;
  }

  return {};
}

bool IsEqual(const AudioStreamBasicDescription lhs, const AudioStreamBasicDescription rhs) {
  return (lhs.mSampleRate == rhs.mSampleRate) &&             //
         (lhs.mFormatID == rhs.mFormatID) &&                 //
         (lhs.mFormatFlags == rhs.mFormatFlags) &&           //
         (lhs.mBitsPerChannel == rhs.mBitsPerChannel) &&     //
         (lhs.mChannelsPerFrame == rhs.mChannelsPerFrame) && //
         (lhs.mBytesPerFrame == rhs.mBytesPerFrame) &&       //
         (lhs.mFramesPerPacket == rhs.mFramesPerPacket) &&   //
         (lhs.mBytesPerPacket == rhs.mBytesPerPacket);
}

AudioFormat SetStreamFormat(const AudioObjectID id, const AudioFormat format) {
  OSStatus err = noErr;

  AudioStreamBasicDescription desc{};
  desc.mSampleRate = format.rate;
  desc.mFormatID = kAudioFormatLinearPCM;
  desc.mFormatFlags = kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked | kAudioFormatFlagIsNonMixable;
  desc.mBitsPerChannel = 24;
  desc.mChannelsPerFrame = 4;
  desc.mBytesPerFrame = 12;
  desc.mFramesPerPacket = 1;
  desc.mBytesPerPacket = desc.mFramesPerPacket * desc.mBytesPerFrame;

  AudioObjectPropertyAddress address{};
  address.mSelector = kAudioStreamPropertyPhysicalFormat;
  address.mScope = kAudioObjectPropertyScopeOutput;
  address.mElement = kAudioObjectPropertyElementMaster;

  err = AudioObjectSetPropertyData(id, &address, 0, nullptr, sizeof(desc), &desc);
  ENSURES(err == noErr, "setting physical format failed");

  AudioStreamBasicDescription actual{};
  UInt32 size{sizeof(actual)};

  address.mSelector = kAudioStreamPropertyPhysicalFormat;
  err = AudioObjectGetPropertyData(id, &address, 0, nullptr, &size, &actual);
  ENSURES(err == noErr, "getting physical format failed");
  ENSURES(IsEqual(desc, actual), "wrong phyiscal format");

  address.mSelector = kAudioStreamPropertyVirtualFormat;
  err = AudioObjectGetPropertyData(id, &address, 0, nullptr, &size, &actual);
  ENSURES(err == noErr, "getting virtual format failed");
  ENSURES(IsEqual(desc, actual), "wrong virtual format");

  return {desc.mBitsPerChannel, desc.mChannelsPerFrame, static_cast<unsigned>(desc.mSampleRate)};
}

} // namespace

CoreAudioDevice::CoreAudioDevice(AudioBuffer<228000> &audio_buffer, FlowControl &flow)
    : uln2_{}, proc_id_{}, format_{}, audio_buffer_{audio_buffer}, flow_{flow} {}

CoreAudioDevice::~CoreAudioDevice() {
  OSStatus err = noErr;
  err = AudioDeviceStop(uln2_, proc_id_);
  ENSURES(err == noErr, "stop failed");
  err = AudioDeviceDestroyIOProcID(uln2_, proc_id_);
  ENSURES(err == noErr, "destroy failed");
}

void CoreAudioDevice::Init(const AudioFormat format) {
  const std::vector<AudioObjectID> ids = GetAudioDevices();
  const StringRef metric_halo{CFSTR("ULN-2")};

  for (const AudioObjectID id : ids) {
    const StringRef device_name = GetDeviceName(id);
    if (device_name == metric_halo) {
      uln2_ = id;
      break;
    }
  }
  ENSURES(uln2_ != 0, "can't find ULN2");

  SetHogMode(uln2_);
  SetFrameSize(uln2_);
  SetStreamFormat(uln2_, format);

  if (format.bits == 16) {
    const OSStatus err = AudioDeviceCreateIOProcID(uln2_, Raw16, this, &proc_id_);
    ENSURES(err == noErr, "setting callback failed");
  } else if (format.bits == 24) {
    const OSStatus err = AudioDeviceCreateIOProcID(uln2_, Raw24, this, &proc_id_);
    ENSURES(err == noErr, "setting callback failed");
  } else {
    ENSURES(false, "unsupported format.bits");
  }

  format_ = format;
}

void CoreAudioDevice::Playback() {
  const OSStatus err = AudioDeviceStart(uln2_, proc_id_);
  ENSURES(err == noErr, "start failed");
}

} // namespace plac
