// SPDX-License-Identifier: MIT

#include "alsa_audio_device.h"
#include "stream.h"
#include <sched.h>

int main(int argc, char *argv[]) {
    EXPECTS(argc > 1, "no file provided");

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(3, &cpu_set);
    // to make affinity work,
    // fence cpu `isolcpus=3 rcu_nocbs=3 nohz_full=3 irqaffinity=0-2` in /boot/firmware/cmdline.txt
    // explanation of nohz_full https://www.suse.com/c/cpu-isolation-introduction-part-1/
    if (::sched_setaffinity(0, sizeof(cpu_set), &cpu_set) != 0) {
        LOG_ERROR("failed to set CPU affinity");
    }

    // https://wiki.linuxfoundation.org/realtime/documentation/howto/applications/application_base
    // check that PREEMPT_RT is enabled with `cat /sys/kernel/realtime`. expected value is 1

    sched_param param{};
    // stay below 50 which is the default RT priority for interrupts.
    // relevant interrupts are USB for audio and disk
    // list them with `sudo cat /proc/interrupts `
    param.sched_priority = 40;
    if (::sched_setscheduler(0, SCHED_FIFO, &param) != 0) {
        // if EPERM is returned,
        // add capability for the file with `sudo setcap 'cap_sys_nice=eip' <path>/flacplayer`
        // check it with `getcap <path>/flacplayer`
        LOG_ERROR("failed to set scheduling parameters: {}", ::strerror(errno));
    }

    ::plac::Stream stream{plac::AlsaAudioDevice::Output::uln2};

    bool first{true};
    while (optind <= (argc - 1)) {
        if (!stream.Reset(argv[optind++])) {
            continue;
        }
        if (first) {
            first = false;
            stream.device_.Init(stream.format_, ::plac::AlsaAudioDevice::LogLevel::non_verbose);
        }
        if (stream.device_.format_ != stream.format_) {
            LOG_ERROR("audio format mismatch");
            break;
        }

        stream.Decode();
    }

    stream.device_.Drain();

    return 0;
}
