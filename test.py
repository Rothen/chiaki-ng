from chiaki_py import discover, ChiakiLog, LogLevel, Target, Settings, StreamSessionConnectInfo, StreamSession

log = ChiakiLog(level=LogLevel.INFO)
host = "192.168.42.43"
regist_key = "b02d1ceb"  # 2955746539
nickname = "PS5-083"
ps5Id = "78c881a8214a"
morning = "ª?RÿGC\\x1d/,ðñA\\x10öy³"
morning_x = 0x000001b142182250
initial_login_pin = ""  # None
duid = "" # None
auto_regist = False
fullscreen = False
zoom = False
stretch = False
ps5 = True
discover_timout = 2000

regist_key_list: list[int] = [a for a in map(ord, regist_key)]
morning_list: list[int] = [a for a in map(ord, morning)]

print(regist_key_list)
print(morning_list)

settings: Settings = Settings()
connect_info: StreamSessionConnectInfo = StreamSessionConnectInfo(
    settings=settings,
    target=Target.PS5_1,
    host=host,
    nickname=nickname,
    regist_key=regist_key,
    morning=morning,
    initial_login_pin=initial_login_pin,
    duid=duid,
    auto_regist=auto_regist,
    fullscreen=fullscreen,
    zoom=zoom,
    stretch=stretch
)

print(connect_info)

stream_session: StreamSession = StreamSession(connect_info)
print(stream_session)
stream_session.ffmpeg_frame_available = lambda : print('ffmpeg_frame_available')
stream_session.session_quit = lambda a, b: print('session_quit')
stream_session.login_pin_requested = lambda a: print('login_pin_requested')
stream_session.data_holepunch_progress = lambda a: print('data_holepunch_progress')
stream_session.auto_regist_succeeded = lambda a: print('auto_regist_succeeded')
stream_session.nickname_received = lambda a: print('nickname_received')
stream_session.connected_changed = lambda : print('connected_changed')
stream_session.measured_bitrate_changed = lambda : print('measured_bitrate_changed')
stream_session.average_packet_loss_changed = lambda : print('average_packet_loss_changed')
stream_session.cant_display_changed = lambda a: print('cant_display_changed')
print(stream_session.start())


# log.set_callback(lambda level, message: print(f"[{level}] {message}"))
# print(wakeup(log, host, registKey, ps5))
# print(discover(log, host, discover_timout))
