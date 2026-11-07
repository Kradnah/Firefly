// SPDX-License-Identifier: GPL-3.0-or-later

#include <memory>
#include <catch2/catch_test_macros.hpp>
#include "common/game_presets.h"

namespace {
void SetConflictingPreferences(Settings::Values& settings, bool per_game) {
    const auto set = [per_game](auto& setting, auto value) {
        setting.SetGlobal(!per_game);
        setting.SetValue(value);
    };
    set(settings.renderer_backend, Settings::RendererBackend::Null);
    set(settings.resolution_setup, Settings::ResolutionSetup::Res2X);
    set(settings.gpu_accuracy, Settings::GpuAccuracy::High);
    set(settings.use_asynchronous_shaders, false);
    set(settings.astc_recompression, Settings::AstcRecompression::Uncompressed);
    set(settings.vram_usage_mode, Settings::VramUsageMode::Aggressive);
    set(settings.use_reactive_flushing, true);
    set(settings.vsync_mode, Settings::VSyncMode::Immediate);
    set(settings.scaling_filter, Settings::ScalingFilter::NearestNeighbor);
    set(settings.anti_aliasing, Settings::AntiAliasing::Smaa);
}

void CheckPreset(const Settings::Values& settings) {
    CHECK(settings.renderer_backend.GetValue() == Settings::RendererBackend::Vulkan);
    CHECK(settings.resolution_setup.GetValue() == Settings::ResolutionSetup::Res1X);
    CHECK(settings.gpu_accuracy.GetValue() == Settings::GpuAccuracy::Low);
    CHECK(settings.use_asynchronous_shaders.GetValue());
    CHECK(settings.astc_recompression.GetValue() == Settings::AstcRecompression::Bc3);
    CHECK(settings.vram_usage_mode.GetValue() == Settings::VramUsageMode::Conservative);
    CHECK_FALSE(settings.use_reactive_flushing.GetValue());
    CHECK(settings.vsync_mode.GetValue() == Settings::VSyncMode::FifoRelaxed);
    CHECK(settings.use_docked_mode.GetValue() == Settings::ConsoleMode::Handheld);
    CHECK(settings.scaling_filter.GetValue() == Settings::ScalingFilter::ScaleForce);
    CHECK(settings.anti_aliasing.GetValue() == Settings::AntiAliasing::Fxaa);
}

void CheckPreferences(const Settings::Values& settings, bool global = false) {
    CHECK(settings.renderer_backend.GetValue(global) == Settings::RendererBackend::Null);
    CHECK(settings.resolution_setup.GetValue(global) == Settings::ResolutionSetup::Res2X);
    CHECK(settings.gpu_accuracy.GetValue(global) == Settings::GpuAccuracy::High);
    CHECK_FALSE(settings.use_asynchronous_shaders.GetValue(global));
    CHECK(settings.astc_recompression.GetValue(global) ==
          Settings::AstcRecompression::Uncompressed);
    CHECK(settings.vram_usage_mode.GetValue(global) == Settings::VramUsageMode::Aggressive);
    CHECK(settings.use_reactive_flushing.GetValue(global));
    CHECK(settings.vsync_mode.GetValue(global) == Settings::VSyncMode::Immediate);
    CHECK(settings.scaling_filter.GetValue(global) == Settings::ScalingFilter::NearestNeighbor);
    CHECK(settings.anti_aliasing.GetValue(global) == Settings::AntiAliasing::Smaa);
}
} // namespace

TEST_CASE("Display filter defaults use ScaleForce and FXAA", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    CHECK(settings->scaling_filter.GetDefault() == Settings::ScalingFilter::ScaleForce);
    CHECK(settings->anti_aliasing.GetDefault() == Settings::AntiAliasing::Fxaa);
}

TEST_CASE("Accepted MKW settings replace visible global preferences", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    SetConflictingPreferences(*settings, false);
    Settings::SetMarioKartWorldRecommendedSettings(*settings, true);
    CheckPreset(*settings);
    CHECK(settings->renderer_backend.UsingGlobal());
    CHECK(settings->gpu_accuracy.UsingGlobal());
    CHECK(settings->renderer_backend.GetValue(true) == Settings::RendererBackend::Vulkan);
    CHECK(settings->gpu_accuracy.GetValue(true) == Settings::GpuAccuracy::Low);
}

TEST_CASE("MKW preset overrides saved settings for the session", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    SetConflictingPreferences(*settings, false);
    SetConflictingPreferences(*settings, true);
    REQUIRE(
        Settings::ApplyMarioKartWorldPreset(*settings, Settings::MarioKartWorldProgramId, true));
    CheckPreset(*settings);
    CheckPreferences(*settings, true);
    for (const auto& restore : settings->linkage.restore_functions) {
        restore();
    }
    CheckPreferences(*settings);
    CHECK(settings->renderer_backend.UsingGlobal());
    CHECK(settings->resolution_setup.UsingGlobal());
    REQUIRE(
        Settings::ApplyMarioKartWorldPreset(*settings, Settings::MarioKartWorldProgramId, true));
    CheckPreset(*settings);
}

TEST_CASE("MKW preset does not change unrelated games", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    SetConflictingPreferences(*settings, false);
    CHECK_FALSE(Settings::ApplyMarioKartWorldPreset(*settings, 0, true));
    CHECK_FALSE(Settings::ApplyMarioKartWorldPreset(*settings,
                                                    Settings::MarioKartWorldProgramId + 1, true));
    CheckPreferences(*settings);
    CHECK(settings->renderer_backend.UsingGlobal());
    CHECK(settings->resolution_setup.UsingGlobal());
}

TEST_CASE("Declining the MKW preset preserves global and per-game preferences", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    for (bool per_game : {false, true}) {
        SetConflictingPreferences(*settings, per_game);
        CHECK_FALSE(Settings::ApplyMarioKartWorldPreset(*settings,
                                                        Settings::MarioKartWorldProgramId, false));
        CheckPreferences(*settings);
        CHECK(settings->renderer_backend.UsingGlobal() == !per_game);
        CHECK(settings->resolution_setup.UsingGlobal() == !per_game);
    }
}

TEST_CASE("Ocarina of Time starts at GPU accuracy Low for the session", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    settings->gpu_accuracy.SetGlobal(true);
    settings->gpu_accuracy.SetValue(Settings::GpuAccuracy::High);
    REQUIRE(Settings::ApplyOcarinaOfTimeDefaults(*settings, Settings::OcarinaOfTimeProgramId));
    CHECK(settings->gpu_accuracy.GetValue() == Settings::GpuAccuracy::Low);
    CHECK(settings->gpu_accuracy.GetValue(true) == Settings::GpuAccuracy::High);
    for (const auto& restore : settings->linkage.restore_functions) {
        restore();
    }
    CHECK(settings->gpu_accuracy.UsingGlobal());
    CHECK(settings->gpu_accuracy.GetValue() == Settings::GpuAccuracy::High);
}

TEST_CASE("Ocarina of Time keeps an accuracy its own settings choose", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    settings->gpu_accuracy.SetGlobal(true);
    settings->gpu_accuracy.SetValue(Settings::GpuAccuracy::Low);
    settings->gpu_accuracy.SetGlobal(false);
    settings->gpu_accuracy.SetValue(Settings::GpuAccuracy::High);
    CHECK_FALSE(Settings::ApplyOcarinaOfTimeDefaults(*settings, Settings::OcarinaOfTimeProgramId));
    CHECK(settings->gpu_accuracy.GetValue() == Settings::GpuAccuracy::High);
    settings->gpu_accuracy.SetGlobal(true);
    CHECK_FALSE(Settings::ApplyOcarinaOfTimeDefaults(*settings, Settings::MarioKartWorldProgramId));
    CHECK(settings->gpu_accuracy.UsingGlobal());
}

TEST_CASE("Performance profile recommendation follows GPU memory and CPU threads",
          "[game_presets]") {
    constexpr u64 GiB = 1ULL << 30;
    using Settings::PerformanceProfile;
    CHECK(Settings::RecommendedPerformanceProfile(16 * GiB, false, 12) == PerformanceProfile::Fps60);
    CHECK(Settings::RecommendedPerformanceProfile(16 * GiB, false, 8) == PerformanceProfile::Fps30);
    CHECK(Settings::RecommendedPerformanceProfile(4 * GiB, false, 12) == PerformanceProfile::Fps30);
    CHECK(Settings::RecommendedPerformanceProfile(3 * GiB, false, 12) ==
          PerformanceProfile::MaxPerformance);
    CHECK(Settings::RecommendedPerformanceProfile(8 * GiB, true, 16) ==
          PerformanceProfile::MaxPerformance);
}

TEST_CASE("Ocarina of Time's 30 FPS profiles apply for the session only", "[game_presets]") {
    auto settings = std::make_unique<Settings::Values>();
    settings->vsync_mode.SetValue(Settings::VSyncMode::Immediate);
    settings->use_docked_mode.SetValue(Settings::ConsoleMode::Docked);
    settings->performance_profile.SetValue(Settings::PerformanceProfile::MaxPerformance);
    REQUIRE(Settings::ApplyOcarinaOfTimeProfile(*settings, Settings::OcarinaOfTimeProgramId));
    CHECK(settings->light_profile_active);
    CHECK(settings->low_end_mode.GetValue());
    CHECK(settings->use_docked_mode.GetValue() == Settings::ConsoleMode::Handheld);
    CHECK(settings->vsync_mode.GetValue() == Settings::VSyncMode::Mailbox);
    CHECK_FALSE(settings->low_end_mode.GetValue(true));
    for (const auto& restore : settings->linkage.restore_functions) {
        restore();
    }
    CHECK_FALSE(settings->low_end_mode.GetValue());
    CHECK(settings->vsync_mode.GetValue() == Settings::VSyncMode::Immediate);

    settings->performance_profile.SetValue(Settings::PerformanceProfile::Fps60);
    CHECK_FALSE(Settings::ApplyOcarinaOfTimeProfile(*settings, Settings::OcarinaOfTimeProgramId));
    CHECK_FALSE(settings->light_profile_active);
    settings->performance_profile.SetValue(Settings::PerformanceProfile::Fps30);
    CHECK_FALSE(Settings::ApplyOcarinaOfTimeProfile(*settings, Settings::MarioKartWorldProgramId));
    CHECK_FALSE(settings->light_profile_active);
    Common::Event::SetBlockingTimedWaits(false);
}
