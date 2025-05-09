// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include "py_streamsession.h"
#include "py_settings.h"
#include "py_av_frame.h"

#include <chiaki-pybind.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <set>
#include <optional> // Required for std::optional

#define PYBIND11_DETAILED_ERROR_MESSAGES
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>

extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
#include <libswscale/swscale.h>
}

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
    return chiaki_pybind_discover(log.get_raw_log(), host.c_str(), timeout);
}

ChiakiErrorCode chiaki_pybind_wakeup_wrapper(PyChiakiLog &log, const std::string &host, const std::string &registkey, bool ps5)
{
    return chiaki_pybind_wakeup(log.get_raw_log(), host.c_str(), registkey.c_str(), ps5);
}

/*PyAVFrame get_frame(StreamSession &session, bool disable_zero_copy)
{
    PyAVFrame pyAvFrame;

    ChiakiFfmpegDecoder *decoder = session.GetFfmpegDecoder();
    if (!decoder)
    {
        // qCCritical(chiakiGui) << "Session has no FFmpeg decoder";
        return pyAvFrame;
    }
    int32_t frames_lost;
    AVFrame *frame = chiaki_ffmpeg_decoder_pull_frame(decoder, &frames_lost);
    if (!frame)
        // qCCritical(chiakiGui) << "Session has no FFmpeg decoder";
        return pyAvFrame;

    static const std::set<int> zero_copy_formats = { AV_PIX_FMT_VULKAN };

    if (frame->hw_frames_ctx && (zero_copy_formats.find(frame->format) != zero_copy_formats.end() || disable_zero_copy))
    {
        AVFrame *sw_frame = av_frame_alloc();
        if (av_hwframe_transfer_data(sw_frame, frame, 0) < 0)
        {
            // qCWarning(chiakiGui) << "Failed to transfer frame from hardware";
            av_frame_unref(frame);
            av_frame_free(&sw_frame);
            return pyAvFrame;
        }
        av_frame_copy_props(sw_frame, frame);
        av_frame_unref(frame);
        frame = sw_frame;
    }

    pyAvFrame.frame = frame;
    return pyAvFrame;
}*/

// Function to return a NumPy array
py::object get_frame(StreamSession &session, bool disable_zero_copy, py::array_t<uint8_t> target)
{
    // Retrieve the FFmpeg decoder
    ChiakiFfmpegDecoder *decoder = session.GetFfmpegDecoder();

    if (!decoder)
    {
        return py::str("Session has no FFmpeg decoder");
    }

    int32_t frames_lost;
    AVFrame *frame = chiaki_ffmpeg_decoder_pull_frame(decoder, &frames_lost);
    if (!frame)
    {
        return py::str("Failed to pull frame from FFmpeg decoder");
    }

    // Ensure proper cleanup if an error occurs
    struct AVFrameGuard
    {
        AVFrame *frame;
        ~AVFrameGuard() { av_frame_unref(frame); }
    } frame_guard{frame};

    // Handle hardware decoding cases
    static const std::set<int> zero_copy_formats = {AV_PIX_FMT_VULKAN,
                                                    AV_PIX_FMT_D3D11
#ifdef __linux__
                                                    ,
                                                    AV_PIX_FMT_VAAPI
#endif
    };

    if ((zero_copy_formats.find(frame->format) != zero_copy_formats.end() || disable_zero_copy))
    {
        AVFrame *sw_frame = av_frame_alloc();
        if (!sw_frame)
        {
            return py::str("Failed to allocate software frame");
        }

        if (av_hwframe_transfer_data(sw_frame, frame, 0) < 0)
        {
            av_frame_free(&sw_frame);
            return py::str("Failed to transfer frame from hardware (D3D11)");
        }

        av_frame_copy_props(sw_frame, frame);
        av_frame_unref(frame);
        frame = sw_frame;
        frame_guard.frame = frame; // Ensure cleanup
    }

    if (frame->format == AV_PIX_FMT_NV12) {
        // Step 1: Initialize SwsContext
        struct SwsContext *sws_ctx = sws_getContext(
            frame->width, frame->height, (AVPixelFormat)frame->format,
            frame->width, frame->height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr);

        if (!sws_ctx)
        {
            av_frame_free(&frame);
            return py::str("Failed to create SwsContext");
        }

        // Step 2: Allocate Target Frame
        AVFrame *rgb_frame = av_frame_alloc();
        if (!rgb_frame)
        {
            sws_freeContext(sws_ctx);
            return py::str("Failed to allocate RGB frame");
        }

        rgb_frame->format = AV_PIX_FMT_RGB24;
        rgb_frame->width = frame->width;
        rgb_frame->height = frame->height;

        if (av_image_alloc(rgb_frame->data, rgb_frame->linesize, rgb_frame->width,
                        rgb_frame->height, AV_PIX_FMT_RGB24, 1) < 0)
        {
            av_frame_free(&rgb_frame);
            sws_freeContext(sws_ctx);
            return py::str("Failed to allocate RGB image buffer");
        }

        // Step 3: Perform the Conversion
        sws_scale(
            sws_ctx,
            frame->data, frame->linesize, 0, frame->height,
            rgb_frame->data, rgb_frame->linesize);

        // Clean up the old frame and replace it with the new one
        av_frame_free(&frame);
        sws_freeContext(sws_ctx);
        frame = rgb_frame;
    }

    // Ensure the frame is in a readable format
    if (frame->format != AV_PIX_FMT_RGB24 && frame->format != AV_PIX_FMT_GRAY8 && frame->format != AV_PIX_FMT_YUV420P) // AV_PIX_FMT_D3D11
    {
        return py::str("Unsupported pixel format for NumPy conversion");
    }

    int height = frame->height;
    int width = frame->width;
    int channels = (frame->format == AV_PIX_FMT_RGB24 || frame->format == AV_PIX_FMT_YUV420P) ? 3 : 1;
    int data_size = av_image_get_buffer_size((AVPixelFormat)frame->format, width, height, 1);

    if (data_size <= 0)
    {
        return py::str("Failed to get image buffer size");
    }

    // Copy data into a buffer
    std::vector<uint8_t> buffer(data_size);
    
    // Request buffer info from the NumPy array
    py::buffer_info array_buf = target.request();

    av_image_copy_to_buffer(static_cast<uint8_t *>(array_buf.ptr), data_size, frame->data, frame->linesize, (AVPixelFormat)frame->format, width, height, 1);

    return py::str("Success");
}

PYBIND11_MODULE(chiaki_py, m)
{
    m.doc() = "Python bindings for Chiaki CLI commands";

    py::class_<PyAVFrame>(m, "AVFrame")
        .def(py::init<>())
        .def("width", &PyAVFrame::width)
        .def("set_width", &PyAVFrame::set_width)
        .def("height", &PyAVFrame::height)
        .def("set_height", &PyAVFrame::set_height)
        .def("format", &PyAVFrame::format)
        .def("set_format", &PyAVFrame::set_format)
        .def("pts", &PyAVFrame::pts)
        .def("set_pts", &PyAVFrame::set_pts)
        .def("data", &PyAVFrame::data)
        .def("to_numpy", &PyAVFrame::to_numpy, "Convert frame data to numpy array");

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

    m.def("get_frame", &get_frame,
          py::arg("session"),
          py::arg("disable_zero_copy"),
          py::arg("target"),
          "Get the next frame from the session.");

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
        .def("get_audio_video_disabled", &Settings::GetAudioVideoDisabled, "Get the audio/video disabled.")
        .def("get_log_verbose", &Settings::GetLogVerbose, "Get the log verbose.")
        .def("set_log_verbose", &Settings::SetLogVerbose, py::arg("log_verbose") , "Set the log verbose.")
        .def("get_log_level_mask", &Settings::GetLogLevelMask, "Get the log level mask.")
        .def("get_rumble_haptics_intensity", &Settings::GetRumbleHapticsIntensity, "Get the rumble haptics intensity.")
        .def("set_rumble_haptics_intensity", &Settings::SetRumbleHapticsIntensity, py::arg("rumble_haptics_intensity"), "Set the rumble haptics intensity.")
        .def("get_buttons_by_position", &Settings::GetButtonsByPosition, "Get the buttons by position.")
        .def("set_buttons_by_position", &Settings::SetButtonsByPosition, py::arg("buttons_by_position"), "Set the buttons by position.")
        .def("get_start_mic_unmuted", &Settings::GetStartMicUnmuted, "Get the start mic unmuted.")
        .def("set_start_mic_unmuted", &Settings::SetStartMicUnmuted, py::arg("start_mic_unmuted"), "Set the start mic unmuted.")
        .def("get_haptic_override", &Settings::GetHapticOverride, "Get the haptic override.")
        .def("set_haptic_override", &Settings::SetHapticOverride, py::arg("haptic_override"), "Set the haptic override.")
        .def("get_resolution_local_ps4", &Settings::GetResolutionLocalPS4, "Get the local PS4 resolution.")
        .def("get_resolution_remote_ps4", &Settings::GetResolutionRemotePS4, "Get the remote PS4 resolution.")
        .def("get_resolution_local_ps5", &Settings::GetResolutionLocalPS5, "Get the local PS5 resolution.")
        .def("get_resolution_remote_ps5", &Settings::GetResolutionRemotePS5, "Get the remote PS5 resolution.")
        .def("set_resolution_local_ps4", &Settings::SetResolutionLocalPS4, py::arg("resolution_local_ps4"), "Set the local PS4 resolution.")
        .def("set_resolution_remote_ps4", &Settings::SetResolutionRemotePS4, py::arg("resolution_remote_ps4"), "Set the remote PS4 resolution.")
        .def("set_resolution_local_ps5", &Settings::SetResolutionLocalPS5, py::arg("resolution_local_ps5"), "Set the local PS5 resolution.")
        .def("set_resolution_remote_ps5", &Settings::SetResolutionRemotePS5, py::arg("resolution_remote_ps5"), "Set the remote PS5 resolution.")
        .def("get_fpslocal_ps4", &Settings::GetFPSLocalPS4, "Get the local PS4 FPS.")
        .def("get_fpsremote_ps4", &Settings::GetFPSRemotePS4, "Get the remote PS4 FPS.")
        .def("get_fpslocal_ps5", &Settings::GetFPSLocalPS5, "Get the local PS5 FPS.")
        .def("get_fpsremote_ps5", &Settings::GetFPSRemotePS5, "Get the remote PS5 FPS.")
        .def("set_fpslocal_ps4", &Settings::SetFPSLocalPS4, py::arg("fps_local_ps4"), "Set the local PS4 FPS.")
        .def("set_fpsremote_ps4", &Settings::SetFPSRemotePS4, py::arg("fps_remote_ps4"), "Set the remote PS4 FPS.")
        .def("set_fpslocal_ps5", &Settings::SetFPSLocalPS5, py::arg("fps_local_ps5"), "Set the local PS5 FPS.")
        .def("set_fpsremote_ps5", &Settings::SetFPSRemotePS5, py::arg("fps_remote_ps5"), "Set the remote PS5 FPS.")
        .def("get_bitrate_local_ps4", &Settings::GetBitrateLocalPS4, "Get the local PS4 bitrate.")
        .def("get_bitrate_remote_ps4", &Settings::GetBitrateRemotePS4, "Get the remote PS4 bitrate.")
        .def("get_bitrate_local_ps5", &Settings::GetBitrateLocalPS5, "Get the local PS5 bitrate.")
        .def("get_bitrate_remote_ps5", &Settings::GetBitrateRemotePS5, "Get the remote PS5 bitrate.")
        .def("set_bitrate_local_ps4", &Settings::SetBitrateLocalPS4, py::arg("bitrate_local_ps4"), "Set the local PS4 bitrate.")
        .def("set_bitrate_remote_ps4", &Settings::SetBitrateRemotePS4, py::arg("bitrate_remote_ps4"), "Set the remote PS4 bitrate.")
        .def("set_bitrate_local_ps5", &Settings::SetBitrateLocalPS5, py::arg("bitrate_local_ps5"), "Set the local PS5 bitrate.")
        .def("set_bitrate_remote_ps5", &Settings::SetBitrateRemotePS5, py::arg("bitrate_remote_ps5"), "Set the remote PS5 bitrate.")
        .def("get_codec_ps4", &Settings::GetCodecPS4, "Get the PS4 codec.")
        .def("get_codec_local_ps5", &Settings::GetCodecLocalPS5, "Get the local PS5 codec.")
        .def("get_codec_remote_ps5", &Settings::GetCodecRemotePS5, "Get the remote PS5 codec.")
        .def("set_codec_ps4", &Settings::SetCodecPS4, py::arg("codec_ps4"), "Set the PS4 codec.")
        .def("set_codec_local_ps5", &Settings::SetCodecLocalPS5, py::arg("codec_local_ps5"), "Set the local PS5 codec.")
        .def("set_codec_remote_ps5", &Settings::SetCodecRemotePS5, py::arg("codec_remote_ps5"), "Set the remote PS5 codec.")
        .def("get_display_target_contrast", &Settings::GetDisplayTargetContrast, "Get the display target contrast.")
        .def("set_display_target_contrast", &Settings::SetDisplayTargetContrast, py::arg("display_target_contrast"), "Set the display target contrast.")
        .def("get_display_target_peak", &Settings::GetDisplayTargetPeak, "Get the display target peak.")
        .def("set_display_target_peak", &Settings::SetDisplayTargetPeak, py::arg("display_target_peak"), "Set the display target peak.")
        .def("get_display_target_trc", &Settings::GetDisplayTargetTrc, "Get the display target TRC.")
        .def("set_display_target_trc", &Settings::SetDisplayTargetTrc, py::arg("display_target_trc"), "Set the display target TRC.")
        .def("get_display_target_prim", &Settings::GetDisplayTargetPrim, "Get the display target PRIM.")
        .def("set_display_target_prim", &Settings::SetDisplayTargetPrim, py::arg("display_target_prim"), "Set the display target PRIM.")
        .def("get_decoder", &Settings::GetDecoder, "Get the decoder.")
        .def("set_decoder", &Settings::SetDecoder, py::arg("decoder"), "Set the decoder.")
        .def("get_hardware_decoder", &Settings::GetHardwareDecoder, "Get the hardware decoder.")
        .def("set_hardware_decoder", &Settings::SetHardwareDecoder, py::arg("hardware_decoder"), "Set the hardware decoder.")
        .def("get_packet_loss_max", &Settings::GetPacketLossMax, "Get the packet loss max.")
        .def("set_packet_loss_max", &Settings::SetPacketLossMax, py::arg("packet_loss_max"), "Set the packet loss max.")
        .def("get_audio_volume", &Settings::GetAudioVolume, "Get the audio volume.")
        .def("set_audio_volume", &Settings::SetAudioVolume, py::arg("audio_volume"), "Set the audio volume.")
        .def("get_audio_buffer_size_default", &Settings::GetAudioBufferSizeDefault, "Get the audio buffer size default.")
        .def("get_audio_buffer_size_raw", &Settings::GetAudioBufferSizeRaw, "Get the audio buffer size raw.")
        .def("get_audio_buffer_size", &Settings::GetAudioBufferSize, "Get the audio buffer size.")
        .def("set_audio_buffer_size", &Settings::SetAudioBufferSize, py::arg("audio_buffer_size"), "Set the audio buffer size.")
        .def("get_audio_out_device", &Settings::GetAudioOutDevice, "Get the audio out device.")
        .def("set_audio_out_device", &Settings::SetAudioOutDevice, py::arg("audio_out_device"), "Set the audio out device.")
        .def("get_audio_in_device", &Settings::GetAudioInDevice, "Get the audio in device.")
        .def("set_audio_in_device", &Settings::SetAudioInDevice, py::arg("audio_in_device"), "Set the audio in device.")
        .def("get_psn_auth_token", &Settings::GetPsnAuthToken, "Get the PSN auth token.")
        .def("set_psn_auth_token", &Settings::SetPsnAuthToken, py::arg("psn_auth_token"), "Set the PSN auth token.")
        .def("get_dpad_touch_enabled", &Settings::GetDpadTouchEnabled, "Get the D-pad touch enabled.")
        .def("set_dpad_touch_enabled", &Settings::SetDpadTouchEnabled, py::arg("dpad_touch_enabled"), "Set the D-pad touch enabled.")
        .def("get_dpad_touch_increment", &Settings::GetDpadTouchIncrement, "Get the D-pad touch increment.")
        .def("set_dpad_touch_increment", &Settings::SetDpadTouchIncrement, py::arg("dpad_touch_increment"), "Set the D-pad touch increment.")
        .def("get_dpad_touch_shortcut1", &Settings::GetDpadTouchShortcut1, "Get the D-pad touch shortcut 1.")
        .def("set_dpad_touch_shortcut1", &Settings::SetDpadTouchShortcut1, py::arg("dpad_touch_shortcut1"), "Set the D-pad touch shortcut 1.")
        .def("get_dpad_touch_shortcut2", &Settings::GetDpadTouchShortcut2, "Get the D-pad touch shortcut 2.")
        .def("set_dpad_touch_shortcut2", &Settings::SetDpadTouchShortcut2, py::arg("dpad_touch_shortcut2"), "Set the D-pad touch shortcut 2.")
        .def("get_dpad_touch_shortcut3", &Settings::GetDpadTouchShortcut3, "Get the D-pad touch shortcut 3.")
        .def("set_dpad_touch_shortcut3", &Settings::SetDpadTouchShortcut3, py::arg("dpad_touch_shortcut3"), "Set the D-pad touch shortcut 3.")
        .def("get_dpad_touch_shortcut4", &Settings::GetDpadTouchShortcut4, "Get the D-pad touch shortcut 4.")
        .def("set_dpad_touch_shortcut4", &Settings::SetDpadTouchShortcut4, py::arg("dpad_touch_shortcut4"), "Set the D-pad touch shortcut 4.")
        .def("get_psn_account_id", &Settings::GetPsnAccountId, "Get the PSN account ID.")
        .def("set_psn_account_id", &Settings::SetPsnAccountId, py::arg("psn_account_id"), "Set the PSN account ID.")
        .def("get_video_profile_local_ps4", &Settings::GetVideoProfileLocalPS4, "Get the local PS4 video profile.")
        .def("get_video_profile_remote_ps4", &Settings::GetVideoProfileRemotePS4, "Get the remote PS4 video profile.")
        .def("get_video_profile_local_ps5", &Settings::GetVideoProfileLocalPS5, "Get the local PS5 video profile.")
        .def("get_video_profile_remote_ps5", &Settings::GetVideoProfileRemotePS5, "Get the remote PS5 video profile.")
        .def_static("get_chiaki_controller_button_name", &Settings::GetChiakiControllerButtonName, py::arg("chiaki_button"), "Get the Chiaki controller button name.")
        .def("set_controller_button_mapping", &Settings::SetControllerButtonMapping, py::arg("chiaki_button"), py::arg("key"), "Set the controller button mapping.")
        .def("get_controller_mapping", &Settings::GetControllerMapping, "Get the controller mapping.")
        .def("get_controller_mapping_for_decoding", &Settings::GetControllerMappingForDecoding, "Get the controller mapping for decoding.")
        .def("__repr__", [](const Settings &s)
             {
                std::ostringstream repr;
                repr << "<Settings("
                    << "logVerbose=" << (s.GetLogVerbose() ? "True" : "False") << ", "
                    << "logLevelMask=" << s.GetLogLevelMask() << ", "
                    << "rumbleHapticsIntensity=" << static_cast<int>(s.GetRumbleHapticsIntensity()) << ", "
                    << "buttonsByPosition=" << (s.GetButtonsByPosition() ? "True" : "False") << ", "
                    << "startMicUnmuted=" << (s.GetStartMicUnmuted() ? "True" : "False") << ", "
                    << "hapticOverride=" << s.GetHapticOverride() << ", "
                    << "resolutionLocalPS4=" << static_cast<int>(s.GetResolutionLocalPS4()) << ", "
                    << "resolutionRemotePS4=" << static_cast<int>(s.GetResolutionRemotePS4()) << ", "
                    << "fpsLocalPS4=" << static_cast<int>(s.GetFPSLocalPS4()) << ", "
                    << "fpsRemotePS4=" << static_cast<int>(s.GetFPSRemotePS4()) << ", "
                    << "bitrateLocalPS4=" << s.GetBitrateLocalPS4() << ", "
                    << "bitrateRemotePS4=" << s.GetBitrateRemotePS4() << ", "
                    << "codecPS4=" << static_cast<int>(s.GetCodecPS4()) << ", "
                    << "decoder=" << static_cast<int>(s.GetDecoder()) << ", "
                    << "hardwareDecoder='" << s.GetHardwareDecoder() << "', "
                    << "packetLossMax=" << s.GetPacketLossMax() << ", "
                    << "audioVolume=" << s.GetAudioVolume() << ", "
                    << "audioBufferSize=" << s.GetAudioBufferSize() << ", "
                    << "audioOutDevice='" << s.GetAudioOutDevice() << "', "
                    << "audioInDevice='" << s.GetAudioInDevice() << "', "
                    << "psnAccountId='" << s.GetPsnAccountId() << "'"
                    << ")>";
                return repr.str(); });

    py::class_<StreamSessionConnectInfo>(m, "StreamSessionConnectInfo")
        .def(py::init<>())
        .def(py::init<Settings *, ChiakiTarget, std::string, std::string, std::string &,
                      std::vector<uint8_t> &, std::string, std::string, bool, bool, bool, bool>(),
             py::arg("settings"),
             py::arg("target"),
             py::arg("host"),
             py::arg("nickname"),
             py::arg("regist_key"),
             py::arg("morning"),
             py::arg("initial_login_pin"),
             py::arg("duid"),
             py::arg("auto_regist"),
             py::arg("fullscreen"),
             py::arg("zoom"),
             py::arg("stretch"));

    py::class_<StreamSession>(m, "StreamSession")
        .def(py::init<const StreamSessionConnectInfo &>(), py::arg("connect_info"))
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
        .def("get_cant_display", &StreamSession::GetCantDisplay)
        .def("get_ffmpeg_decoder", &StreamSession::GetFfmpegDecoder)
        .def_readwrite("ffmpeg_frame_available", &StreamSession::FfmpegFrameAvailable)
        .def_readwrite("session_quit", &StreamSession::SessionQuit)
        .def_readwrite("login_pin_requested", &StreamSession::LoginPINRequested)
        .def_readwrite("data_holepunch_progress", &StreamSession::DataHolepunchProgress)
        .def_readwrite("auto_regist_succeeded", &StreamSession::AutoRegistSucceeded)
        .def_readwrite("nickname_received", &StreamSession::NicknameReceived)
        .def_readwrite("connected_changed", &StreamSession::ConnectedChanged)
        .def_readwrite("measured_bitrate_changed", &StreamSession::MeasuredBitrateChanged)
        .def_readwrite("average_packet_loss_changed", &StreamSession::AveragePacketLossChanged)
        .def_readwrite("cant_display_changed", &StreamSession::CantDisplayChanged);
}