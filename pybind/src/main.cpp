// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki-pybind.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

// Wrapper class for ChiakiLog
class PyChiakiLog
{
public:
    uint32_t level_mask;
    std::function<void(int, std::string)> log_callback;
    ChiakiLog chiaki_log;

    PyChiakiLog(uint32_t level_mask = 0xFFFFFFFF) : level_mask(level_mask)
    {
        chiaki_log.level_mask = level_mask;
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

int chiaki_pybind_discover_wrapper(PyChiakiLog &log, const std::string &host, const std::string &timeout)
{
    return chiaki_pybind_discover(log.get_raw_log(), host.c_str(), timeout.c_str());
}

int chiaki_pybind_wakeup_wrapper(ChiakiLog *log, const std::string &host, const std::string &registkey, bool ps5)
{
    return chiaki_pybind_wakeup(log, host.c_str(), registkey.c_str(), ps5);
}

PYBIND11_MODULE(chiaki_py, m)
{
    m.doc() = "Python bindings for Chiaki CLI commands";

    py::enum_<ChiakiErrorCode>(m, "ChiakiErrorCode")
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

    py::enum_<ChiakiTarget>(m, "ChiakiTarget")
        .value("PS4_UNKNOWN", CHIAKI_TARGET_PS4_UNKNOWN)
        .value("PS4_8", CHIAKI_TARGET_PS4_8)
        .value("PS4_9", CHIAKI_TARGET_PS4_9)
        .value("PS4_10", CHIAKI_TARGET_PS4_10)
        .value("PS5_UNKNOWN", CHIAKI_TARGET_PS5_UNKNOWN)
        .value("PS5_1", CHIAKI_TARGET_PS5_1)
        .export_values();

    py::enum_<ChiakiLogLevel>(m, "ChiakiLogLevel")
        .value("DEBUG", CHIAKI_LOG_DEBUG)
        .value("VERBOSE", CHIAKI_LOG_VERBOSE)
        .value("INFO", CHIAKI_LOG_INFO)
        .value("WARNING", CHIAKI_LOG_WARNING)
        .value("ERROR", CHIAKI_LOG_ERROR)
        .export_values();

    // Expose ChiakiLog class
    py::class_<PyChiakiLog>(m, "ChiakiLog")
        .def(py::init<uint32_t>(), py::arg("level_mask") = 0xFFFFFFFF)
        .def("set_callback", &PyChiakiLog::set_callback, "Set the logging callback function.");

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
}