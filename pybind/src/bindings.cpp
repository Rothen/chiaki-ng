// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include "py_streamsession.h"
#include "py_settings.h"

// #include <chiaki-pybind.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

#define PYBIND11_DETAILED_ERROR_MESSAGES
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>

namespace py = pybind11;

// Python wrapper for ChiakiLogCb callback
void log_callback_proxy(ChiakiLogLevel level, const char *msg, void *user)
{
    if (user)
    {
        auto *callback = static_cast<std::function<void(int, std::string)> *>(user);
        (*callback)(static_cast<int>(level), std::string(msg)); // Call Python callback
    }
}

class PyChiakiLog
{
public:
    ChiakiLogLevel level;
    std::function<void(int, std::string)> log_callback;
    ChiakiLog chiaki_log;

    PyChiakiLog(ChiakiLogLevel level = CHIAKI_LOG_INFO) : level(level)
    {
        chiaki_log.level_mask = level;
        chiaki_log.cb = log_callback_proxy;
        chiaki_log.user = &log_callback; // Store reference to callback
    }

    void set_callback(std::function<void(int, std::string)> cb)
    {
        log_callback = std::move(cb);
        chiaki_log.user = &log_callback;
    }

    ChiakiLog *get_raw_log()
    {
        return &chiaki_log;
    }
};

ChiakiErrorCode chiaki_pybind_discover_wrapper(PyChiakiLog &log, const std::string &host, const float timeout)
{
    return ChiakiErrorCode::CHIAKI_ERR_SUCCESS; // return chiaki_pybind_discover(log.get_raw_log(), host.c_str(), timeout);
}

ChiakiErrorCode chiaki_pybind_wakeup_wrapper(ChiakiLog *log, const std::string &host, const std::string &registkey, bool ps5){
    return ChiakiErrorCode::CHIAKI_ERR_SUCCESS; // return chiaki_pybind_wakeup(log, host.c_str(), registkey.c_str(), ps5);
}

PYBIND11_MODULE(chiaki_py, m)
{
    m.doc() = "Python bindings for Chiaki CLI commands";

    py::enum_<ChiakiErrorCode>(m, "ErrorCode")
        .value("SUCCESS", CHIAKI_ERR_SUCCESS)
        .value("UNKNOWN", CHIAKI_ERR_UNKNOWN)
        .value("PARSE_ADDR", CHIAKI_ERR_PARSE_ADDR)
        .value("THREAD", CHIAKI_ERR_THREAD)
        .value("MEMORY", CHIAKI_ERR_MEMORY)
        .value("OVERFLOW", CHIAKI_ERR_OVERFLOW)
        .value("NETWORK", CHIAKI_ERR_NETWORK)
        .value("CONNECTION_REFUSED", CHIAKI_ERR_CONNECTION_REFUSED)
        .value("HOST_DOWN", CHIAKI_ERR_HOST_DOWN)
        .value("HOST_UNREACH", CHIAKI_ERR_HOST_UNREACH)
        .value("DISCONNECTED", CHIAKI_ERR_DISCONNECTED)
        .value("INVALID_DATA", CHIAKI_ERR_INVALID_DATA)
        .value("BUF_TOO_SMALL", CHIAKI_ERR_BUF_TOO_SMALL)
        .value("MUTEX_LOCKED", CHIAKI_ERR_MUTEX_LOCKED)
        .value("CANCELED", CHIAKI_ERR_CANCELED)
        .value("TIMEOUT", CHIAKI_ERR_TIMEOUT)
        .value("INVALID_RESPONSE", CHIAKI_ERR_INVALID_RESPONSE)
        .value("INVALID_MAC", CHIAKI_ERR_INVALID_MAC)
        .value("UNINITIALIZED", CHIAKI_ERR_UNINITIALIZED)
        .value("FEC_FAILED", CHIAKI_ERR_FEC_FAILED)
        .value("VERSION_MISMATCH", CHIAKI_ERR_VERSION_MISMATCH)
        .value("HTTP_NONOK", CHIAKI_ERR_HTTP_NONOK)
        .export_values();

    py::enum_<ChiakiTarget>(m, "Target")
        .value("PS4_UNKNOWN", CHIAKI_TARGET_PS4_UNKNOWN)
        .value("PS4_8", CHIAKI_TARGET_PS4_8)
        .value("PS4_9", CHIAKI_TARGET_PS4_9)
        .value("PS4_10", CHIAKI_TARGET_PS4_10)
        .value("PS5_UNKNOWN", CHIAKI_TARGET_PS5_UNKNOWN)
        .value("PS5_1", CHIAKI_TARGET_PS5_1)
        .export_values();

    py::enum_<ChiakiLogLevel>(m, "LogLevel")
        .value("DEBUG", CHIAKI_LOG_DEBUG)
        .value("VERBOSE", CHIAKI_LOG_VERBOSE)
        .value("INFO", CHIAKI_LOG_INFO)
        .value("WARNING", CHIAKI_LOG_WARNING)
        .value("ERROR", CHIAKI_LOG_ERROR)
        .export_values();

    py::enum_<ChiakiDisableAudioVideo>(m, "DisableAudioVideo")
        .value("NONE", CHIAKI_NONE_DISABLED)
        .value("AUDIO", CHIAKI_AUDIO_DISABLED)
        .value("VIDEO", CHIAKI_VIDEO_DISABLED)
        .value("AUDIO_VIDEO", CHIAKI_AUDIO_VIDEO_DISABLED)
        .export_values();

    py::enum_<RumbleHapticsIntensity>(m, "RumbleHapticsIntensity")
        .value("Off", RumbleHapticsIntensity::Off)
        .value("VeryWeak", RumbleHapticsIntensity::VeryWeak)
        .value("Weak", RumbleHapticsIntensity::Weak)
        .value("Normal", RumbleHapticsIntensity::Normal)
        .value("Strong", RumbleHapticsIntensity::Strong)
        .value("VeryStrong", RumbleHapticsIntensity::VeryStrong)
        .export_values();

    py::enum_<ChiakiVideoResolutionPreset>(m, "VideoResolutionPreset")
        .value("PRESET_360p", CHIAKI_VIDEO_RESOLUTION_PRESET_360p)
        .value("PRESET_540p", CHIAKI_VIDEO_RESOLUTION_PRESET_540p)
        .value("PRESET_720p", CHIAKI_VIDEO_RESOLUTION_PRESET_720p)
        .value("PRESET_1080p", CHIAKI_VIDEO_RESOLUTION_PRESET_1080p)
        .export_values();

    py::enum_<ChiakiVideoFPSPreset>(m, "VideoFPSPreset")
        .value("PRESET_30", CHIAKI_VIDEO_FPS_PRESET_30)
        .value("PRESET_60", CHIAKI_VIDEO_FPS_PRESET_60)
        .export_values();

    py::enum_<ChiakiCodec>(m, "Codec")
        .value("H264", CHIAKI_CODEC_H264)
        .value("H265", CHIAKI_CODEC_H265)
        .value("H265_HDR", CHIAKI_CODEC_H265_HDR)
        .export_values();

    py::enum_<Decoder>(m, "Decoder")
        .value("Ffmpeg", Decoder::Ffmpeg)
        .value("Pi", Decoder::Pi)
        .export_values();

    py::class_<PyChiakiLog>(m, "ChiakiLog")
        .def(py::init<ChiakiLogLevel>(), py::arg("level") = CHIAKI_LOG_INFO)
        .def("set_callback", &PyChiakiLog::set_callback, py::arg("callback"), "Set the logging callback function.");

    m.def("discover", &chiaki_pybind_discover_wrapper,
          py::arg("log"),
          py::arg("host"),
          py::arg("timeout"),
          "Discovers available Chiaki devices.");

    m.def("wakeup", &chiaki_pybind_wakeup_wrapper,
          py::arg("log"),
          py::arg("host"),
          py::arg("registkey"),
          py::arg("ps5"),
          "Wakeup Chiaki device.");

    py::class_<ChiakiConnectVideoProfile>(m, "ChiakiConnectVideoProfile")
        .def(py::init<>())
        .def_readwrite("width", &ChiakiConnectVideoProfile::width)
        .def_readwrite("height", &ChiakiConnectVideoProfile::height)
        .def_readwrite("max_fps", &ChiakiConnectVideoProfile::max_fps)
        .def_readwrite("bitrate", &ChiakiConnectVideoProfile::bitrate)
        .def_readwrite("codec", &ChiakiConnectVideoProfile::codec)
        .def("__repr__", [](const ChiakiConnectVideoProfile &p)
             { return "<ChiakiConnectVideoProfile width=" + std::to_string(p.width) +
                      " height=" + std::to_string(p.height) +
                      " max_fps=" + std::to_string(p.max_fps) +
                      " bitrate=" + std::to_string(p.bitrate) +
                      " codec=" + std::to_string(static_cast<int>(p.codec)) + ">"; });

    py::class_<Settings>(m, "Settings")
        .def(py::init<>())
        .def("GetAudioVideoDisabled", &Settings::GetAudioVideoDisabled)
        .def("GetLogVerbose", &Settings::GetLogVerbose)
        .def("GetLogLevelMask", &Settings::GetLogLevelMask)
        .def("GetRumbleHapticsIntensity", &Settings::GetRumbleHapticsIntensity)
        .def("SetRumbleHapticsIntensity", &Settings::SetRumbleHapticsIntensity, py::arg("rumbleHapticsIntensity"), "Set the rumble haptics intensity.")
        .def("GetButtonsByPosition", &Settings::GetButtonsByPosition)
        .def("SetButtonsByPosition", &Settings::SetButtonsByPosition, py::arg("buttonsByPosition"), "Set the buttons by position.")
        .def("GetStartMicUnmuted", &Settings::GetStartMicUnmuted)
        .def("SetStartMicUnmuted", &Settings::SetStartMicUnmuted, py::arg("startMicUnmuted"), "Set the start mic unmuted.")
        .def("GetHapticOverride", &Settings::GetHapticOverride)
        .def("SetHapticOverride", &Settings::SetHapticOverride, py::arg("hapticOverride"), "Set the haptic override.")
        .def("GetResolutionLocalPS4", &Settings::GetResolutionLocalPS4)
        .def("GetResolutionRemotePS4", &Settings::GetResolutionRemotePS4)
        .def("GetResolutionLocalPS5", &Settings::GetResolutionLocalPS5)
        .def("GetResolutionRemotePS5", &Settings::GetResolutionRemotePS5)
        .def("SetResolutionLocalPS4", &Settings::SetResolutionLocalPS4, py::arg("resolutionLocalPS4"), "Set the local PS4 resolution.")
        .def("SetResolutionRemotePS4", &Settings::SetResolutionRemotePS4, py::arg("resolutionRemotePS4"), "Set the remote PS4 resolution.")
        .def("SetResolutionLocalPS5", &Settings::SetResolutionLocalPS5, py::arg("resolutionLocalPS5"), "Set the local PS5 resolution.")
        .def("SetResolutionRemotePS5", &Settings::SetResolutionRemotePS5, py::arg("resolutionRemotePS5"), "Set the remote PS5 resolution.")
        .def("GetFPSLocalPS4", &Settings::GetFPSLocalPS4)
        .def("GetFPSRemotePS4", &Settings::GetFPSRemotePS4)
        .def("GetFPSLocalPS5", &Settings::GetFPSLocalPS5)
        .def("GetFPSRemotePS5", &Settings::GetFPSRemotePS5)
        .def("SetFPSLocalPS4", &Settings::SetFPSLocalPS4, py::arg("fpsLocalPS4"), "Set the local PS4 FPS.")
        .def("SetFPSRemotePS4", &Settings::SetFPSRemotePS4, py::arg("fpsRemotePS4"), "Set the remote PS4 FPS.")
        .def("SetFPSLocalPS5", &Settings::SetFPSLocalPS5, py::arg("fpsLocalPS5"), "Set the local PS5 FPS.")
        .def("SetFPSRemotePS5", &Settings::SetFPSRemotePS5, py::arg("fpsRemotePS5"), "Set the remote PS5 FPS.")
        .def("GetBitrateLocalPS4", &Settings::GetBitrateLocalPS4)
        .def("GetBitrateRemotePS4", &Settings::GetBitrateRemotePS4)
        .def("GetBitrateLocalPS5", &Settings::GetBitrateLocalPS5)
        .def("GetBitrateRemotePS5", &Settings::GetBitrateRemotePS5)
        .def("SetBitrateLocalPS4", &Settings::SetBitrateLocalPS4, py::arg("bitrateLocalPS4"), "Set the local PS4 bitrate.")
        .def("SetBitrateRemotePS4", &Settings::SetBitrateRemotePS4, py::arg("bitrateRemotePS4"), "Set the remote PS4 bitrate.")
        .def("SetBitrateLocalPS5", &Settings::SetBitrateLocalPS5, py::arg("bitrateLocalPS5"), "Set the local PS5 bitrate.")
        .def("SetBitrateRemotePS5", &Settings::SetBitrateRemotePS5, py::arg("bitrateRemotePS5"), "Set the remote PS5 bitrate.")
        .def("GetCodecPS4", &Settings::GetCodecPS4)
        .def("GetCodecLocalPS5", &Settings::GetCodecLocalPS5)
        .def("GetCodecRemotePS5", &Settings::GetCodecRemotePS5)
        .def("SetCodecPS4", &Settings::SetCodecPS4, py::arg("codecPS4"), "Set the PS4 codec.")
        .def("SetCodecLocalPS5", &Settings::SetCodecLocalPS5, py::arg("codecLocalPS5"), "Set the local PS5 codec.")
        .def("SetCodecRemotePS5", &Settings::SetCodecRemotePS5, py::arg("codecRemotePS5"), "Set the remote PS5 codec.")
        .def("GetDisplayTargetContrast", &Settings::GetDisplayTargetContrast)
        .def("SetDisplayTargetContrast", &Settings::SetDisplayTargetContrast, py::arg("displayTargetContrast"), "Set the display target contrast.")
        .def("GetDisplayTargetPeak", &Settings::GetDisplayTargetPeak)
        .def("SetDisplayTargetPeak", &Settings::SetDisplayTargetPeak, py::arg("displayTargetPeak"), "Set the display target peak.")
        .def("GetDisplayTargetTrc", &Settings::GetDisplayTargetTrc)
        .def("SetDisplayTargetTrc", &Settings::SetDisplayTargetTrc, py::arg("displayTargetTrc"), "Set the display target TRC.")
        .def("GetDisplayTargetPrim", &Settings::GetDisplayTargetPrim)
        .def("SetDisplayTargetPrim", &Settings::SetDisplayTargetPrim, py::arg("displayTargetPrim"), "Set the display target PRIM.")
        .def("GetDecoder", &Settings::GetDecoder)
        .def("SetDecoder", &Settings::SetDecoder, py::arg("decoder"), "Set the decoder.")
        .def("GetHardwareDecoder", &Settings::GetHardwareDecoder)
        .def("SetHardwareDecoder", &Settings::SetHardwareDecoder, py::arg("hardwareDecoder"), "Set the hardware decoder.")
        .def("GetPacketLossMax", &Settings::GetPacketLossMax)
        .def("SetPacketLossMax", &Settings::SetPacketLossMax, py::arg("packetLossMax"), "Set the packet loss max.")
        .def("GetAudioVolume", &Settings::GetAudioVolume)
        .def("SetAudioVolume", &Settings::SetAudioVolume, py::arg("audioVolume"), "Set the audio volume.")
        .def("GetAudioBufferSizeDefault", &Settings::GetAudioBufferSizeDefault)
        .def("GetAudioBufferSizeRaw", &Settings::GetAudioBufferSizeRaw)
        .def("GetAudioBufferSize", &Settings::GetAudioBufferSize)
        .def("SetAudioBufferSize", &Settings::SetAudioBufferSize, py::arg("audioBufferSize"), "Set the audio buffer size.")
        .def("GetAudioOutDevice", &Settings::GetAudioOutDevice)
        .def("SetAudioOutDevice", &Settings::SetAudioOutDevice, py::arg("audioOutDevice"), "Set the audio out device.")
        .def("GetAudioInDevice", &Settings::GetAudioInDevice)
        .def("SetAudioInDevice", &Settings::SetAudioInDevice, py::arg("audioInDevice"), "Set the audio in device.")
        .def("GetPsnAuthToken", &Settings::GetPsnAuthToken)
        .def("SetPsnAuthToken", &Settings::SetPsnAuthToken, py::arg("psnAuthToken"), "Set the PSN auth token.")
        .def("GetDpadTouchEnabled", &Settings::GetDpadTouchEnabled)
        .def("SetDpadTouchEnabled", &Settings::SetDpadTouchEnabled, py::arg("dpadTouchEnabled"), "Set the D-pad touch enabled.")
        .def("GetDpadTouchIncrement", &Settings::GetDpadTouchIncrement)
        .def("SetDpadTouchIncrement", &Settings::SetDpadTouchIncrement, py::arg("dpadTouchIncrement"), "Set the D-pad touch increment.")
        .def("GetDpadTouchShortcut1", &Settings::GetDpadTouchShortcut1)
        .def("SetDpadTouchShortcut1", &Settings::SetDpadTouchShortcut1, py::arg("dpadTouchShortcut1"), "Set the D-pad touch shortcut 1.")
        .def("GetDpadTouchShortcut2", &Settings::GetDpadTouchShortcut2)
        .def("SetDpadTouchShortcut2", &Settings::SetDpadTouchShortcut2, py::arg("dpadTouchShortcut2"), "Set the D-pad touch shortcut 2.")
        .def("GetDpadTouchShortcut3", &Settings::GetDpadTouchShortcut3)
        .def("SetDpadTouchShortcut3", &Settings::SetDpadTouchShortcut3, py::arg("dpadTouchShortcut3"), "Set the D-pad touch shortcut 3.")
        .def("GetDpadTouchShortcut4", &Settings::GetDpadTouchShortcut4)
        .def("SetDpadTouchShortcut4", &Settings::SetDpadTouchShortcut4, py::arg("dpadTouchShortcut4"), "Set the D-pad touch shortcut 4.")
        .def("GetPsnAccountId", &Settings::GetPsnAccountId)
        .def("SetPsnAccountId", &Settings::SetPsnAccountId, py::arg("psnAccountId"), "Set the PSN account ID.")
        .def("GetVideoProfileLocalPS4", &Settings::GetVideoProfileLocalPS4)
        .def("GetVideoProfileRemotePS4", &Settings::GetVideoProfileRemotePS4)
        .def("GetVideoProfileLocalPS5", &Settings::GetVideoProfileLocalPS5)
        .def("GetVideoProfileRemotePS5", &Settings::GetVideoProfileRemotePS5)
        .def("GetChiakiControllerButtonName", &Settings::GetChiakiControllerButtonName)
        .def("SetControllerButtonMapping", &Settings::SetControllerButtonMapping, py::arg("chiaki_button"), py::arg("key"), "Set the controller button mapping.")
        .def("GetControllerMapping", &Settings::GetControllerMapping)
        .def("GetControllerMappingForDecoding", &Settings::GetControllerMappingForDecoding);

    /*py::class_<StreamSessionConnectInfo>(m, "StreamSessionConnectInfo")
        .def(py::init<>())
        .def(py::init<Settings *, ChiakiTarget, std::string, std::string, std::vector<uint8_t>,
                      std::vector<uint8_t>, std::string, std::string, bool, bool, bool, bool>())
        .def_readwrite("settings", &StreamSessionConnectInfo::settings)
        .def_readwrite("key_map", &StreamSessionConnectInfo::key_map)
        .def_readwrite("decoder", &StreamSessionConnectInfo::decoder)
        .def_readwrite("hw_decoder", &StreamSessionConnectInfo::hw_decoder)
        .def_readwrite("audio_out_device", &StreamSessionConnectInfo::audio_out_device)
        .def_readwrite("audio_in_device", &StreamSessionConnectInfo::audio_in_device)
        .def_readwrite("log_level_mask", &StreamSessionConnectInfo::log_level_mask)
        .def_readwrite("log_file", &StreamSessionConnectInfo::log_file)
        .def_readwrite("target", &StreamSessionConnectInfo::target)
        .def_readwrite("host", &StreamSessionConnectInfo::host)
        .def_readwrite("nickname", &StreamSessionConnectInfo::nickname)
        .def_readwrite("regist_key", &StreamSessionConnectInfo::regist_key)
        .def_readwrite("morning", &StreamSessionConnectInfo::morning)
        .def_readwrite("initial_login_pin", &StreamSessionConnectInfo::initial_login_pin)
        .def_readwrite("video_profile", &StreamSessionConnectInfo::video_profile)
        .def_readwrite("packet_loss_max", &StreamSessionConnectInfo::packet_loss_max)
        .def_readwrite("audio_buffer_size", &StreamSessionConnectInfo::audio_buffer_size)
        .def_readwrite("audio_volume", &StreamSessionConnectInfo::audio_volume)
        .def_readwrite("fullscreen", &StreamSessionConnectInfo::fullscreen)
        .def_readwrite("zoom", &StreamSessionConnectInfo::zoom)
        .def_readwrite("stretch", &StreamSessionConnectInfo::stretch)
        .def_readwrite("enable_keyboard", &StreamSessionConnectInfo::enable_keyboard)
        .def_readwrite("enable_dualsense", &StreamSessionConnectInfo::enable_dualsense)
        .def_readwrite("auto_regist", &StreamSessionConnectInfo::auto_regist);

    py::class_<StreamSession>(m, "StreamSession")
        .def(py::init<const StreamSessionConnectInfo &>())
        .def("start", &StreamSession::Start)
        .def("stop", &StreamSession::Stop)
        .def("go_to_bed", &StreamSession::GoToBed)
        .def("set_login_pin", &StreamSession::SetLoginPIN)
        .def("go_home", &StreamSession::GoHome)
        .def("get_host", &StreamSession::GetHost)
        .def("is_connected", &StreamSession::IsConnected)
        .def("is_connecting", &StreamSession::IsConnecting)
        .def("get_measured_bitrate", &StreamSession::GetMeasuredBitrate)
        .def("get_average_packet_loss", &StreamSession::GetAveragePacketLoss)
        .def("get_muted", &StreamSession::GetMuted)
        .def("set_audio_volume", &StreamSession::SetAudioVolume)
        .def("get_cant_display", &StreamSession::GetCantDisplay);*/
}