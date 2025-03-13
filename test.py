from chiaki_py import ChiakiLog, discover, ChiakiLogLevel

log = ChiakiLog(level=ChiakiLogLevel.INFO)

def my_log_callback(level: int, message: str):
    print(f"[{level}] {message}")

log.set_callback(my_log_callback)

print(discover(log, "192.168.42.43", 2000))
