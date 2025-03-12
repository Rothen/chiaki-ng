// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki-pybind.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define PYBIND11_DETAILED_ERROR_MESSAGES
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>

namespace py = pybind11;

int chiaki_pybind_discover_wrapper(ChiakiLog *log, const std::string &host, const std::string &timeout)
{
    return chiaki_pybind_discover(log, host.c_str(), timeout.c_str());
}

int chiaki_pybind_wakeup_wrapper(ChiakiLog *log, const std::string &host, const std::string &registkey, bool ps5)
{
    return chiaki_pybind_wakeup(log, host.c_str(), registkey.c_str(), ps5);
}

PYBIND11_MODULE(chiaki, m)
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
        .value("CHIAKI_TARGET_PS4_UNKNOWN", CHIAKI_TARGET_PS4_UNKNOWN)
        .value("CHIAKI_TARGET_PS4_8", CHIAKI_TARGET_PS4_8)
        .value("CHIAKI_TARGET_PS4_9", CHIAKI_TARGET_PS4_9)
        .value("CHIAKI_TARGET_PS4_10", CHIAKI_TARGET_PS4_10)
        .value("CHIAKI_TARGET_PS5_UNKNOWN", CHIAKI_TARGET_PS5_UNKNOWN)
        .value("CHIAKI_TARGET_PS5_1", CHIAKI_TARGET_PS5_1)
        .export_values();

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