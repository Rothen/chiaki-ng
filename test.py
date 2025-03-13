import chiaki_py
import time




# Create a ChiakiLog instance
log = chiaki_py.ChiakiLog()

# Set a Python function as the log callback


def my_log_callback(level, message):
    print(f"[{level}] {message}")


log.set_callback(my_log_callback)

# Call the discover function
chiaki_py.discover(log, "192.168.42.43", "5000")
