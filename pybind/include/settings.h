// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_PY_SETTINGS_H
#define CHIAKI_PY_SETTINGS_H

#include "py_host.h"
#include <chiaki/session.h>

#include <unordered_map>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <limits>
#include <map>
#include <list>
#include <algorithm>
#include <functional>
#include <vector>

class HostMAC;
class HiddenHost;
class RegisteredHost;
class ManualHost;
class PsnHost;

enum class ControllerButtonExt
{
    // must not overlap with ChiakiControllerButton and ChiakiControllerAnalogButton
    ANALOG_STICK_LEFT_X_UP = (1 << 18),
    ANALOG_STICK_LEFT_X_DOWN = (1 << 19),
    ANALOG_STICK_LEFT_Y_UP = (1 << 20),
    ANALOG_STICK_LEFT_Y_DOWN = (1 << 21),
    ANALOG_STICK_RIGHT_X_UP = (1 << 22),
    ANALOG_STICK_RIGHT_X_DOWN = (1 << 23),
    ANALOG_STICK_RIGHT_Y_UP = (1 << 24),
    ANALOG_STICK_RIGHT_Y_DOWN = (1 << 25),
    ANALOG_STICK_LEFT_X = (1 << 26),
    ANALOG_STICK_LEFT_Y = (1 << 27),
    ANALOG_STICK_RIGHT_X = (1 << 28),
    ANALOG_STICK_RIGHT_Y = (1 << 29),
    MISC1 = (1 << 30),
};

enum class RumbleHapticsIntensity
{
	Off,
	VeryWeak,
	Weak,
	Normal,
	Strong,
	VeryStrong
};

enum class DisconnectAction
{
	AlwaysNothing,
	AlwaysSleep,
	Ask
};

enum class SuspendAction
{
	Nothing,
	Sleep,
};

enum class Decoder
{
	Ffmpeg,
	Pi
};

enum class PlaceboPreset {
	Fast,
	Default,
	HighQuality,
	Custom
};

enum class WindowType {
	SelectedResolution,
	CustomResolution,
	AdjustableResolution,
	Fullscreen,
	Zoom,
	Stretch
};

enum class PlaceboUpscaler {
	None,
	Nearest,
	Bilinear,
	Oversample,
	Bicubic,
	Gaussian,
	CatmullRom,
	Lanczos,
	EwaLanczos,
	EwaLanczosSharp,
	EwaLanczos4Sharpest
};

enum class PlaceboDownscaler {
	None,
	Box,
	Hermite,
	Bilinear,
	Bicubic,
	Gaussian,
	CatmullRom,
	Mitchell,
	Lanczos
};

enum class PlaceboFrameMixer {
	None,
	Oversample,
	Hermite,
	Linear,
	Cubic
};

enum class PlaceboDebandPreset {
	None,
	Default
};

enum class PlaceboSigmoidPreset {
	None,
	Default
};

enum class PlaceboColorAdjustmentPreset {
	None,
	Neutral
};

enum class PlaceboPeakDetectionPreset {
	None,
	Default,
	HighQuality
};

enum class PlaceboColorMappingPreset {
	None,
	Default,
	HighQuality
};

enum class PlaceboGamutMappingFunction {
	Clip,
	Perceptual,
	SoftClip,
	Relative,
	Saturation,
	Absolute,
	Desaturate,
	Darken,
	Highlight,
	Linear
};

enum class PlaceboToneMappingFunction {
	Clip,
	Spline,
	St209440,
	St209410,
	Bt2390,
	Bt2446a,
	Reinhard,
	Mobius,
	Hable,
	Gamma,
	Linear,
	LinearLight
};

enum class PlaceboToneMappingMetadata {
	Any,
	None,
	Hdr10,
	Hdr10Plus,
	CieY
};

struct Rect
{
    int x, y, width, height;

    Rect(int x = 0, int y = 0, int width = 0, int height = 0)
        : x(x), y(y), width(width), height(height) {}

    bool contains(int px, int py) const
    {
        return px >= x && px < x + width && py >= y && py < y + height;
    }

    bool intersects(const Rect &other) const
    {
        return !(x + width <= other.x || other.x + other.width <= x ||
                 y + height <= other.y || other.y + other.height <= y);
    }
};

unsigned stou(std::string const &str, size_t *idx = 0, int base = 10)
{
    unsigned long result = std::stoul(str, idx, base);
    if (result > std::numeric_limits<unsigned>::max())
    {
        throw std::out_of_range("stou");
    }
    return result;
}

class Settings
{
	private:
		SettingsObj settings;
		std::string time_format;
		std::map<HostMAC, HiddenHost> hidden_hosts;

		std::map<HostMAC, RegisteredHost> registered_hosts;
		std::map<std::string, RegisteredHost> nickname_registered_hosts;
		std::map<std::string, std::string> controller_mappings;
		std::list<std::string> profiles;
		size_t ps4s_registered;
		std::map<int, ManualHost> manual_hosts;
		int manual_hosts_id_next;

		void LoadRegisteredHosts(SettingsObj *qsettings = nullptr);
		void SaveRegisteredHosts(SettingsObj *qsettings = nullptr);

		void LoadHiddenHosts(SettingsObj *qsettings = nullptr);
		void SaveHiddenHosts(SettingsObj *qsettings = nullptr);

		void LoadManualHosts(SettingsObj *qsettings = nullptr);
		void SaveManualHosts(SettingsObj *qsettings = nullptr);

		void LoadControllerMappings(SettingsObj *qsettings = nullptr);
		void SaveControllerMappings(SettingsObj *qsettings = nullptr);

		void LoadProfiles();
		void SaveProfiles();

	public:
		explicit Settings(const std::string &conf);

		ChiakiDisableAudioVideo GetAudioVideoDisabled() const 
        { 
            return static_cast<ChiakiDisableAudioVideo>(settings.getIntValue("settings/audio_video_disabled", 0));
        }

		bool GetLogVerbose() const 				{ return settings.getBoolValue("settings/log_verbose", false); }
		uint32_t GetLogLevelMask();


		RumbleHapticsIntensity GetRumbleHapticsIntensity() const;
		void SetRumbleHapticsIntensity(RumbleHapticsIntensity intensity);

		bool GetButtonsByPosition() const 		{ return settings.getBoolValue("settings/buttons_by_pos", false); }
		void SetButtonsByPosition(bool enabled) { settings.setValue("settings/buttons_by_pos", enabled); }

		bool GetStartMicUnmuted() const          { return settings.getBoolValue("settings/start_mic_unmuted", false); }
		void SetStartMicUnmuted(bool unmuted) { return settings.setValue("settings/start_mic_unmuted", unmuted); }

		float GetHapticOverride() const 			{ return settings.getFloatValue("settings/haptic_override", 1.0); }
		void SetHapticOverride(float override)	{ settings.setValue("settings/haptic_override", override); }

		ChiakiVideoResolutionPreset GetResolutionLocalPS4() const;
		ChiakiVideoResolutionPreset GetResolutionRemotePS4() const;
		ChiakiVideoResolutionPreset GetResolutionLocalPS5() const;
		ChiakiVideoResolutionPreset GetResolutionRemotePS5() const;
		void SetResolutionLocalPS4(ChiakiVideoResolutionPreset resolution);
		void SetResolutionRemotePS4(ChiakiVideoResolutionPreset resolution);
		void SetResolutionLocalPS5(ChiakiVideoResolutionPreset resolution);
		void SetResolutionRemotePS5(ChiakiVideoResolutionPreset resolution);

		/**
		 * @return 0 if set to "automatic"
		 */
		ChiakiVideoFPSPreset GetFPSLocalPS4() const;
		ChiakiVideoFPSPreset GetFPSRemotePS4() const;
		ChiakiVideoFPSPreset GetFPSLocalPS5() const;
		ChiakiVideoFPSPreset GetFPSRemotePS5() const;
		void SetFPSLocalPS4(ChiakiVideoFPSPreset fps);
		void SetFPSRemotePS4(ChiakiVideoFPSPreset fps);
		void SetFPSLocalPS5(ChiakiVideoFPSPreset fps);
		void SetFPSRemotePS5(ChiakiVideoFPSPreset fps);

		unsigned int GetBitrateLocalPS4() const;
		unsigned int GetBitrateRemotePS4() const;
		unsigned int GetBitrateLocalPS5() const;
		unsigned int GetBitrateRemotePS5() const;
		void SetBitrateLocalPS4(unsigned int bitrate);
		void SetBitrateRemotePS4(unsigned int bitrate);
		void SetBitrateLocalPS5(unsigned int bitrate);
		void SetBitrateRemotePS5(unsigned int bitrate);

		ChiakiCodec GetCodecPS4() const;
		ChiakiCodec GetCodecLocalPS5() const;
		ChiakiCodec GetCodecRemotePS5() const;
		void SetCodecPS4(ChiakiCodec codec);
		void SetCodecLocalPS5(ChiakiCodec codec);
		void SetCodecRemotePS5(ChiakiCodec codec);

		int GetDisplayTargetContrast() const;
		void SetDisplayTargetContrast(int contrast);

		int GetDisplayTargetPeak() const;
		void SetDisplayTargetPeak(int peak);

		int GetDisplayTargetTrc() const;
		void SetDisplayTargetTrc(int trc);

		int GetDisplayTargetPrim() const;
		void SetDisplayTargetPrim(int prim);

		Decoder GetDecoder() const;
		void SetDecoder(Decoder decoder);

		std::string GetHardwareDecoder() const;
		void SetHardwareDecoder(const std::string &hw_decoder);

		WindowType GetWindowType() const;
		void SetWindowType(WindowType type);

		uint GetCustomResolutionWidth() const;
		void SetCustomResolutionWidth(uint width);

		uint GetCustomResolutionHeight() const;
		void SetCustomResolutionHeight(uint length);

		PlaceboPreset GetPlaceboPreset() const;
		void SetPlaceboPreset(PlaceboPreset preset);

		float GetZoomFactor() const;
		void SetZoomFactor(float factor);

		float GetPacketLossMax() const;
		void SetPacketLossMax(float factor);

        int GetAudioVolume() const;
		void SetAudioVolume(int volume);

		unsigned int GetAudioBufferSizeDefault() const;

		/**
		 * @return 0 if set to "automatic"
		 */
		unsigned int GetAudioBufferSizeRaw() const;

		/**
		 * @return actual size to be used, default value if GetAudioBufferSizeRaw() would return 0
		 */
		unsigned int GetAudioBufferSize() const;
		void SetAudioBufferSize(unsigned int size);
		
		std::string GetAudioOutDevice() const;
		void SetAudioOutDevice(std::string device_name);

		std::string GetAudioInDevice() const;
		void SetAudioInDevice(std::string device_name);

		uint GetWifiDroppedNotif() const;
		void SetWifiDroppedNotif(uint percent);

		std::string GetPsnAuthToken() const;
		void SetPsnAuthToken(std::string auth_token);

		std::string GetPsnRefreshToken() const;
		void SetPsnRefreshToken(std::string refresh_token);

		std::string GetPsnAuthTokenExpiry() const;
		void SetPsnAuthTokenExpiry(std::string expiry_date);

		std::string GetCurrentProfile() const;
		void SetCurrentProfile(std::string profile);

		bool GetDpadTouchEnabled() const;
		void SetDpadTouchEnabled(bool enabled);

		uint16_t GetDpadTouchIncrement() const;
		void SetDpadTouchIncrement(uint16_t increment);

		uint GetDpadTouchShortcut1() const;
		void SetDpadTouchShortcut1(uint button);

		uint GetDpadTouchShortcut2() const;
		void SetDpadTouchShortcut2(uint button);

		uint GetDpadTouchShortcut3() const;
		void SetDpadTouchShortcut3(uint button);

		uint GetDpadTouchShortcut4() const;
		void SetDpadTouchShortcut4(uint button);

		bool GetStreamMenuEnabled() const;
		void SetStreamMenuEnabled(bool enabled);

		uint GetStreamMenuShortcut1() const;
		void SetStreamMenuShortcut1(uint button);

		uint GetStreamMenuShortcut2() const;
		void SetStreamMenuShortcut2(uint button);

		uint GetStreamMenuShortcut3() const;
		void SetStreamMenuShortcut3(uint button);

		uint GetStreamMenuShortcut4() const;
		void SetStreamMenuShortcut4(uint button);

		void DeleteProfile(std::string profile);

		std::string GetPsnAccountId() const;
		void SetPsnAccountId(std::string account_id);

		std::string GetTimeFormat() const     { return time_format; }
		void ClearKeyMapping();

		ChiakiConnectVideoProfile GetVideoProfileLocalPS4();
		ChiakiConnectVideoProfile GetVideoProfileRemotePS4();
		ChiakiConnectVideoProfile GetVideoProfileLocalPS5();
		ChiakiConnectVideoProfile GetVideoProfileRemotePS5();

		DisconnectAction GetDisconnectAction();
		void SetDisconnectAction(DisconnectAction action);

		SuspendAction GetSuspendAction();
		void SetSuspendAction(SuspendAction action);

		PlaceboUpscaler GetPlaceboUpscaler() const;
		void SetPlaceboUpscaler(PlaceboUpscaler upscaler);

		PlaceboUpscaler GetPlaceboPlaneUpscaler() const;
		void SetPlaceboPlaneUpscaler(PlaceboUpscaler upscaler);
		
		PlaceboDownscaler GetPlaceboDownscaler() const;
		void SetPlaceboDownscaler(PlaceboDownscaler downscaler);

		PlaceboDownscaler GetPlaceboPlaneDownscaler() const;
		void SetPlaceboPlaneDownscaler(PlaceboDownscaler downscaler);

		PlaceboFrameMixer GetPlaceboFrameMixer() const;
		void SetPlaceboFrameMixer(PlaceboFrameMixer frame_mixer);

		float GetPlaceboAntiringingStrength() const;
		void SetPlaceboAntiringingStrength(float strength);

		bool GetPlaceboDebandEnabled() const;
		void SetPlaceboDebandEnabled(bool enabled);

		PlaceboDebandPreset GetPlaceboDebandPreset() const;
		void SetPlaceboDebandPreset(PlaceboDebandPreset preset);

		int GetPlaceboDebandIterations() const;
		void SetPlaceboDebandIterations(int iterations);

		float GetPlaceboDebandThreshold() const;
		void SetPlaceboDebandThreshold(float threshold);

		float GetPlaceboDebandRadius() const;
		void SetPlaceboDebandRadius(float radius);
		
		float GetPlaceboDebandGrain() const;
		void SetPlaceboDebandGrain(float grain);
		
		bool GetPlaceboSigmoidEnabled() const;
		void SetPlaceboSigmoidEnabled(bool enabled);

		PlaceboSigmoidPreset GetPlaceboSigmoidPreset() const;
		void SetPlaceboSigmoidPreset(PlaceboSigmoidPreset preset);
		
		float GetPlaceboSigmoidCenter() const;
		void SetPlaceboSigmoidCenter(float center);

		float GetPlaceboSigmoidSlope() const;
		void SetPlaceboSigmoidSlope(float slope);

		bool GetPlaceboColorAdjustmentEnabled() const;
		void SetPlaceboColorAdjustmentEnabled(bool enabled);

		PlaceboColorAdjustmentPreset GetPlaceboColorAdjustmentPreset() const;
		void SetPlaceboColorAdjustmentPreset(PlaceboColorAdjustmentPreset preset);

		float GetPlaceboColorAdjustmentBrightness() const;
		void SetPlaceboColorAdjustmentBrightness(float brightness);

		float GetPlaceboColorAdjustmentContrast() const;
		void SetPlaceboColorAdjustmentContrast(float contrast);

		float GetPlaceboColorAdjustmentSaturation() const;
		void SetPlaceboColorAdjustmentSaturation(float saturation);

		float GetPlaceboColorAdjustmentHue() const;
		void SetPlaceboColorAdjustmentHue(float hue);

		float GetPlaceboColorAdjustmentGamma() const;
		void SetPlaceboColorAdjustmentGamma(float gamma);

		float GetPlaceboColorAdjustmentTemperature() const;
		void SetPlaceboColorAdjustmentTemperature(float temperature);

		bool GetPlaceboPeakDetectionEnabled() const;
		void SetPlaceboPeakDetectionEnabled(bool enabled);

		PlaceboPeakDetectionPreset GetPlaceboPeakDetectionPreset() const;
		void SetPlaceboPeakDetectionPreset(PlaceboPeakDetectionPreset preset);

		float GetPlaceboPeakDetectionPeakSmoothingPeriod() const;
		void SetPlaceboPeakDetectionPeakSmoothingPeriod(float period);

		float GetPlaceboPeakDetectionSceneThresholdLow() const;
		void SetPlaceboPeakDetectionSceneThresholdLow(float threshold_low);

		float GetPlaceboPeakDetectionSceneThresholdHigh() const;
		void SetPlaceboPeakDetectionSceneThresholdHigh(float threshold_high);

		float GetPlaceboPeakDetectionPeakPercentile() const;
		void SetPlaceboPeakDetectionPeakPercentile(float percentile);

		float GetPlaceboPeakDetectionBlackCutoff() const;
		void SetPlaceboPeakDetectionBlackCutoff(float cutoff);

		bool GetPlaceboPeakDetectionAllowDelayedPeak() const;
		void SetPlaceboPeakDetectionAllowDelayedPeak(bool allowed);

		bool GetPlaceboColorMappingEnabled() const;
		void SetPlaceboColorMappingEnabled(bool enabled);

		PlaceboColorMappingPreset GetPlaceboColorMappingPreset() const;
		void SetPlaceboColorMappingPreset(PlaceboColorMappingPreset preset);

		PlaceboGamutMappingFunction GetPlaceboGamutMappingFunction() const;
		void SetPlaceboGamutMappingFunction(PlaceboGamutMappingFunction function);

		float GetPlaceboGamutMappingPerceptualDeadzone() const;
		void SetPlaceboGamutMappingPerceptualDeadzone(float deadzone);

		float GetPlaceboGamutMappingPerceptualStrength() const;
		void SetPlaceboGamutMappingPerceptualStrength(float strength);

		float GetPlaceboGamutMappingColorimetricGamma() const;
		void SetPlaceboGamutMappingColorimetricGamma(float gamma);

		float GetPlaceboGamutMappingSoftClipKnee() const;
		void SetPlaceboGamutMappingSoftClipKnee(float knee);

		float GetPlaceboGamutMappingSoftClipDesat() const;
		void SetPlaceboGamutMappingSoftClipDesat(float strength);

		int GetPlaceboGamutMappingLut3dSizeI() const;
		void SetPlaceboGamutMappingLut3dSizeI(int size);

		int GetPlaceboGamutMappingLut3dSizeC() const;
		void SetPlaceboGamutMappingLut3dSizeC(int size);

		int GetPlaceboGamutMappingLut3dSizeH() const;
		void SetPlaceboGamutMappingLut3dSizeH(int size);

		bool GetPlaceboGamutMappingLut3dTricubicEnabled() const;
		void SetPlaceboGamutMappingLut3dTricubicEnabled(bool enabled);

		bool GetPlaceboGamutMappingGamutExpansionEnabled() const;
		void SetPlaceboGamutMappingGamutExpansionEnabled(bool enabled);

		PlaceboToneMappingFunction GetPlaceboToneMappingFunction() const;
		void SetPlaceboToneMappingFunction(PlaceboToneMappingFunction function);

		float GetPlaceboToneMappingKneeAdaptation() const;
		void SetPlaceboToneMappingKneeAdaptation(float knee);

		float GetPlaceboToneMappingKneeMinimum() const;
		void SetPlaceboToneMappingKneeMinimum(float knee);

		float GetPlaceboToneMappingKneeMaximum() const;
		void SetPlaceboToneMappingKneeMaximum(float knee);

		float GetPlaceboToneMappingKneeDefault() const;
		void SetPlaceboToneMappingKneeDefault(float knee);

		float GetPlaceboToneMappingKneeOffset() const;
		void SetPlaceboToneMappingKneeOffset(float knee);

		float GetPlaceboToneMappingSlopeTuning() const;
		void SetPlaceboToneMappingSlopeTuning(float slope);

		float GetPlaceboToneMappingSlopeOffset() const;
		void SetPlaceboToneMappingSlopeOffset(float offset);

		float GetPlaceboToneMappingSplineContrast() const;
		void SetPlaceboToneMappingSplineContrast(float contrast);

		float GetPlaceboToneMappingReinhardContrast() const;
		void SetPlaceboToneMappingReinhardContrast(float contrast);

		float GetPlaceboToneMappingLinearKnee() const;
		void SetPlaceboToneMappingLinearKnee(float knee);

		float GetPlaceboToneMappingExposure() const;
		void SetPlaceboToneMappingExposure(float exposure);

		bool GetPlaceboToneMappingInverseToneMappingEnabled() const;
		void SetPlaceboToneMappingInverseToneMappingEnabled(bool enabled);

		PlaceboToneMappingMetadata GetPlaceboToneMappingMetadata() const;
		void SetPlaceboToneMappingMetadata(PlaceboToneMappingMetadata function);

		int GetPlaceboToneMappingToneLutSize() const;
		void SetPlaceboToneMappingToneLutSize(int size);

		float GetPlaceboToneMappingContrastRecovery() const;
		void SetPlaceboToneMappingContrastRecovery(float recovery);

		float GetPlaceboToneMappingContrastSmoothness() const;
		void SetPlaceboToneMappingContrastSmoothness(float smoothness);

		void PlaceboSettingsApply();

		std::list<RegisteredHost> GetRegisteredHosts() const
        {
            std::list<RegisteredHost> registered_host_list{registered_hosts.size()};
            for (auto &host : registered_hosts) {
                registered_host_list.push_back(host.second);
            }
            return registered_host_list;
        }
        void AddRegisteredHost(const RegisteredHost &host);
		void RemoveRegisteredHost(const HostMAC &mac);
        bool GetRegisteredHostRegistered(const HostMAC &mac) const { return registered_hosts.find(mac) != registered_hosts.end(); }
        RegisteredHost GetRegisteredHost(const HostMAC &mac) const	{ return registered_hosts.at(mac); }
		std::list<HiddenHost> GetHiddenHosts() const
        {
            std::list<HiddenHost> hidden_host_list{hidden_hosts.size()};
            for (auto &host : hidden_hosts)
            {
                hidden_host_list.push_back(host.second);
            }
            return hidden_host_list;
        }
        void AddHiddenHost(const HiddenHost &host);
		void RemoveHiddenHost(const HostMAC &mac);
        bool GetHiddenHostHidden(const HostMAC &mac) const { return hidden_hosts.find(mac) != hidden_hosts.end(); }
        HiddenHost GetHiddenHost(const HostMAC &mac) const 			{ return hidden_hosts.at(mac); }
        bool GetNicknameRegisteredHostRegistered(const std::string &nickname) const { return nickname_registered_hosts.find(nickname) != nickname_registered_hosts.end(); }
        RegisteredHost GetNicknameRegisteredHost(const std::string &nickname) const { return nickname_registered_hosts.at(nickname); }
		size_t GetPS4RegisteredHostsRegistered() const { return ps4s_registered; }
		std::list<std::string> GetProfiles() const                          { return profiles; }

		std::list<ManualHost> GetManualHosts() const
        {
            std::list<ManualHost> manual_host_list{manual_hosts.size()};
            for (auto &host : manual_hosts)
            {
                manual_host_list.push_back(host.second);
            }
            return manual_host_list;
        }
        int SetManualHost(const ManualHost &host);
		void RemoveManualHost(int id);
        bool GetManualHostExists(int id) { return manual_hosts.find(id) != manual_hosts.end(); }
        ManualHost GetManualHost(int id) const						{ return manual_hosts.at(id); }

		std::map<std::string, std::string> GetControllerMappings() const		{ return controller_mappings; }
		void SetControllerMapping(const std::string &vidpid, const std::string &mapping);
		void RemoveControllerMapping(const std::string &vidpid);

		static std::string GetChiakiControllerButtonName(int);
		void SetControllerButtonMapping(int, int);
		std::map<int, int> GetControllerMapping();
		std::map<int, int> GetControllerMappingForDecoding();

		std::function<void()> RegisteredHostsUpdated;
		std::function<void()> HiddenHostsUpdated;
		std::function<void()> ManualHostsUpdated;
		std::function<void()> ControllerMappingsUpdated;
		std::function<void()> CurrentProfileChanged;
		std::function<void()> ProfilesUpdated;
		std::function<void()> PlaceboSettingsUpdated;
};

#endif // CHIAKI_PY_SETTINGS_H
