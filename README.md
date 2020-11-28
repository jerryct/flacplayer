# flacplayer

## Quickstart

```
cmake -H. -B_build_arm -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=arm_toolchain.cmake
```

Copy [`asoundrc`](https://www.alsa-project.org/main/index.php/Asoundrc) into your home directory as `.asoundrc`.

## Misc

http://www.volkerschatz.com/noise/alsa.html

```
cat /proc/asound/card1/pcm0p/sub0/status
cat /proc/asound/card1/pcm0p/sub0/sw_params
cat /proc/asound/card1/pcm0p/sub0/hw_params
```

https://www.kernel.org/doc/html/latest/sound/designs/timestamping.html

```
--------------------------------------------------------------> time
  ^               ^              ^                ^           ^
  |               |              |                |           |
 analog         link            dma              app       FullBuffer
 time           time           time              time        time
  |               |              |                |           |
  |< codec delay >|<--hw delay-->|<queued samples>|<---avail->|
  |<----------------- delay---------------------->|           |
                                 |<----ring buffer length---->|
```

ULN2 hardware parameters

```
 ACCESS:  MMAP_INTERLEAVED RW_INTERLEAVED
 FORMAT:  S24_3LE
 SUBFORMAT:  STD
 SAMPLE_BITS: 24
 FRAME_BITS: 192
 CHANNELS: 8
 RATE: [44100 192000]
 PERIOD_TIME: [125 495352)
 PERIOD_SIZE: [6 21845]
 PERIOD_BYTES: [144 524280]
 PERIODS: [2 1024]
 BUFFER_TIME: (62 990703)
 BUFFER_SIZE: [12 43690]
 BUFFER_BYTES: [288 1048560]
 TICK_TIME: ALL
```

Then period size * period is your total buffer size in bytes.
