// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_CHIAKI_CLI_H
#define CHIAKI_CHIAKI_CLI_H

#include <chiaki/common.h>
#include <chiaki/log.h>

#ifdef __cplusplus
extern "C" {
#endif

    CHIAKI_EXPORT int chiaki_pybind_discover(ChiakiLog *log, const char *host, const char *timeout);
    CHIAKI_EXPORT int chiaki_pybind_wakeup(ChiakiLog *log, const char *host, const char *registkey, bool ps5);

#ifdef __cplusplus
}
#endif

#endif //CHIAKI_CHIAKI_CLI_H
