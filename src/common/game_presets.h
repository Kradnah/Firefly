// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "common/settings.h"
#include "common/thread.h"

namespace Settings {

inline constexpr u64 MarioKartWorldProgramId = 0x0400C3F00006E000ULL;

inline void SetMarioKartWorldRecommendedSettings(Values& settings, bool global) {
    const auto session = [global](auto& setting, auto value) {
        setting.SetGlobal(global);
        setting.SetValue(value);
    };
    session(settings.renderer_backend, RendererBackend::Vulkan);
    session(settings.resolution_setup, ResolutionSetup::Res1X);
    session(settings.gpu_accuracy, GpuAccuracy::Low);
    session(settings.use_asynchronous_shaders, true);
    session(settings.astc_recompression, AstcRecompression::Bc3);
    session(settings.vram_usage_mode, VramUsageMode::Conservative);
    session(settings.use_reactive_flushing, false);
    session(settings.vsync_mode, VSyncMode::FifoRelaxed);
    session(settings.use_docked_mode, ConsoleMode::Handheld);
    session(settings.scaling_filter, ScalingFilter::ScaleForce);
    session(settings.anti_aliasing, AntiAliasing::Fxaa);
}

inline bool ApplyMarioKartWorldPreset(Values& settings, u64 program_id, bool accepted) {
    if (!accepted || program_id != MarioKartWorldProgramId) {
        return false;
    }
    SetMarioKartWorldRecommendedSettings(settings, false);
    return true;
}

inline constexpr u64 OcarinaOfTimeProgramId = 0x0400486024DEC000ULL;

// Firefly is tuned for Ocarina of Time at GPU accuracy Low, where the GPU work runs on two
// threads (High stops the split, about 25-30 FPS instead of 60). The game starts there for the
// session unless its own settings choose an accuracy; saved settings are not changed.
inline bool ApplyOcarinaOfTimeDefaults(Values& settings, u64 program_id) {
    if (program_id != OcarinaOfTimeProgramId || !settings.gpu_accuracy.UsingGlobal()) {
        return false;
    }
    settings.gpu_accuracy.SetGlobal(false);
    settings.gpu_accuracy.SetValue(GpuAccuracy::Low);
    return true;
}

// The profile Ocarina of Time starts with on this hardware: a GPU with little memory gets the
// lightest profile, one under 6 GiB or a CPU with fewer than 12 threads 30 FPS.
inline PerformanceProfile RecommendedPerformanceProfile(u64 device_local_bytes, bool integrated,
                                                        u32 cpus) {
    constexpr u64 GiB = 1ULL << 30;
    if (integrated || device_local_bytes < 3 * GiB + GiB / 2) {
        return PerformanceProfile::MaxPerformance;
    }
    if (device_local_bytes < 6 * GiB || cpus < 12) {
        return PerformanceProfile::Fps30;
    }
    return PerformanceProfile::Fps60;
}

// Applies the performance profile for the session when Ocarina of Time starts. 30 FPS: docked, in
// low-end mode, frames held to 30 with the game's step doubled to match (or Dark's Dynamic FPS
// when it is enabled), Mailbox instead of Immediate so the host pacer can space the frames, and
// no polling waits. The lightest profile is the same in handheld mode. Settings the game's own
// configuration chooses are kept; saved settings are not changed.
inline bool ApplyOcarinaOfTimeProfile(Values& settings, u64 program_id) {
    const auto profile = settings.performance_profile.GetValue();
    const bool light =
        program_id == OcarinaOfTimeProgramId && profile != PerformanceProfile::Fps60;
    settings.light_profile_active = light;
    Common::Event::SetBlockingTimedWaits(light);
    if (!light) {
        return false;
    }
    const auto session = [](auto& setting, auto value) {
        if (!setting.UsingGlobal()) {
            return;
        }
        setting.SetGlobal(false);
        setting.SetValue(value);
    };
    session(settings.use_docked_mode, profile == PerformanceProfile::MaxPerformance
                                          ? ConsoleMode::Handheld
                                          : ConsoleMode::Docked);
    session(settings.low_end_mode, true);
    if (settings.vsync_mode.GetValue() == VSyncMode::Immediate) {
        session(settings.vsync_mode, VSyncMode::Mailbox);
    }
    return true;
}

} // namespace Settings
