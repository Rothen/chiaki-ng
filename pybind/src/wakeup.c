// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki-pybind.h>

#include <chiaki/discovery.h>

#include <string.h>

/*static struct argp_option options[] = {
	{ "host", ARG_KEY_HOST, "Host", 0, "Host to send wakeup packet to", 0 },
	{ "registkey", ARG_KEY_REGISTKEY, "RegistKey", 0, "Remote Play registration key (plaintext)", 0 },
	{ "ps4", ARG_KEY_PS4, NULL, 0, "PlayStation 4", 0 },
	{ "ps5", ARG_KEY_PS5, NULL, 0, "PlayStation 5 (default)", 0 },
	{ 0 }
};*/

CHIAKI_EXPORT int chiaki_pybind_wakeup(ChiakiLog *log, const char *host, const char *registkey, bool ps5)
{
	if(!host)
	{
		fprintf(stderr, "No host specified, see --help.\n");
		return 1;
	}
	if(!registkey)
	{
		fprintf(stderr, "No registration key specified, see --help.\n");
		return 1;
	}
	if(strlen(registkey) > 8)
	{
		fprintf(stderr, "Given registkey is too long.\n");
		return 1;
	}

	uint64_t credential = (uint64_t)strtoull(registkey, NULL, 16);

	return chiaki_discovery_wakeup(log, NULL, host, credential, ps5);
}
