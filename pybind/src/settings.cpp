// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include "settings.h"

#include <variant>
#include <chiaki/config.h>

#define SETTINGS_VERSION 2

void SaveHiddenHostToSettings(HiddenHost *hiddenHost, SettingsObj *settings)
{
    settings->setValue("server_nickname", hiddenHost->server_nickname);
    settings->setValue("server_mac", std::vector<uint8_t>(hiddenHost->server_mac.GetMAC(), hiddenHost->server_mac.GetMAC() + 6));
}
HiddenHost LoadHiddenHostFromSettings(SettingsObj *settings)
{
    HiddenHost r;
    r.server_nickname = settings->getValue("server_nickname");
    auto server_mac = settings->getBinaryValue("server_mac");
    if (server_mac.size() == 6)
        r.server_mac = HostMAC(server_mac.data());
    return r;
}

void SaveRegisteredHostToSettings(RegisteredHost *registeredHost, SettingsObj *settings)
{
    settings->setValue("target", static_cast<int>(registeredHost->target));
    settings->setValue("ap_ssid", registeredHost->ap_ssid);
    settings->setValue("ap_bssid", registeredHost->ap_bssid);
    settings->setValue("ap_key", registeredHost->ap_key);
    settings->setValue("ap_name", registeredHost->ap_name);
    settings->setValue("server_nickname", registeredHost->server_nickname);

    settings->setValue("server_mac", std::vector<uint8_t>(registeredHost->server_mac.GetMAC(), registeredHost->server_mac.GetMAC() + 6));
    settings->setValue("rp_regist_key", std::vector<uint8_t>(registeredHost->rp_regist_key, registeredHost->rp_regist_key + sizeof(registeredHost->rp_regist_key)));
    settings->setValue("rp_key_type", registeredHost->rp_key_type);
    settings->setValue("rp_key", std::vector<uint8_t>(registeredHost->rp_key, registeredHost->rp_key + sizeof(registeredHost->rp_key)));
    settings->setValue("console_pin", registeredHost->console_pin);
}
RegisteredHost LoadRegisteredHostFromSettings(SettingsObj *settings)
{
    RegisteredHost r;
    r.target = static_cast<ChiakiTarget>(settings->getIntValue("target"));
    r.ap_ssid = settings->getValue("ap_ssid");
    r.ap_bssid = settings->getValue("ap_bssid");
    r.ap_key = settings->getValue("ap_key");
    r.ap_name = settings->getValue("ap_name");
    r.server_nickname = settings->getValue("server_nickname");

    auto server_mac = settings->getBinaryValue("server_mac");
    if (server_mac.size() == 6)
        r.server_mac = HostMAC(server_mac.data());

    auto rp_regist_key = settings->getBinaryValue("rp_regist_key");
    if (rp_regist_key.size() == sizeof(r.rp_regist_key))
        memcpy(r.rp_regist_key, rp_regist_key.data(), sizeof(r.rp_regist_key));

    r.rp_key_type = settings->getUIntValue("rp_key_type");

    auto rp_key = settings->getBinaryValue("rp_key");
    if (rp_key.size() == sizeof(r.rp_key))
        memcpy(r.rp_key, rp_key.data(), sizeof(r.rp_key));

    r.console_pin = settings->getValue("console_pin");
    return r;
}
void SaveManualHostToSettings(ManualHost *manualHost, SettingsObj *settings)
{
    settings->setValue("id", manualHost->id);
    settings->setValue("host", manualHost->host);
    settings->setValue("registered", manualHost->registered);
    settings->setValue("registered_mac", std::vector<uint8_t>(manualHost->registered_mac.GetMAC(), manualHost->registered_mac.GetMAC() + 6));
}
ManualHost LoadManualHostFromSettings(SettingsObj *settings)
{
    ManualHost r;
    r.id = settings->getIntValue("id", -1);
    r.host = settings->getValue("host");
    r.registered = settings->getBoolValue("registered");
    auto registered_mac = settings->getBinaryValue("registered_mac");
    if (registered_mac.size() == 6)
        r.registered_mac = HostMAC(registered_mac.data());
    return r;
}

using HostVariant = std::variant<RegisteredHost, HiddenHost, ManualHost>;

static void MigrateSettingsTo2(SettingsObj *settings)
{
	std::list<std::map<std::string, HostVariant>> hosts;
	int count = settings->beginReadArray("registered_hosts");
	for(int i=0; i<count; i++)
	{
		settings->setArrayIndex(i);
		std::map<std::string, HostVariant> host;
		for(std::string k : settings->allKeys())
			host[k] = settings->value(k);
		hosts.append(host);
	}
	settings->endArray();
	settings->remove("registered_hosts");
	settings->beginWriteArray("registered_hosts");
	int i=0;
	for(const auto &host : hosts)
	{
		settings->setArrayIndex(i);
		settings->setValue("target", (int)CHIAKI_TARGET_PS4_10);
		for(auto it = host.constBegin(); it != host.constEnd(); it++)
		{
			std::string k = it.key();
			if(k == "ps4_nickname")
				k = "server_nickname";
			else if(k == "ps4_mac")
				k = "server_mac";
			settings->setValue(k, it.value());
		}
		i++;
	}
	settings->endArray();

	std::string hw_decoder = settings->value("settings/hw_decode_engine").toString();
	settings->remove("settings/hw_decode_engine");
	if(hw_decoder != "none")
		settings->setValue("settings/hw_decoder", hw_decoder);
}

static void MigrateSettings(SettingsObj *settings)
{
	int version_prev = settings->value("version", 0).toInt();
	if(version_prev < 1)
		return;
	if(version_prev > SETTINGS_VERSION)
	{
		CHIAKI_LOGE(NULL, "Settings version %d is higher than application one (%d)", version_prev, SETTINGS_VERSION);
		return;
	}
	while(version_prev < SETTINGS_VERSION)
	{
		version_prev++;
		switch(version_prev)
		{
			case 2:
				CHIAKI_LOGI(NULL, "Migrating settings to 2");
				MigrateSettingsTo2(settings);
				break;
			default:
				break;
		}
	}
}

static void InitializePlaceboSettings(SettingsObj *settings)
{
	if(settings->contains("placebo_settings/version"))
		return;
	settings->beginGroup("placebo_settings");
	settings->setValue("version", "0");
	settings->setValue("upscaler", "ewa_lanczossharp");
	settings->setValue("deband", "yes");
	settings->setValue("peak_detect_preset", "high_quality");
	settings->setValue("color_map_preset", "high_quality");
	settings->setValue("contrast_recovery", 0.3);
	settings->setValue("peak_percentile", 99.995);
	settings->endGroup();
}

static void MigrateVideoProfile(SettingsObj *settings)
{
	if(settings->contains("settings/resolution"))
	{
		settings->setValue("settings/resolution_local_ps5", settings->value("settings/resolution"));
		settings->remove("settings/resolution");
	}
	if(settings->contains("settings/fps"))
	{
		settings->setValue("settings/fps_local_ps5", settings->value("settings/fps"));
		settings->remove("settings/fps");
	}
	if(settings->contains("settings/codec"))
	{
		settings->setValue("settings/codec_local_ps5", settings->value("settings/codec"));
		settings->remove("settings/codec");
	}
	if(settings->contains("settings/bitrate"))
	{
		settings->setValue("settings/bitrate_local_ps5", settings->value("settings/bitrate"));
		settings->remove("settings/bitrate");
	}
}

static void MigrateControllerMappings(SettingsObj *settings)
{
	std::list<std::map<std::string, std::string>> mappings;
	int count = settings->beginReadArray("controller_mappings");
	for(int i=0; i<count; i++)
	{
		settings->setArrayIndex(i);
		std::map<std::string, std::string> mapping;
		for(std::string k : settings->allKeys())
			mapping.insert(k, settings->value(k).toString());
		mappings.append(mapping);
	}
	settings->endArray();

	settings->beginWriteArray("controller_mappings");
	int i=0;
	for(const auto &mapping : mappings)
	{
		settings->setArrayIndex(i);
		for(auto it = mapping.constBegin(); it != mapping.constEnd(); it++)
		{
			std::string k = it.key();
			// change old guid field to new vidpid
			if(k == "guid")
			{
				k = "vidpid";
				settings->remove("guid");
				if(settings->value("vidpid").toString().isEmpty())
					settings->setValue(k, it.value());
			}
			if(k == "vidpid")
			{
				// move decimal vid/pid to hexadecimal vid/pid
				if(it.value().contains(":") && !it.value().contains("x"))
				{
					std::stringList ids = it.value().split(u':');
					if(ids.length() == 2)
					{
						auto vid = ids.at(0).toUInt();
						auto pid = ids.at(1).toUInt();
						std::string vid_pid = std::string("0x%1:0x%2").arg(vid, 4, 16, '0').arg(pid, 4, 16, '0');
						settings->setValue(k, vid_pid);
					}
				}
			}
		}
		i++;
	}
	settings->endArray();
}

Settings::Settings(const std::string &conf) : time_format("yyyy-MM-dd HH:mm:ss t"),
	settings(),
	default_settings(),
	placebo_settings()
{
	settings.setFallbacksEnabled(false);
	MigrateSettings(&settings);
	MigrateVideoProfile(&settings);
	MigrateControllerMappings(&settings);
	manual_hosts_id_next = 0;
	settings.setValue("version", SETTINGS_VERSION);
	LoadRegisteredHosts();
	LoadHiddenHosts();
	LoadManualHosts();
	LoadControllerMappings();
	LoadProfiles();
	InitializePlaceboSettings(&placebo_settings);
}

uint32_t Settings::GetLogLevelMask()
{
	uint32_t mask = CHIAKI_LOG_ALL;
	if(!GetLogVerbose())
		mask &= ~CHIAKI_LOG_VERBOSE;
	return mask;
}

static const std::map<RumbleHapticsIntensity, std::string> intensities = {
	{ RumbleHapticsIntensity::Off, "Off" },
	{ RumbleHapticsIntensity::VeryWeak, "Very weak"},
	{ RumbleHapticsIntensity::Weak, "Weak" },
	{ RumbleHapticsIntensity::Normal, "Normal" },
	{ RumbleHapticsIntensity::Strong, "Strong" },
	{ RumbleHapticsIntensity::VeryStrong, "Very Strong" }
};

static const RumbleHapticsIntensity intensity_default = RumbleHapticsIntensity::Normal;

RumbleHapticsIntensity Settings::GetRumbleHapticsIntensity() const
{
	auto s = settings.value("settings/rumble_haptics_intensity", intensities[intensity_default]).toString();
	return intensities.key(s, intensity_default);
}

void Settings::SetRumbleHapticsIntensity(RumbleHapticsIntensity intensity)
{
	settings.setValue("settings/rumble_haptics_intensity", intensities[intensity]);
}

static const std::map<ChiakiVideoResolutionPreset, std::string> resolutions = {
	{ CHIAKI_VIDEO_RESOLUTION_PRESET_360p, "360p" },
	{ CHIAKI_VIDEO_RESOLUTION_PRESET_540p, "540p" },
	{ CHIAKI_VIDEO_RESOLUTION_PRESET_720p, "720p" },
	{ CHIAKI_VIDEO_RESOLUTION_PRESET_1080p, "1080p" }
};

static const ChiakiVideoResolutionPreset resolution_default_ps4 = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;
static const ChiakiVideoResolutionPreset resolution_default_ps5_local = CHIAKI_VIDEO_RESOLUTION_PRESET_1080p;
static const ChiakiVideoResolutionPreset resolution_default_ps5_remote = CHIAKI_VIDEO_RESOLUTION_PRESET_720p;

ChiakiVideoResolutionPreset Settings::GetResolutionLocalPS4() const
{
	auto s = settings.value("settings/resolution_local_ps4", resolutions[resolution_default_ps4]).toString();
	return resolutions.key(s, resolution_default_ps4);
}

ChiakiVideoResolutionPreset Settings::GetResolutionRemotePS4() const
{
	auto s = settings.value("settings/resolution_remote_ps4", resolutions[resolution_default_ps4]).toString();
	return resolutions.key(s, resolution_default_ps4);
}

ChiakiVideoResolutionPreset Settings::GetResolutionLocalPS5() const
{
	auto s = settings.value("settings/resolution_local_ps5", resolutions[resolution_default_ps5_local]).toString();
	return resolutions.key(s, resolution_default_ps5_local);
}

ChiakiVideoResolutionPreset Settings::GetResolutionRemotePS5() const
{
	auto s = settings.value("settings/resolution_remote_ps5", resolutions[resolution_default_ps5_remote]).toString();
	return resolutions.key(s, resolution_default_ps5_remote);
}

void Settings::SetResolutionLocalPS4(ChiakiVideoResolutionPreset resolution)
{
	settings.setValue("settings/resolution_local_ps4", resolutions[resolution]);
}

void Settings::SetResolutionRemotePS4(ChiakiVideoResolutionPreset resolution)
{
	settings.setValue("settings/resolution_remote_ps4", resolutions[resolution]);
}

void Settings::SetResolutionLocalPS5(ChiakiVideoResolutionPreset resolution)
{
	settings.setValue("settings/resolution_local_ps5", resolutions[resolution]);
}

void Settings::SetResolutionRemotePS5(ChiakiVideoResolutionPreset resolution)
{
	settings.setValue("settings/resolution_remote_ps5", resolutions[resolution]);
}

static const std::map<ChiakiVideoFPSPreset, int> fps_values = {
	{ CHIAKI_VIDEO_FPS_PRESET_30, 30 },
	{ CHIAKI_VIDEO_FPS_PRESET_60, 60 }
};

static const ChiakiVideoFPSPreset fps_default = CHIAKI_VIDEO_FPS_PRESET_60;

ChiakiVideoFPSPreset Settings::GetFPSLocalPS4() const
{
	auto v = settings.value("settings/fps_local_ps4", fps_values[fps_default]).toInt();
	return fps_values.key(v, fps_default);
}

ChiakiVideoFPSPreset Settings::GetFPSRemotePS4() const
{
	auto v = settings.value("settings/fps_remote_ps4", fps_values[fps_default]).toInt();
	return fps_values.key(v, fps_default);
}

ChiakiVideoFPSPreset Settings::GetFPSLocalPS5() const
{
	auto v = settings.value("settings/fps_local_ps5", fps_values[fps_default]).toInt();
	return fps_values.key(v, fps_default);
}

ChiakiVideoFPSPreset Settings::GetFPSRemotePS5() const
{
	auto v = settings.value("settings/fps_remote_ps5", fps_values[fps_default]).toInt();
	return fps_values.key(v, fps_default);
}

void Settings::SetFPSLocalPS4(ChiakiVideoFPSPreset fps)
{
	settings.setValue("settings/fps_local_ps4", fps_values[fps]);
}

void Settings::SetFPSRemotePS4(ChiakiVideoFPSPreset fps)
{
	settings.setValue("settings/fps_remote_ps4", fps_values[fps]);
}

void Settings::SetFPSLocalPS5(ChiakiVideoFPSPreset fps)
{
	settings.setValue("settings/fps_local_ps5", fps_values[fps]);
}

void Settings::SetFPSRemotePS5(ChiakiVideoFPSPreset fps)
{
	settings.setValue("settings/fps_remote_ps5", fps_values[fps]);
}

unsigned int Settings::GetBitrateLocalPS4() const
{
	return settings.value("settings/bitrate_local_ps4", 0).toUInt();
}

unsigned int Settings::GetBitrateRemotePS4() const
{
	return settings.value("settings/bitrate_remote_ps4", 0).toUInt();
}

unsigned int Settings::GetBitrateLocalPS5() const
{
	return settings.value("settings/bitrate_local_ps5", 0).toUInt();
}

unsigned int Settings::GetBitrateRemotePS5() const
{
	return settings.value("settings/bitrate_remote_ps5", 0).toUInt();
}

void Settings::SetBitrateLocalPS4(unsigned int bitrate)
{
	settings.setValue("settings/bitrate_local_ps4", bitrate);
}

void Settings::SetBitrateRemotePS4(unsigned int bitrate)
{
	settings.setValue("settings/bitrate_remote_ps4", bitrate);
}

void Settings::SetBitrateLocalPS5(unsigned int bitrate)
{
	settings.setValue("settings/bitrate_local_ps5", bitrate);
}

void Settings::SetBitrateRemotePS5(unsigned int bitrate)
{
	settings.setValue("settings/bitrate_remote_ps5", bitrate);
}

static const std::map<ChiakiCodec, std::string> codecs = {
	{ CHIAKI_CODEC_H264, "h264" },
	{ CHIAKI_CODEC_H265, "h265" },
	{ CHIAKI_CODEC_H265_HDR, "h265_hdr" }
};

static const ChiakiCodec codec_default_ps5 = CHIAKI_CODEC_H265;
static const ChiakiCodec codec_default_ps4 = CHIAKI_CODEC_H264;

ChiakiCodec Settings::GetCodecPS4() const
{
	auto v = settings.value("settings/codec_ps4", codecs[codec_default_ps4]).toString();
	auto codec = codecs.key(v, codec_default_ps4);
	return codec;
}

ChiakiCodec Settings::GetCodecLocalPS5() const
{
	auto v = settings.value("settings/codec_local_ps5", codecs[codec_default_ps5]).toString();
	auto codec = codecs.key(v, codec_default_ps5);
	return codec;
}

ChiakiCodec Settings::GetCodecRemotePS5() const
{
	auto v = settings.value("settings/codec_remote_ps5", codecs[codec_default_ps5]).toString();
	auto codec = codecs.key(v, codec_default_ps5);
	return codec;
}

void Settings::SetCodecPS4(ChiakiCodec codec)
{
	settings.setValue("settings/codec_ps4", codecs[codec]);
}

void Settings::SetCodecLocalPS5(ChiakiCodec codec)
{
	settings.setValue("settings/codec_local_ps5", codecs[codec]);
}

void Settings::SetCodecRemotePS5(ChiakiCodec codec)
{
	settings.setValue("settings/codec_remote_ps5", codecs[codec]);
}

unsigned int Settings::GetAudioBufferSizeDefault() const
{
	return 9600;
}

unsigned int Settings::GetAudioBufferSizeRaw() const
{
	return settings.value("settings/audio_buffer_size", 0).toUInt();
}

static const std::map<Decoder, std::string> decoder_values = {
	{ Decoder::Ffmpeg, "ffmpeg" },
	{ Decoder::Pi, "pi" }
};

static const Decoder decoder_default = Decoder::Pi;

Decoder Settings::GetDecoder() const
{
#if CHIAKI_LIB_ENABLE_PI_DECODER
	auto v = settings.value("settings/decoder", decoder_values[decoder_default]).toString();
	return decoder_values.key(v, decoder_default);
#else
	return Decoder::Ffmpeg;
#endif
}

void Settings::SetDecoder(Decoder decoder)
{
	settings.setValue("settings/decoder", decoder_values[decoder]);
}

static const std::map<PlaceboPreset, std::string> placebo_preset_values = {
	{ PlaceboPreset::Fast, "fast" },
	{ PlaceboPreset::Default, "default" },
	{ PlaceboPreset::HighQuality, "high_quality" },
	{ PlaceboPreset::Custom, "custom" }
};

static const PlaceboPreset placebo_preset_default = PlaceboPreset::HighQuality;

PlaceboPreset Settings::GetPlaceboPreset() const
{
	auto v = settings.value("settings/placebo_preset", placebo_preset_values[placebo_preset_default]).toString();
	return placebo_preset_values.key(v, placebo_preset_default);
}

void Settings::SetPlaceboPreset(PlaceboPreset preset)
{
	settings.setValue("settings/placebo_preset", placebo_preset_values[preset]);
}

float Settings::GetZoomFactor() const
{
	return settings.value("settings/zoom_factor", -1).toFloat();
}

void Settings::SetZoomFactor(float factor)
{
	settings.setValue("settings/zoom_factor", std::string("%1").arg(factor, 0, 'f', 2));
}

float Settings::GetPacketLossMax() const
{
	return settings.value("settings/packet_loss_max", 0.05).toFloat();
}

void Settings::SetPacketLossMax(float packet_loss_max)
{
	settings.setValue("settings/packet_loss_max", std::string("%1").arg(packet_loss_max, 0, 'f', 2));
}

static const std::map<WindowType, std::string> window_type_values = {
	{ WindowType::SelectedResolution, "Selected Resolution" },
	{ WindowType::CustomResolution, "Custom Resolution"},
	{ WindowType::AdjustableResolution, "Adjust Manually"},
	{ WindowType::Fullscreen, "Fullscreen" },
	{ WindowType::Zoom, "Zoom" },
	{ WindowType::Stretch, "Stretch" }
};

WindowType Settings::GetWindowType() const
{
	auto v = settings.value("settings/window_type", window_type_values[WindowType::SelectedResolution]).toString();
	return window_type_values.key(v, WindowType::SelectedResolution);
}

void Settings::SetWindowType(WindowType type)
{
	settings.setValue("settings/window_type", window_type_values[type]);
}

uint Settings::GetCustomResolutionWidth() const
{
	return settings.value("settings/custom_resolution_width", 1920).toUInt();
}

void Settings::SetCustomResolutionWidth(uint width)
{
	settings.setValue("settings/custom_resolution_width", width);
}

uint Settings::GetCustomResolutionHeight() const
{
	return settings.value("settings/custom_resolution_length", 1080).toUInt();
}

void Settings::SetCustomResolutionHeight(uint length)
{
	settings.setValue("settings/custom_resolution_length", length);
}

std::string Settings::GetHardwareDecoder() const
{
	return settings.value("settings/hw_decoder", "auto").toString();
}

void Settings::SetHardwareDecoder(const std::string &hw_decoder)
{
	settings.setValue("settings/hw_decoder", hw_decoder);
}

void Settings::SetAudioVolume(int volume)
{
	settings.setValue("settings/audio_volume", volume);
}

unsigned int Settings::GetAudioBufferSize() const
{
	unsigned int v = GetAudioBufferSizeRaw();
	return v ? v : GetAudioBufferSizeDefault();
}

std::string Settings::GetAudioOutDevice() const
{
	return settings.value("settings/audio_out_device").toString();
}

std::string Settings::GetAudioInDevice() const
{
	return settings.value("settings/audio_in_device").toString();
}

void Settings::SetAudioOutDevice(std::string device_name)
{
	settings.setValue("settings/audio_out_device", device_name);
}

void Settings::SetAudioInDevice(std::string device_name)
{
	settings.setValue("settings/audio_in_device", device_name);
}

void Settings::SetAudioBufferSize(unsigned int size)
{
	settings.setValue("settings/audio_buffer_size", size);
}

unsigned int Settings::GetWifiDroppedNotif() const
{
	return settings.value("settings/wifi_dropped_notif_percent", 3).toUInt();
}

void Settings::SetWifiDroppedNotif(unsigned int percent)
{
	settings.setValue("settings/wifi_dropped_notif_percent", percent);
}

#if CHIAKI_GUI_ENABLE_SPEEX

bool Settings::GetSpeechProcessingEnabled() const
{
	return settings.value("settings/enable_speech_processing", false).toBool();
}

int Settings::GetNoiseSuppressLevel() const
{
	return settings.value("settings/noise_suppress_level", 6).toInt();
}

int Settings::GetEchoSuppressLevel() const
{
	return settings.value("settings/echo_suppress_level", 30).toInt();
}

void Settings::SetSpeechProcessingEnabled(bool enabled)
{
	settings.setValue("settings/enable_speech_processing", enabled);
}

void Settings::SetNoiseSuppressLevel(int reduceby)
{
	settings.setValue("settings/noise_suppress_level", reduceby);
}

void Settings::SetEchoSuppressLevel(int reduceby)
{
	settings.setValue("settings/echo_suppress_level", reduceby);
}
#endif

std::string Settings::GetPsnAuthToken() const
{
	return settings.value("settings/psn_auth_token").toString();
}

void Settings::SetPsnAuthToken(std::string auth_token)
{
	settings.setValue("settings/psn_auth_token", auth_token);
}

std::string Settings::GetPsnRefreshToken() const
{
	return settings.value("settings/psn_refresh_token").toString();
}

void Settings::SetPsnRefreshToken(std::string refresh_token)
{
	settings.setValue("settings/psn_refresh_token", refresh_token);
}

std::string Settings::GetPsnAuthTokenExpiry() const
{
	return settings.value("settings/psn_auth_token_expiry").toString();
}

void Settings::SetPsnAuthTokenExpiry(std::string expiry_date)
{
	settings.setValue("settings/psn_auth_token_expiry", expiry_date);
}

void Settings::SetCurrentProfile(std::string profile)
{
	if(!profile.isEmpty() && !profiles.contains(profile))
	{
		profiles.append(profile);
		SaveProfiles();
		emit ProfilesUpdated();
	}
	emit CurrentProfileChanged();
}

bool Settings::GetDpadTouchEnabled() const
{
	return settings.value("settings/dpad_touch_enabled", true).toBool();
}

void Settings::SetDpadTouchEnabled(bool enabled)
{
	settings.setValue("settings/dpad_touch_enabled", enabled);
}

uint16_t Settings::GetDpadTouchIncrement() const
{
	return settings.value("settings/dpad_touch_increment", 30).toUInt();
}

void Settings::SetDpadTouchIncrement(uint16_t increment)
{
	settings.setValue("settings/dpad_touch_increment", increment);
}

uint Settings::GetDpadTouchShortcut1() const {
	return settings.value("settings/dpad_touch_shortcut1", 9).toUInt();
}
void Settings::SetDpadTouchShortcut1(uint button) {
	settings.setValue("settings/dpad_touch_shortcut1", button);
}

uint Settings::GetDpadTouchShortcut2() const {
	return settings.value("settings/dpad_touch_shortcut2", 10).toUInt();
}
void Settings::SetDpadTouchShortcut2(uint button) {
	settings.setValue("settings/dpad_touch_shortcut2", button);
}

uint Settings::GetDpadTouchShortcut3() const {
	return settings.value("settings/dpad_touch_shortcut3", 7).toUInt();
}
void Settings::SetDpadTouchShortcut3(uint button) {
	settings.setValue("settings/dpad_touch_shortcut3", button);
}

uint Settings::GetDpadTouchShortcut4() const {
	return settings.value("settings/dpad_touch_shortcut4", 0).toUInt();
}
void Settings::SetDpadTouchShortcut4(uint button) {
	settings.setValue("settings/dpad_touch_shortcut4", button);
}

bool Settings::GetStreamMenuEnabled() const
{
	return settings.value("settings/stream_menu_enabled", true).toBool();
}

void Settings::SetStreamMenuEnabled(bool enabled)
{
	settings.setValue("settings/stream_menu_enabled", enabled);
}

uint Settings::GetStreamMenuShortcut1() const {
	return settings.value("settings/stream_menu_shortcut1", 9).toUInt();
}
void Settings::SetStreamMenuShortcut1(uint button) {
	settings.setValue("settings/stream_menu_shortcut1", button);
}

uint Settings::GetStreamMenuShortcut2() const {
	return settings.value("settings/stream_menu_shortcut2", 10).toUInt();
}
void Settings::SetStreamMenuShortcut2(uint button) {
	settings.setValue("settings/stream_menu_shortcut2", button);
}

uint Settings::GetStreamMenuShortcut3() const {
	return settings.value("settings/stream_menu_shortcut3", 11).toUInt();
}
void Settings::SetStreamMenuShortcut3(uint button) {
	settings.setValue("settings/stream_menu_shortcut3", button);
}

uint Settings::GetStreamMenuShortcut4() const {
	return settings.value("settings/stream_menu_shortcut4", 12).toUInt();
}
void Settings::SetStreamMenuShortcut4(uint button) {
	settings.setValue("settings/stream_menu_shortcut4", button);
}

std::string Settings::GetPsnAccountId() const
{
	return settings.value("settings/psn_account_id").toString();
}

void Settings::SetPsnAccountId(std::string account_id)
{
	settings.setValue("settings/psn_account_id", account_id);
}

ChiakiConnectVideoProfile Settings::GetVideoProfileLocalPS4()
{
	ChiakiConnectVideoProfile profile = {};
	chiaki_connect_video_profile_preset(&profile, GetResolutionLocalPS4(), GetFPSLocalPS4());
	unsigned int bitrate = GetBitrateLocalPS4();
	if(bitrate)
		profile.bitrate = bitrate;
	profile.codec = GetCodecPS4();
	return profile;
}

ChiakiConnectVideoProfile Settings::GetVideoProfileRemotePS4()
{
	ChiakiConnectVideoProfile profile = {};
	chiaki_connect_video_profile_preset(&profile, GetResolutionRemotePS4(), GetFPSRemotePS4());
	unsigned int bitrate = GetBitrateRemotePS4();
	if(bitrate)
		profile.bitrate = bitrate;
	profile.codec = GetCodecPS4();
	return profile;
}

ChiakiConnectVideoProfile Settings::GetVideoProfileLocalPS5()
{
	ChiakiConnectVideoProfile profile = {};
	chiaki_connect_video_profile_preset(&profile, GetResolutionLocalPS5(), GetFPSLocalPS5());
	unsigned int bitrate = GetBitrateLocalPS5();
	if(bitrate)
		profile.bitrate = bitrate;
	profile.codec = GetCodecLocalPS5();
	return profile;
}

ChiakiConnectVideoProfile Settings::GetVideoProfileRemotePS5()
{
	ChiakiConnectVideoProfile profile = {};
	chiaki_connect_video_profile_preset(&profile, GetResolutionRemotePS5(), GetFPSRemotePS5());
	unsigned int bitrate = GetBitrateRemotePS5();
	if(bitrate)
		profile.bitrate = bitrate;
	profile.codec = GetCodecRemotePS5();
	return profile;
}

static const std::map<DisconnectAction, std::string> disconnect_action_values = {
	{ DisconnectAction::Ask, "ask" },
	{ DisconnectAction::AlwaysNothing, "nothing" },
	{ DisconnectAction::AlwaysSleep, "sleep" }
};

static const DisconnectAction disconnect_action_default = DisconnectAction::Ask;

DisconnectAction Settings::GetDisconnectAction()
{
	auto v = settings.value("settings/disconnect_action", disconnect_action_values[disconnect_action_default]).toString();
	return disconnect_action_values.key(v, disconnect_action_default);
}

void Settings::SetDisconnectAction(DisconnectAction action)
{
	settings.setValue("settings/disconnect_action", disconnect_action_values[action]);
}

static const std::map<SuspendAction, std::string> suspend_action_values = {
	{ SuspendAction::Sleep, "sleep" },
	{ SuspendAction::Nothing, "nothing" },
};

static const SuspendAction suspend_action_default = SuspendAction::Nothing;

SuspendAction Settings::GetSuspendAction()
{
	auto v = settings.value("settings/suspend_action", suspend_action_values[suspend_action_default]).toString();
	return suspend_action_values.key(v, suspend_action_default);
}

void Settings::SetSuspendAction(SuspendAction action)
{
	settings.setValue("settings/suspend_action", suspend_action_values[action]);
}

int Settings::GetDisplayTargetContrast() const
{
	return settings.value("settings/display_target_contrast", 0).toInt();
}

void Settings::SetDisplayTargetContrast(int contrast)
{
	settings.setValue("settings/display_target_contrast", contrast);
}

int Settings::GetDisplayTargetPeak() const
{
	return settings.value("settings/display_target_peak", 0).toInt();
}

void Settings::SetDisplayTargetPeak(int peak)
{
	settings.setValue("settings/display_target_peak", peak);
}

int Settings::GetDisplayTargetTrc() const
{
	return settings.value("settings/display_target_trc", 0).toInt();
}

void Settings::SetDisplayTargetTrc(int trc)
{
	settings.setValue("settings/display_target_trc", trc);
}

int Settings::GetDisplayTargetPrim() const
{
	return settings.value("settings/display_target_prim", 0).toInt();
}

void Settings::SetDisplayTargetPrim(int prim)
{
	settings.setValue("settings/display_target_prim", prim);
}


static const std::map<PlaceboUpscaler, std::string> placebo_upscaler_values = {
	{ PlaceboUpscaler::None, "none" },
	{ PlaceboUpscaler::Nearest, "nearest" },
	{ PlaceboUpscaler::Bilinear, "bilinear" },
	{ PlaceboUpscaler::Oversample, "oversample" },
	{ PlaceboUpscaler::Bicubic, "bicubic" },
	{ PlaceboUpscaler::Gaussian, "gaussian" },
	{ PlaceboUpscaler::CatmullRom, "catmull_rom" },
	{ PlaceboUpscaler::Lanczos, "lanczos" },
	{ PlaceboUpscaler::EwaLanczos, "ewa_lanczos" },
	{ PlaceboUpscaler::EwaLanczosSharp, "ewa_lanczossharp" },
	{ PlaceboUpscaler::EwaLanczos4Sharpest, "ewa_lanczos4sharpest" },
};

static const PlaceboUpscaler placebo_upscaler_default = PlaceboUpscaler::Lanczos;

PlaceboUpscaler Settings::GetPlaceboUpscaler() const
{
	auto v = placebo_settings.value("placebo_settings/upscaler", placebo_upscaler_values[placebo_upscaler_default]).toString();
	return placebo_upscaler_values.key(v, placebo_upscaler_default);
}

void Settings::SetPlaceboUpscaler(PlaceboUpscaler upscaler)
{
	placebo_settings.setValue("placebo_settings/upscaler", placebo_upscaler_values[upscaler]);
}

static const PlaceboUpscaler placebo_plane_upscaler_default = PlaceboUpscaler::None;

PlaceboUpscaler Settings::GetPlaceboPlaneUpscaler() const
{
	auto v = placebo_settings.value("placebo_settings/plane_upscaler", placebo_upscaler_values[placebo_plane_upscaler_default]).toString();
	return placebo_upscaler_values.key(v, placebo_plane_upscaler_default);
}

void Settings::SetPlaceboPlaneUpscaler(PlaceboUpscaler upscaler)
{
	placebo_settings.setValue("placebo_settings/plane_upscaler", placebo_upscaler_values[upscaler]);
}

static const std::map<PlaceboDownscaler, std::string> placebo_downscaler_values = {
	{ PlaceboDownscaler::None, "none" },
	{ PlaceboDownscaler::Box, "box" },
	{ PlaceboDownscaler::Hermite, "hermite" },
	{ PlaceboDownscaler::Bilinear, "bilinear" },
	{ PlaceboDownscaler::Bicubic, "bicubic" },
	{ PlaceboDownscaler::Gaussian, "gaussian" },
	{ PlaceboDownscaler::CatmullRom, "catmull_rom" },
	{ PlaceboDownscaler::Mitchell, "mitchell" },
	{ PlaceboDownscaler::Lanczos, "lanczos" },
};

static const PlaceboDownscaler placebo_downscaler_default = PlaceboDownscaler::Hermite;

PlaceboDownscaler Settings::GetPlaceboDownscaler() const
{
	auto v = placebo_settings.value("placebo_settings/downscaler", placebo_downscaler_values[placebo_downscaler_default]).toString();
	return placebo_downscaler_values.key(v, placebo_downscaler_default);
}

void Settings::SetPlaceboDownscaler(PlaceboDownscaler downscaler)
{
	placebo_settings.setValue("placebo_settings/downscaler", placebo_downscaler_values[downscaler]);
}

static const PlaceboDownscaler placebo_plane_downscaler_default = PlaceboDownscaler::None;

PlaceboDownscaler Settings::GetPlaceboPlaneDownscaler() const
{
	auto v = placebo_settings.value("placebo_settings/plane_downscaler", placebo_downscaler_values[placebo_plane_downscaler_default]).toString();
	return placebo_downscaler_values.key(v, placebo_plane_downscaler_default);
}

void Settings::SetPlaceboPlaneDownscaler(PlaceboDownscaler downscaler)
{
	placebo_settings.setValue("placebo_settings/plane_downscaler", placebo_downscaler_values[downscaler]);
}

static const std::map<PlaceboFrameMixer, std::string> placebo_frame_mixer_values = {
	{ PlaceboFrameMixer::None, "none" },
	{ PlaceboFrameMixer::Oversample, "oversample" },
	{ PlaceboFrameMixer::Hermite, "hermite" },
	{ PlaceboFrameMixer::Linear, "linear" },
	{ PlaceboFrameMixer::Cubic, "cubic" },
};

static const PlaceboFrameMixer placebo_frame_mixer_default = PlaceboFrameMixer::Oversample;

PlaceboFrameMixer Settings::GetPlaceboFrameMixer() const
{
	auto v = placebo_settings.value("placebo_settings/frame_mixer", placebo_frame_mixer_values[placebo_frame_mixer_default]).toString();
	return placebo_frame_mixer_values.key(v, placebo_frame_mixer_default);
}

void Settings::SetPlaceboFrameMixer(PlaceboFrameMixer frame_mixer)
{
	placebo_settings.setValue("placebo_settings/frame_mixer", placebo_frame_mixer_values[frame_mixer]);
}

float Settings::GetPlaceboAntiringingStrength() const
{
	return placebo_settings.value("placebo_settings/antiringing_strength", 0.0).toFloat();
}

void Settings::SetPlaceboAntiringingStrength(float strength)
{
	placebo_settings.setValue("placebo_settings/antiringing_strength", std::string("%1").arg(strength, 0, 'f', 2));
}

bool Settings::GetPlaceboDebandEnabled() const
{
	std::string value = placebo_settings.value("placebo_settings/deband", "yes").toString();
	bool enabled;
	if(value == "yes")
		enabled = true;
	else
		enabled = false;
	return enabled;
}

void Settings::SetPlaceboDebandEnabled(bool enabled)
{
	if(enabled)
		placebo_settings.setValue("placebo_settings/deband", "yes");
	else
		placebo_settings.setValue("placebo_settings/inverse_tone_mapping", "no");
}

static const std::map<PlaceboDebandPreset, std::string> placebo_deband_preset_values = {
	{ PlaceboDebandPreset::None, "" },
	{ PlaceboDebandPreset::Default, "default" },
};

static const PlaceboDebandPreset placebo_deband_preset_default = PlaceboDebandPreset::None;

PlaceboDebandPreset Settings::GetPlaceboDebandPreset() const
{
	auto v = placebo_settings.value("placebo_settings/deband_preset", placebo_deband_preset_values[placebo_deband_preset_default]).toString();
	return placebo_deband_preset_values.key(v, placebo_deband_preset_default);
}

void Settings::SetPlaceboDebandPreset(PlaceboDebandPreset preset)
{
	if(placebo_deband_preset_values[preset].isEmpty())
		placebo_settings.remove("placebo_settings/deband_preset");
	else
		placebo_settings.setValue("placebo_settings/deband_preset", placebo_deband_preset_values[preset]);
}

int Settings::GetPlaceboDebandIterations() const
{
	return placebo_settings.value("placebo_settings/deband_iterations", 1).toInt();
}

void Settings::SetPlaceboDebandIterations(int iterations)
{
	placebo_settings.setValue("placebo_settings/deband_iterations", iterations);
}

float Settings::GetPlaceboDebandThreshold() const
{
	return placebo_settings.value("placebo_settings/deband_threshold", 3.0).toFloat();
}

void Settings::SetPlaceboDebandThreshold(float threshold)
{
	placebo_settings.setValue("placebo_settings/deband_threshold",std::string("%1").arg(threshold, 0, 'f', 1));
}

float Settings::GetPlaceboDebandRadius() const
{
	return placebo_settings.value("placebo_settings/deband_radius", 16.0).toFloat();
}

void Settings::SetPlaceboDebandRadius(float radius)
{
	placebo_settings.setValue("placebo_settings/deband_radius", std::string("%1").arg(radius, 0, 'f', 1));
}

float Settings::GetPlaceboDebandGrain() const
{
	return placebo_settings.value("placebo_settings/deband_grain", 4.0).toFloat();
}

void Settings::SetPlaceboDebandGrain(float grain)
{
	placebo_settings.setValue("placebo_settings/deband_grain", std::string("%1").arg(grain, 0, 'f', 1));
}

bool Settings::GetPlaceboSigmoidEnabled() const
{
	std::string value = placebo_settings.value("placebo_settings/sigmoid", "yes").toString();
	bool enabled;
	if(value == "yes")
		enabled = true;
	else
		enabled = false;
	return enabled;
}

void Settings::SetPlaceboSigmoidEnabled(bool enabled)
{
	if(enabled)
		placebo_settings.setValue("placebo_settings/sigmoid", "yes");
	else
		placebo_settings.setValue("placebo_settings/sigmoid", "no");
}

static const std::map<PlaceboSigmoidPreset, std::string> placebo_sigmoid_preset_values = {
	{ PlaceboSigmoidPreset::None, "" },
	{ PlaceboSigmoidPreset::Default, "default" },
};

static const PlaceboSigmoidPreset placebo_sigmoid_preset_default = PlaceboSigmoidPreset::None;

PlaceboSigmoidPreset Settings::GetPlaceboSigmoidPreset() const
{
	auto v = placebo_settings.value("placebo_settings/sigmoid_preset", placebo_sigmoid_preset_values[placebo_sigmoid_preset_default]).toString();
	return placebo_sigmoid_preset_values.key(v, placebo_sigmoid_preset_default);
}

void Settings::SetPlaceboSigmoidPreset(PlaceboSigmoidPreset preset)
{
	if(placebo_sigmoid_preset_values[preset].isEmpty())
		placebo_settings.remove("placebo_settings/sigmoid_preset");
	else
		placebo_settings.setValue("placebo_settings/sigmoid_preset", placebo_sigmoid_preset_values[preset]);
}

float Settings::GetPlaceboSigmoidCenter() const
{
	return placebo_settings.value("placebo_settings/sigmoid_center", 0.75).toFloat();
}

void Settings::SetPlaceboSigmoidCenter(float center)
{
	placebo_settings.setValue("placebo_settings/sigmoid_center", std::string("%1").arg(center, 0, 'f', 2));
}

float Settings::GetPlaceboSigmoidSlope() const
{
	return placebo_settings.value("placebo_settings/sigmoid_slope", 6.5).toFloat();
}

void Settings::SetPlaceboSigmoidSlope(float slope)
{
	placebo_settings.setValue("placebo_settings/sigmoid_slope", std::string("%1").arg(slope, 0, 'f', 1));
}

bool Settings::GetPlaceboColorAdjustmentEnabled() const
{
	std::string value = placebo_settings.value("placebo_settings/color_adjustment", "yes").toString();
	bool enabled;
	if(value == "yes")
		enabled = true;
	else
		enabled = false;
	return enabled;
}

void Settings::SetPlaceboColorAdjustmentEnabled(bool enabled)
{
	if(enabled)
		placebo_settings.setValue("placebo_settings/color_adjustment", "yes");
	else
		placebo_settings.setValue("placebo_settings/color_adjustment", "no");
}

static const std::map<PlaceboColorAdjustmentPreset, std::string> placebo_color_adjustment_preset_values = {
	{ PlaceboColorAdjustmentPreset::None, "" },
	{ PlaceboColorAdjustmentPreset::Neutral, "neutral" },
};

static const PlaceboColorAdjustmentPreset placebo_color_adjustment_preset_default = PlaceboColorAdjustmentPreset::None;

PlaceboColorAdjustmentPreset Settings::GetPlaceboColorAdjustmentPreset() const
{
	auto v = placebo_settings.value("placebo_settings/color_adjustment_preset", placebo_color_adjustment_preset_values[placebo_color_adjustment_preset_default]).toString();
	return placebo_color_adjustment_preset_values.key(v, placebo_color_adjustment_preset_default);
}

void Settings::SetPlaceboColorAdjustmentPreset(PlaceboColorAdjustmentPreset preset)
{
	if(placebo_color_adjustment_preset_values[preset].isEmpty())
		placebo_settings.remove("placebo_settings/color_adjustment_preset");
	else
		placebo_settings.setValue("placebo_settings/color_adjustment_preset", placebo_color_adjustment_preset_values[preset]);
}

float Settings::GetPlaceboColorAdjustmentBrightness() const
{
	return placebo_settings.value("placebo_settings/brightness", 0.0).toFloat();
}

void Settings::SetPlaceboColorAdjustmentBrightness(float brightness)
{
	placebo_settings.setValue("placebo_settings/brightness", std::string("%1").arg(brightness, 0, 'f', 2));
}

float Settings::GetPlaceboColorAdjustmentContrast() const
{
	return placebo_settings.value("placebo_settings/contrast", 1.0).toFloat();
}

void Settings::SetPlaceboColorAdjustmentContrast(float contrast)
{
	placebo_settings.setValue("placebo_settings/contrast", std::string("%1").arg(contrast, 0, 'f', 1));
}

float Settings::GetPlaceboColorAdjustmentSaturation() const
{
	return placebo_settings.value("placebo_settings/saturation", 1.0).toFloat();
}

void Settings::SetPlaceboColorAdjustmentSaturation(float saturation)
{
	placebo_settings.setValue("placebo_settings/saturation", std::string("%1").arg(saturation, 0, 'f', 2));
}

float Settings::GetPlaceboColorAdjustmentHue() const
{
	return placebo_settings.value("placebo_settings/hue", 0.0).toFloat();
}

void Settings::SetPlaceboColorAdjustmentHue(float hue)
{
	placebo_settings.setValue("placebo_settings/hue", std::string("%1").arg(hue, 0, 'f', 2));
}

float Settings::GetPlaceboColorAdjustmentGamma() const
{
	return placebo_settings.value("placebo_settings/gamma", 1.0).toFloat();
}

void Settings::SetPlaceboColorAdjustmentGamma(float gamma)
{
	placebo_settings.setValue("placebo_settings/gamma", std::string("%1").arg(gamma, 0, 'f', 1));
}

float Settings::GetPlaceboColorAdjustmentTemperature() const
{
	return placebo_settings.value("placebo_settings/temperature", 0.0).toFloat();
}

void Settings::SetPlaceboColorAdjustmentTemperature(float temperature)
{
	placebo_settings.setValue("placebo_settings/temperature", std::string("%1").arg(temperature, 0, 'f', 3));
}

bool Settings::GetPlaceboPeakDetectionEnabled() const
{
	std::string value = placebo_settings.value("placebo_settings/peak_detect", "yes").toString();
	bool enabled;
	if(value == "yes")
		enabled = true;
	else
		enabled = false;
	return enabled;
}

void Settings::SetPlaceboPeakDetectionEnabled(bool enabled)
{
	if(enabled)
		placebo_settings.setValue("placebo_settings/peak_detect", "yes");
	else
		placebo_settings.setValue("placebo_settings/peak_detect", "no");
}

static const std::map<PlaceboPeakDetectionPreset, std::string> placebo_peak_detection_preset_values = {
	{ PlaceboPeakDetectionPreset::None, "" },
	{ PlaceboPeakDetectionPreset::Default, "default" },
	{ PlaceboPeakDetectionPreset::HighQuality, "high_quality" },
};

static const PlaceboPeakDetectionPreset placebo_peak_detection_preset_default = PlaceboPeakDetectionPreset::None;

PlaceboPeakDetectionPreset Settings::GetPlaceboPeakDetectionPreset() const
{
	auto v = placebo_settings.value("placebo_settings/peak_detect_preset", placebo_peak_detection_preset_values[placebo_peak_detection_preset_default]).toString();
	return placebo_peak_detection_preset_values.key(v, placebo_peak_detection_preset_default);
}

void Settings::SetPlaceboPeakDetectionPreset(PlaceboPeakDetectionPreset preset)
{
	if(placebo_peak_detection_preset_values[preset].isEmpty())
		placebo_settings.remove("placebo_settings/peak_detect_preset");
	else
		placebo_settings.setValue("placebo_settings/peak_detect_preset", placebo_peak_detection_preset_values[preset]);
}

float Settings::GetPlaceboPeakDetectionPeakSmoothingPeriod() const
{
	return placebo_settings.value("placebo_settings/peak_smoothing_period", 20.0).toFloat();
}

void Settings::SetPlaceboPeakDetectionPeakSmoothingPeriod(float period)
{
	placebo_settings.setValue("placebo_settings/peak_smoothing_period", std::string("%1").arg(period, 0, 'f', 1));
}

float Settings::GetPlaceboPeakDetectionSceneThresholdLow() const
{
	return placebo_settings.value("placebo_settings/scene_threshold_low", 1.0).toFloat();
}

void Settings::SetPlaceboPeakDetectionSceneThresholdLow(float threshold_low)
{
	placebo_settings.setValue("placebo_settings/scene_threshold_low", std::string("%1").arg(threshold_low, 0, 'f', 1));
}

float Settings::GetPlaceboPeakDetectionSceneThresholdHigh() const
{
	return placebo_settings.value("placebo_settings/scene_threshold_high", 3.0).toFloat();
}

void Settings::SetPlaceboPeakDetectionSceneThresholdHigh(float threshold_high)
{
	placebo_settings.setValue("placebo_settings/scene_threshold_high", std::string("%1").arg(threshold_high, 0, 'f', 1));
}

float Settings::GetPlaceboPeakDetectionPeakPercentile() const
{
	return placebo_settings.value("placebo_settings/peak_percentile", 100.0).toFloat();
}

void Settings::SetPlaceboPeakDetectionPeakPercentile(float peak)
{
	placebo_settings.setValue("placebo_settings/peak_percentile", std::string("%1").arg(peak, 0, 'f', 3));
}

float Settings::GetPlaceboPeakDetectionBlackCutoff() const
{
	return placebo_settings.value("placebo_settings/black_cutoff", 1.0).toFloat();
}

void Settings::SetPlaceboPeakDetectionBlackCutoff(float cutoff)
{
	placebo_settings.setValue("placebo_settings/black_cutoff", std::string("%1").arg(cutoff, 0, 'f', 1));
}

bool Settings::GetPlaceboPeakDetectionAllowDelayedPeak() const
{
	bool allowed;
	std::string value = placebo_settings.value("placebo_settings/allow_delayed_peak", "no").toString();
	if(value == "yes")
		allowed = true;
	else
		allowed = false;
	return allowed;
}

void Settings::SetPlaceboPeakDetectionAllowDelayedPeak(bool allowed)
{
	if(allowed)
		placebo_settings.setValue("placebo_settings/allow_delayed_peak", "yes");
	else
		placebo_settings.setValue("placebo_settings/allow_delayed_peak", "no");
}

bool Settings::GetPlaceboColorMappingEnabled() const
{
	bool enabled;
	std::string value = placebo_settings.value("placebo_settings/color_map", "yes").toString();
	if(value == "yes")
		enabled = true;
	else
		enabled = false;
	return enabled;
}

void Settings::SetPlaceboColorMappingEnabled(bool enabled)
{
	if(enabled)
		placebo_settings.setValue("placebo_settings/color_map", "yes");
	else
		placebo_settings.setValue("placebo_settings/color_map", "no");
}

static const std::map<PlaceboColorMappingPreset, std::string> placebo_color_mapping_preset_values = {
	{ PlaceboColorMappingPreset::None, "" },
	{ PlaceboColorMappingPreset::Default, "default" },
	{ PlaceboColorMappingPreset::HighQuality, "high_quality" },
};

static const PlaceboColorMappingPreset placebo_color_mapping_default = PlaceboColorMappingPreset::None;

PlaceboColorMappingPreset Settings::GetPlaceboColorMappingPreset() const
{
	auto v = placebo_settings.value("placebo_settings/color_map_preset", placebo_color_mapping_preset_values[placebo_color_mapping_default]).toString();
	return placebo_color_mapping_preset_values.key(v, placebo_color_mapping_default);
}

void Settings::SetPlaceboColorMappingPreset(PlaceboColorMappingPreset preset)
{
	if(placebo_color_mapping_preset_values[preset].isEmpty())
		placebo_settings.remove("placebo_settings/color_map_preset");
	else
		placebo_settings.setValue("placebo_settings/color_map_preset", placebo_color_mapping_preset_values[preset]);
}

static const std::map<PlaceboGamutMappingFunction, std::string> placebo_gamut_mapping_function_values = {
	{ PlaceboGamutMappingFunction::Clip, "clip" },
	{ PlaceboGamutMappingFunction::Perceptual, "perceptual" },
	{ PlaceboGamutMappingFunction::SoftClip, "softclip" },
	{ PlaceboGamutMappingFunction::Relative, "relative" },
	{ PlaceboGamutMappingFunction::Saturation, "saturation" },
	{ PlaceboGamutMappingFunction::Absolute, "absolute" },
	{ PlaceboGamutMappingFunction::Desaturate, "desaturate" },
	{ PlaceboGamutMappingFunction::Darken, "darken" },
	{ PlaceboGamutMappingFunction::Highlight, "highlight" },
	{ PlaceboGamutMappingFunction::Linear, "linear" },
};

static const PlaceboGamutMappingFunction placebo_gamut_mapping_function_default = PlaceboGamutMappingFunction::Perceptual;

PlaceboGamutMappingFunction Settings::GetPlaceboGamutMappingFunction() const
{
	auto v = placebo_settings.value("placebo_settings/gamut_mapping", placebo_gamut_mapping_function_values[placebo_gamut_mapping_function_default]).toString();
	return placebo_gamut_mapping_function_values.key(v, placebo_gamut_mapping_function_default);
}

void Settings::SetPlaceboGamutMappingFunction(PlaceboGamutMappingFunction function)
{
	placebo_settings.setValue("placebo_settings/gamut_mapping", placebo_gamut_mapping_function_values[function]);
}

float Settings::GetPlaceboGamutMappingPerceptualDeadzone() const
{
	return placebo_settings.value("placebo_settings/perceptual_deadzone", 0.30).toFloat();
}

void Settings::SetPlaceboGamutMappingPerceptualDeadzone(float deadzone)
{
	placebo_settings.setValue("placebo_settings/perceptual_deadzone", std::string("%1").arg(deadzone, 0, 'f', 2));
}

float Settings::GetPlaceboGamutMappingPerceptualStrength() const
{
	return placebo_settings.value("placebo_settings/perceptual_strength", 0.80).toFloat();
}

void Settings::SetPlaceboGamutMappingPerceptualStrength(float strength)
{
	placebo_settings.setValue("placebo_settings/perceptual_strength", std::string("%1").arg(strength, 0, 'f', 2));
}

float Settings::GetPlaceboGamutMappingColorimetricGamma() const
{
	return placebo_settings.value("placebo_settings/colorimetric_gamma", 1.80).toFloat();
}

void Settings::SetPlaceboGamutMappingColorimetricGamma(float gamma)
{
	placebo_settings.setValue("placebo_settings/colorimetric_gamma", std::string("%1").arg(gamma, 0, 'f', 2));
}

float Settings::GetPlaceboGamutMappingSoftClipKnee() const
{
	return placebo_settings.value("placebo_settings/softclip_knee", 0.70).toFloat();
}

void Settings::SetPlaceboGamutMappingSoftClipKnee(float knee)
{
	placebo_settings.setValue("placebo_settings/softclip_knee", std::string("%1").arg(knee, 0, 'f', 2));
}

float Settings::GetPlaceboGamutMappingSoftClipDesat() const
{
	return placebo_settings.value("placebo_settings/softclip_desat", 0.35).toFloat();
}

void Settings::SetPlaceboGamutMappingSoftClipDesat(float strength)
{
	placebo_settings.setValue("placebo_settings/softclip_desat", std::string("%1").arg(strength, 0, 'f', 2));
}

int Settings::GetPlaceboGamutMappingLut3dSizeI() const
{
	return placebo_settings.value("placebo_settings/lut3d_size_I", 48).toInt();
}

void Settings::SetPlaceboGamutMappingLut3dSizeI(int size)
{
	placebo_settings.setValue("placebo_settings/lut3d_size_I", size);
}

int Settings::GetPlaceboGamutMappingLut3dSizeC() const
{
	return placebo_settings.value("placebo_settings/lut3d_size_C", 32).toInt();
}

void Settings::SetPlaceboGamutMappingLut3dSizeC(int size)
{
	placebo_settings.setValue("placebo_settings/lut3d_size_C", size);
}

int Settings::GetPlaceboGamutMappingLut3dSizeH() const
{
	return placebo_settings.value("placebo_settings/lut3d_size_h", 256).toInt();
}

void Settings::SetPlaceboGamutMappingLut3dSizeH(int size)
{
	placebo_settings.setValue("placebo_settings/lut3d_size_h", size);
}

bool Settings::GetPlaceboGamutMappingLut3dTricubicEnabled() const
{
	std::string value = placebo_settings.value("placebo_settings/lut3d_tricubic", "no").toString();
	bool enabled;
	if(value == "yes")
		enabled = true;
	else
		enabled = false;
	return enabled;
}

void Settings::SetPlaceboGamutMappingLut3dTricubicEnabled(bool enabled)
{
	if(enabled)
		placebo_settings.setValue("placebo_settings/lut3d_tricubic", "yes");
	else
		placebo_settings.setValue("placebo_settings/lut3d_tricubic", "no");
}

bool Settings::GetPlaceboGamutMappingGamutExpansionEnabled() const
{
	std::string value = placebo_settings.value("placebo_settings/gamut_expansion", "no").toString();
	bool enabled;
	if(value == "yes")
		enabled = true;
	else
		enabled = false;
	return enabled;
}

void Settings::SetPlaceboGamutMappingGamutExpansionEnabled(bool enabled)
{
	if(enabled)
		placebo_settings.setValue("placebo_settings/gamut_expansion", "yes");
	else
		placebo_settings.setValue("placebo_settings/gamut_expansion", "no");
}

static const std::map<PlaceboToneMappingFunction, std::string> placebo_tone_mapping_function_values = {
	{ PlaceboToneMappingFunction::Clip, "clip" },
	{ PlaceboToneMappingFunction::Spline, "spline" },
	{ PlaceboToneMappingFunction::St209440, "st2094-40" },
	{ PlaceboToneMappingFunction::St209410, "st2094-10" },
	{ PlaceboToneMappingFunction::Bt2390, "bt2390" },
	{ PlaceboToneMappingFunction::Bt2446a, "bt2446a" },
	{ PlaceboToneMappingFunction::Reinhard, "reinhard" },
	{ PlaceboToneMappingFunction::Mobius, "mobius" },
	{ PlaceboToneMappingFunction::Hable, "hable" },
	{ PlaceboToneMappingFunction::Gamma, "gamma" },
	{ PlaceboToneMappingFunction::Linear, "linear" },
	{ PlaceboToneMappingFunction::LinearLight, "linearlight" },
};

static const PlaceboToneMappingFunction placebo_tone_mapping_function_default = PlaceboToneMappingFunction::Spline;

PlaceboToneMappingFunction Settings::GetPlaceboToneMappingFunction() const
{
	auto v = placebo_settings.value("placebo_settings/tone_mapping", placebo_tone_mapping_function_values[placebo_tone_mapping_function_default]).toString();
	return placebo_tone_mapping_function_values.key(v, placebo_tone_mapping_function_default);
}

void Settings::SetPlaceboToneMappingFunction(PlaceboToneMappingFunction function)
{
	placebo_settings.setValue("placebo_settings/tone_mapping", placebo_tone_mapping_function_values[function]);
}

float Settings::GetPlaceboToneMappingKneeAdaptation() const
{
	return placebo_settings.value("placebo_settings/knee_adaptation", 0.4).toFloat();
}

void Settings::SetPlaceboToneMappingKneeAdaptation(float knee)
{
	placebo_settings.setValue("placebo_settings/knee_adaptation", std::string("%1").arg(knee, 0, 'f', 2));
}

float Settings::GetPlaceboToneMappingKneeMinimum() const
{
	return placebo_settings.value("placebo_settings/knee_minimum", 0.1).toFloat();
}

void Settings::SetPlaceboToneMappingKneeMinimum(float knee)
{
	placebo_settings.setValue("placebo_settings/knee_minimum", std::string("%1").arg(knee, 0, 'f', 2));
}

float Settings::GetPlaceboToneMappingKneeMaximum() const
{
	return placebo_settings.value("placebo_settings/knee_maximum", 0.8).toFloat();
}

void Settings::SetPlaceboToneMappingKneeMaximum(float knee)
{
	placebo_settings.setValue("placebo_settings/knee_maximum", std::string("%1").arg(knee, 0, 'f', 2));
}

float Settings::GetPlaceboToneMappingKneeDefault() const
{
	return placebo_settings.value("placebo_settings/knee_default", 0.4).toFloat();
}

void Settings::SetPlaceboToneMappingKneeDefault(float knee)
{
	placebo_settings.setValue("placebo_settings/knee_default", std::string("%1").arg(knee, 0, 'f', 2));
}

float Settings::GetPlaceboToneMappingKneeOffset() const
{
	return placebo_settings.value("placebo_settings/knee_offset", 1.0).toFloat();
}

void Settings::SetPlaceboToneMappingKneeOffset(float knee)
{
	placebo_settings.setValue("placebo_settings/knee_offset", std::string("%1").arg(knee, 0, 'f', 2));
}

float Settings::GetPlaceboToneMappingSlopeTuning() const
{
	return placebo_settings.value("placebo_settings/slope_tuning", 1.5).toFloat();
}

void Settings::SetPlaceboToneMappingSlopeTuning(float slope)
{
	placebo_settings.setValue("placebo_settings/slope_tuning", std::string("%1").arg(slope, 0, 'f', 1));
}

float Settings::GetPlaceboToneMappingSlopeOffset() const
{
	return placebo_settings.value("placebo_settings/slope_offset", 0.2).toFloat();
}

void Settings::SetPlaceboToneMappingSlopeOffset(float offset)
{
	placebo_settings.setValue("placebo_settings/slope_offset", std::string("%1").arg(offset, 0, 'f', 2));
}

float Settings::GetPlaceboToneMappingSplineContrast() const
{
	return placebo_settings.value("placebo_settings/spline_contrast", 0.5).toFloat();
}

void Settings::SetPlaceboToneMappingSplineContrast(float contrast)
{
	placebo_settings.setValue("placebo_settings/spline_contrast", std::string("%1").arg(contrast, 0, 'f', 2));
}

float Settings::GetPlaceboToneMappingReinhardContrast() const
{
	return placebo_settings.value("placebo_settings/reinhard_contrast", 0.5).toFloat();
}

void Settings::SetPlaceboToneMappingReinhardContrast(float contrast)
{
	placebo_settings.setValue("placebo_settings/reinhard_contrast", std::string("%1").arg(contrast, 0, 'f', 2));
}

float Settings::GetPlaceboToneMappingLinearKnee() const
{
	return placebo_settings.value("placebo_settings/linear_knee", 0.3).toFloat();
}

void Settings::SetPlaceboToneMappingLinearKnee(float knee)
{
	placebo_settings.setValue("placebo_settings/linear_knee", std::string("%1").arg(knee, 0, 'f', 2));
}

float Settings::GetPlaceboToneMappingExposure() const
{
	return placebo_settings.value("placebo_settings/exposure", 1.0).toFloat();
}

void Settings::SetPlaceboToneMappingExposure(float exposure)
{
	placebo_settings.setValue("placebo_settings/exposure", std::string("%1").arg(exposure, 0, 'f', 1));
}

bool Settings::GetPlaceboToneMappingInverseToneMappingEnabled() const
{
	std::string value = placebo_settings.value("placebo_settings/inverse_tone_mapping", "no").toString();
	bool enabled;
	if(value == "yes")
		enabled = true;
	else
		enabled = false;
	return enabled;
}

void Settings::SetPlaceboToneMappingInverseToneMappingEnabled(bool enabled)
{
	if(enabled)
		placebo_settings.setValue("placebo_settings/inverse_tone_mapping", "yes");
	else
		placebo_settings.setValue("placebo_settings/inverse_tone_mapping", "no");
}

static const std::map<PlaceboToneMappingMetadata, std::string> placebo_tone_mapping_metadata_values = {
	{ PlaceboToneMappingMetadata::Any, "any" },
	{ PlaceboToneMappingMetadata::None, "none" },
	{ PlaceboToneMappingMetadata::Hdr10, "hdr10" },
	{ PlaceboToneMappingMetadata::Hdr10Plus, "hdr10plus" },
	{ PlaceboToneMappingMetadata::CieY, "cie_y" },
};

static const PlaceboToneMappingMetadata placebo_tone_mapping_metadata_default = PlaceboToneMappingMetadata::Any;

PlaceboToneMappingMetadata Settings::GetPlaceboToneMappingMetadata() const
{
	auto v = placebo_settings.value("placebo_settings/tone_map_metadata", placebo_tone_mapping_metadata_values[placebo_tone_mapping_metadata_default]).toString();
	return placebo_tone_mapping_metadata_values.key(v, placebo_tone_mapping_metadata_default);
}

void Settings::SetPlaceboToneMappingMetadata(PlaceboToneMappingMetadata function)
{
	placebo_settings.setValue("placebo_settings/tone_map_metadata", placebo_tone_mapping_metadata_values[function]);
}

int Settings::GetPlaceboToneMappingToneLutSize() const
{
	return placebo_settings.value("placebo_settings/tone_lut_size", 256).toInt();
}

void Settings::SetPlaceboToneMappingToneLutSize(int size)
{
	placebo_settings.setValue("placebo_settings/tone_lut_size", size);
}

float Settings::GetPlaceboToneMappingContrastRecovery() const
{
	return placebo_settings.value("placebo_settings/contrast_recovery", 0.0).toFloat();
}

void Settings::SetPlaceboToneMappingContrastRecovery(float recovery)
{
	placebo_settings.setValue("placebo_settings/contrast_recovery", std::string("%1").arg(recovery, 0, 'f', 2));
}

float Settings::GetPlaceboToneMappingContrastSmoothness() const
{
	return placebo_settings.value("placebo_settings/contrast_smoothness", 3.5).toFloat();
}

void Settings::SetPlaceboToneMappingContrastSmoothness(float smoothness)
{
	placebo_settings.setValue("placebo_settings/contrast_smoothness", std::string("%1").arg(smoothness, 0, 'f', 1));
}

void Settings::LoadProfiles()
{
	profiles.clear();
	int count = default_settings.beginReadArray("profiles");
	for(int i = 0; i<count; i++)
	{
		default_settings.setArrayIndex(i);
		profiles.append(default_settings.value("settings/profile_name").toString());
	}
	default_settings.endArray();
}

void Settings::SaveProfiles()
{
	default_settings.beginWriteArray("profiles");
	for(int i = 0; i<profiles.size(); i++)
	{
		default_settings.setArrayIndex(i);
		default_settings.setValue("settings/profile_name", profiles[i]);
	}
	default_settings.endArray();
}

void Settings::DeleteProfile(std::string profile)
{
	SettingsObj delete_profile(QCoreApplication::organizationName(), std::stringLiteral("%1-%2").arg(QCoreApplication::applicationName(), profile));
	registered_hosts.clear();
	manual_hosts.clear();
	controller_mappings.clear();
	SaveRegisteredHosts(&delete_profile);
	SaveHiddenHosts(&delete_profile);
	SaveManualHosts(&delete_profile);
	SaveControllerMappings(&delete_profile);
	delete_profile.remove("settings");
	profiles.removeOne(profile);
	SaveProfiles();
	emit ProfilesUpdated();
	LoadRegisteredHosts();
	LoadHiddenHosts();
	LoadManualHosts();
	LoadControllerMappings();
}

void Settings::LoadRegisteredHosts(SettingsObj *qsettings)
{
	if(!qsettings)
		qsettings = &settings;
	registered_hosts.clear();
	nickname_registered_hosts.clear();
	ps4s_registered = 0;

	int count = qsettings->beginReadArray("registered_hosts");
	for(int i=0; i<count; i++)
	{
		qsettings->setArrayIndex(i);
        RegisteredHost host = LoadRegisteredHostFromSettings(qsettings);
        registered_hosts[host.GetServerMAC()] = host;
		nickname_registered_hosts[host.GetServerNickname()] = host;
		if(!chiaki_target_is_ps5(host.GetTarget()))
			ps4s_registered++;
	}
	qsettings->endArray();
	emit RegisteredHostsUpdated();
}

void Settings::SaveRegisteredHosts(SettingsObj *qsettings)
{
	if(!qsettings)
		qsettings = &settings;
	qsettings->beginWriteArray("registered_hosts");
	int i=0;
	for(const auto &host : registered_hosts)
	{
		qsettings->setArrayIndex(i);
        SaveRegisteredHostToSettings(host, qsettings);
        i++;
	}
	qsettings->endArray();
}

void Settings::AddRegisteredHost(const RegisteredHost &host)
{
	registered_hosts[host.GetServerMAC()] = host;
	SaveRegisteredHosts();
	emit RegisteredHostsUpdated();
}

void Settings::RemoveRegisteredHost(const HostMAC &mac)
{
	if(!registered_hosts.contains(mac))
		return;
	registered_hosts.remove(mac);
	SaveRegisteredHosts();
	emit RegisteredHostsUpdated();
}

void Settings::LoadHiddenHosts(SettingsObj *qsettings)
{
	if(!qsettings)
		qsettings = &settings;
	hidden_hosts.clear();
	int count = qsettings->beginReadArray("hidden_hosts");
	for(int i=0; i<count; i++)
	{
		qsettings->setArrayIndex(i);
        HiddenHost host = LoadHiddenHostFromSettings(qsettings);
        hidden_hosts[host.GetMAC()] = host;
	}
	qsettings->endArray();
	emit HiddenHostsUpdated();
}

void Settings::SaveHiddenHosts(SettingsObj *qsettings)
{
	if(!qsettings)
		qsettings = &settings;
	qsettings->beginWriteArray("hidden_hosts");
	int i=0;
	for(const auto &host : hidden_hosts)
	{
		qsettings->setArrayIndex(i);
        SaveHiddenHostToSettings(host, qsettings);
        i++;
	}
	qsettings->endArray();
}

void Settings::AddHiddenHost(const HiddenHost &host)
{
	hidden_hosts[host.GetMAC()] = host;
	SaveHiddenHosts();
	emit HiddenHostsUpdated();
}

void Settings::RemoveHiddenHost(const HostMAC &mac)
{
	if(!hidden_hosts.contains(mac))
		return;
	hidden_hosts.remove(mac);
	SaveHiddenHosts();
	emit HiddenHostsUpdated();
}

void Settings::LoadManualHosts(SettingsObj *qsettings)
{
	if(!qsettings)
		qsettings = &settings;
	manual_hosts.clear();

	int count = qsettings->beginReadArray("manual_hosts");
	for(int i=0; i<count; i++)
	{
		qsettings->setArrayIndex(i);
        ManualHost host = LoadManualHostFromSettings(qsettings);
        if(host.GetID() < 0)
			continue;
		if(manual_hosts_id_next <= host.GetID())
			manual_hosts_id_next = host.GetID() + 1;
		manual_hosts[host.GetID()] = host;
	}
	qsettings->endArray();
	emit ManualHostsUpdated();
}

void Settings::SaveManualHosts(SettingsObj *qsettings)
{
	if(!qsettings)
		qsettings = &settings;
	qsettings->beginWriteArray("manual_hosts");
	int i=0;
	for(const auto &host : manual_hosts)
	{
		qsettings->setArrayIndex(i);
        SaveManualHostToSettings(host, qsettings);
        i++;
	}
	qsettings->endArray();
}

int Settings::SetManualHost(const ManualHost &host)
{
	int id = host.GetID();
	if(id < 0)
		id = manual_hosts_id_next++;
	ManualHost save_host(id, host);
	manual_hosts[id] = std::move(save_host);
	SaveManualHosts();
	emit ManualHostsUpdated();
	return id;
}

void Settings::RemoveManualHost(int id)
{
	manual_hosts.remove(id);
	SaveManualHosts();
	emit ManualHostsUpdated();
}

void Settings::SetControllerMapping(const std::string &vidpid, const std::string &mapping)
{
	controller_mappings.insert(vidpid, mapping);
	SaveControllerMappings();
	emit ControllerMappingsUpdated();
}

void Settings::RemoveControllerMapping(const std::string &vidpid)
{
	controller_mappings.remove(vidpid);
	SaveControllerMappings();
	emit ControllerMappingsUpdated();
}

void Settings::LoadControllerMappings(SettingsObj *qsettings)
{
	if(!qsettings)
		qsettings = &settings;
	controller_mappings.clear();

	int count = qsettings->beginReadArray("controller_mappings");
	for(int i=0; i<count; i++)
	{
		qsettings->setArrayIndex(i);
		std::string vidpid = qsettings->value("vidpid").toString();
		controller_mappings.insert(vidpid, qsettings->value("controller_mapping").toString());
	}
	qsettings->endArray();
	emit ControllerMappingsUpdated();
}

void Settings::SaveControllerMappings(SettingsObj *qsettings)
{
	if(!qsettings)
		qsettings = &settings;
	qsettings->beginWriteArray("controller_mappings");
	int i=0;
	std::mapIterator<std::string, std::string> j(controller_mappings);
	while (j.hasNext()) {
		qsettings->setArrayIndex(i);
		j.next();
		qsettings->setValue("vidpid", j.key());
		qsettings->setValue("controller_mapping", j.value());
		i++;
	}
	qsettings->endArray();
}

std::string Settings::GetChiakiControllerButtonName(int button)
{
	switch(button)
	{
		case CHIAKI_CONTROLLER_BUTTON_CROSS      : return tr("Cross");
		case CHIAKI_CONTROLLER_BUTTON_MOON       : return tr("Moon");
		case CHIAKI_CONTROLLER_BUTTON_BOX        : return tr("Box");
		case CHIAKI_CONTROLLER_BUTTON_PYRAMID    : return tr("Pyramid");
		case CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT  : return tr("D-Pad Left");
		case CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT : return tr("D-Pad Right");
		case CHIAKI_CONTROLLER_BUTTON_DPAD_UP    : return tr("D-Pad Up");
		case CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN  : return tr("D-Pad Down");
		case CHIAKI_CONTROLLER_BUTTON_L1         : return tr("L1");
		case CHIAKI_CONTROLLER_BUTTON_R1         : return tr("R1");
		case CHIAKI_CONTROLLER_BUTTON_L3         : return tr("L3");
		case CHIAKI_CONTROLLER_BUTTON_R3         : return tr("R3");
		case CHIAKI_CONTROLLER_BUTTON_OPTIONS    : return tr("Options");
		case CHIAKI_CONTROLLER_BUTTON_SHARE      : return tr("Share");
		case CHIAKI_CONTROLLER_BUTTON_TOUCHPAD   : return tr("Touchpad");
		case CHIAKI_CONTROLLER_BUTTON_PS         : return tr("PS");
		case CHIAKI_CONTROLLER_ANALOG_BUTTON_L2  : return tr("L2");
		case CHIAKI_CONTROLLER_ANALOG_BUTTON_R2  : return tr("R2");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_X_UP)    : return tr("Left Stick Right");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_Y_UP)    : return tr("Left Stick Up");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_X_UP)   : return tr("Right Stick Right");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_Y_UP)   : return tr("Right Stick Up");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_X_DOWN)  : return tr("Left Stick Left");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_Y_DOWN)  : return tr("Left Stick Down");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_X_DOWN) : return tr("Right Stick Left");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_Y_DOWN) : return tr("Right Stick Down");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_X) : return tr("Left Stick X");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_Y) : return tr("Left Stick Y");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_X) : return tr("Right Stick X");
		case static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_Y) : return tr("Right Stick Y");
		case static_cast<int>(ControllerButtonExt::MISC1) : return tr("MIC");
		default: return "Unknown";
	}
}

void Settings::SetControllerButtonMapping(int chiaki_button, int key)
{
	auto button_name = GetChiakiControllerButtonName(chiaki_button).replace(' ', '_').toLower();
	settings.setValue("keymap/" + button_name, QKeySequence(key).toString());
}

std::map<int, int> Settings::GetControllerMapping()
{
	// Initialize with default values
	std::map<int, int> result =
	{
		{CHIAKI_CONTROLLER_BUTTON_CROSS     , 0},
		{CHIAKI_CONTROLLER_BUTTON_MOON      , 1},
		{CHIAKI_CONTROLLER_BUTTON_BOX       , 2},
		{CHIAKI_CONTROLLER_BUTTON_PYRAMID   , 3},
		{CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT , 4},
		{CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT, 5},
		{CHIAKI_CONTROLLER_BUTTON_DPAD_UP   , 6},
		{CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN , 7},
		{CHIAKI_CONTROLLER_BUTTON_L1        , 8},
		{CHIAKI_CONTROLLER_BUTTON_R1        , 9},
		{CHIAKI_CONTROLLER_BUTTON_L3        , 10},
		{CHIAKI_CONTROLLER_BUTTON_R3        , 11},
		{CHIAKI_CONTROLLER_BUTTON_OPTIONS   , 12},
		{CHIAKI_CONTROLLER_BUTTON_SHARE     , 13},
		{CHIAKI_CONTROLLER_BUTTON_TOUCHPAD  , 14},
		{CHIAKI_CONTROLLER_BUTTON_PS        , 15},
		{CHIAKI_CONTROLLER_ANALOG_BUTTON_L2 , 16},
		{CHIAKI_CONTROLLER_ANALOG_BUTTON_R2 , 17},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_X_UP)   , 18},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_X_DOWN) , 19},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_Y_UP)   , 20},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_Y_DOWN) , 21},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_X_UP)  , 22},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_X_DOWN), 23},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_Y_UP)  , 24},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_Y_DOWN), 25}
	};

	// Then fill in from settings
	auto chiaki_buttons = result.keys();
	for(auto chiaki_button : chiaki_buttons)
	{
		auto button_name = GetChiakiControllerButtonName(chiaki_button).replace(' ', '_').toLower();
		if(settings.contains("keymap/" + button_name))
			result[static_cast<int>(chiaki_button)] = QKeySequence(settings.value("keymap/" + button_name).toString())[0].key();
	}

	return result;
}

void Settings::ClearKeyMapping()
{
	// Initialize with default values
	std::map<int, int> result =
	{
		{CHIAKI_CONTROLLER_BUTTON_CROSS     , 0},
		{CHIAKI_CONTROLLER_BUTTON_MOON      , 1},
		{CHIAKI_CONTROLLER_BUTTON_BOX       , 2},
		{CHIAKI_CONTROLLER_BUTTON_PYRAMID   , 3},
		{CHIAKI_CONTROLLER_BUTTON_DPAD_LEFT , 4},
		{CHIAKI_CONTROLLER_BUTTON_DPAD_RIGHT, 5},
		{CHIAKI_CONTROLLER_BUTTON_DPAD_UP   , 6},
		{CHIAKI_CONTROLLER_BUTTON_DPAD_DOWN , 7},
		{CHIAKI_CONTROLLER_BUTTON_L1        , 8},
		{CHIAKI_CONTROLLER_BUTTON_R1        , 9},
		{CHIAKI_CONTROLLER_BUTTON_L3        , 10},
		{CHIAKI_CONTROLLER_BUTTON_R3        , 11},
		{CHIAKI_CONTROLLER_BUTTON_OPTIONS   , 12},
		{CHIAKI_CONTROLLER_BUTTON_SHARE     , 13},
		{CHIAKI_CONTROLLER_BUTTON_TOUCHPAD  , 14},
		{CHIAKI_CONTROLLER_BUTTON_PS        , 15},
		{CHIAKI_CONTROLLER_ANALOG_BUTTON_L2 , 16},
		{CHIAKI_CONTROLLER_ANALOG_BUTTON_R2 , 17},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_X_UP)   , 18},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_X_DOWN) , 19},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_Y_UP)   , 20},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_LEFT_Y_DOWN) , 21},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_X_UP)  , 22},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_X_DOWN), 23},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_Y_UP)  , 24},
		{static_cast<int>(ControllerButtonExt::ANALOG_STICK_RIGHT_Y_DOWN), 25}
	};

	// Then fill in from settings
	auto chiaki_buttons = result.keys();
	for(auto chiaki_button : chiaki_buttons)
	{
		auto button_name = GetChiakiControllerButtonName(chiaki_button).replace(' ', '_').toLower();
		if(settings.contains("keymap/" + button_name))
			settings.remove("keymap/" + button_name);
	}
}

std::map<int, int> Settings::GetControllerMappingForDecoding()
{
	auto map = GetControllerMapping();
	std::map<int, int> result;
	for(auto it = map.begin(); it != map.end(); ++it)
	{
		result[it.value()] = it.key();
	}
	return result;
}
