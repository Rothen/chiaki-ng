// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki-cli.h>

#include <chiaki/discovery.h>

#include <argp.h>
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib") // Link Winsock automatically
#else
    #include <netdb.h>
    #include <netinet/in.h>
#endif
#include <stdio.h>
#include <string.h>

static char doc[] = "Send a PS4 or PS5 discovery request.";

#define ARG_KEY_HOST 'h'
#define ARG_KEY_TIMEOUT 't'

static struct argp_option options[] = {
	{ "host", ARG_KEY_HOST, "Host", 0, "Host to send discovery request to", 0 },
	{ "timeout", ARG_KEY_TIMEOUT, "Timeout", 0, "Number of seconds to wait for request to return (default=2)", 0},
	{ 0 }
};

typedef struct arguments
{
	const char *host;
	const char *timeout;
} Arguments;

static int parse_opt(int key, char *arg, struct argp_state *state)
{
	Arguments *arguments = state->input;

	switch(key)
	{
		case ARG_KEY_HOST:
			arguments->host = arg;
			break;
		case ARG_KEY_TIMEOUT:
			arguments->timeout = arg;
			break;
		case ARGP_KEY_ARG:
			argp_usage(state);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = { options, parse_opt, 0, doc, 0, 0, 0 };

static void discovery_cb(ChiakiDiscoveryHost *host, void *user)
{
	ChiakiLog *log = user;

	CHIAKI_LOGI(log, "--");
	CHIAKI_LOGI(log, "Discovered Host:");
	CHIAKI_LOGI(log, "State:                             %s", chiaki_discovery_host_state_string(host->state));

	if(host->system_version)
		CHIAKI_LOGI(log, "System Version:                    %s", host->system_version);

	if(host->device_discovery_protocol_version)
		CHIAKI_LOGI(log, "Device Discovery Protocol Version: %s", host->device_discovery_protocol_version);

	if(host->host_request_port)
		CHIAKI_LOGI(log, "Request Port:                      %hu", (unsigned short)host->host_request_port);

	if(host->host_name)
		CHIAKI_LOGI(log, "Host Name:                         %s", host->host_name);

	if(host->host_type)
		CHIAKI_LOGI(log, "Host Type:                         %s", host->host_type);

	if(host->host_id)
		CHIAKI_LOGI(log, "Host ID:                           %s", host->host_id);

	if(host->running_app_titleid)
		CHIAKI_LOGI(log, "Running App Title ID:              %s", host->running_app_titleid);

	if(host->running_app_name)
		CHIAKI_LOGI(log, "Running App Name:                  %s%s", host->running_app_name, (strcmp(host->running_app_name, "Persona 5") == 0 ? " (best game ever)" : ""));

	CHIAKI_LOGI(log, "--");
}

CHIAKI_EXPORT int chiaki_cli_cmd_discover(ChiakiLog *log, int argc, char *argv[])
{
	Arguments arguments = { 0 };
	float timeout_sec = 2;
	error_t argp_r = argp_parse(&argp, argc, argv, ARGP_IN_ORDER, NULL, &arguments);
	if(argp_r != 0)
		return 1;

	if(!arguments.host)
	{
		fprintf(stderr, "No host specified, see --help.\n");
		return 1;
	}

	if(arguments.timeout)
	{
		timeout_sec = atof(arguments.timeout);
	}

	struct addrinfo *host_addrinfos;
	// make hostname use ipv4 for now
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	char *ipv6 = strchr(arguments.host, ':');
	if(ipv6)
		hints.ai_family = AF_INET6;
	else
		hints.ai_family = AF_INET;
	int r = getaddrinfo(arguments.host, NULL, &hints, &host_addrinfos);
	if(r != 0)
	{
		CHIAKI_LOGE(log, "getaddrinfo failed");
		return 1;
	}

	struct sockaddr *host_addr = NULL;
	socklen_t host_addr_len = 0;
	for(struct addrinfo *ai=host_addrinfos; ai; ai=ai->ai_next)
	{
		if(ai->ai_protocol != IPPROTO_UDP)
			continue;
		if(ai->ai_family != AF_INET && ai->ai_family != AF_INET6)
			continue;

		host_addr_len = ai->ai_addrlen;
		host_addr = (struct sockaddr *)malloc(host_addr_len);
		if(!host_addr)
			break;
		memcpy(host_addr, ai->ai_addr, host_addr_len);
		break;
	}
	freeaddrinfo(host_addrinfos);

	if(!host_addr)
	{
		CHIAKI_LOGE(log, "Failed to get addr for hostname");
		return 1;
	}

	ChiakiDiscoveryPacket packet;
	memset(&packet, 0, sizeof(packet));
	packet.cmd = CHIAKI_DISCOVERY_CMD_SRCH;
	packet.protocol_version = CHIAKI_DISCOVERY_PROTOCOL_VERSION_PS4;
	if(host_addr->sa_family == AF_INET)
		((struct sockaddr_in *)host_addr)->sin_port = htons(CHIAKI_DISCOVERY_PORT_PS4);
	else
		((struct sockaddr_in6 *)host_addr)->sin6_port = htons(CHIAKI_DISCOVERY_PORT_PS4);

	ChiakiDiscovery discovery;
	ChiakiErrorCode err = chiaki_discovery_init(&discovery, log, host_addr->sa_family);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(log, "Discovery init failed");
		goto cleanup_host_addr;
	}

	ChiakiDiscoveryThread thread;
	err = chiaki_discovery_thread_start_oneshot(&thread, &discovery, discovery_cb, NULL);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		CHIAKI_LOGE(log, "Discovery thread init failed");
		goto cleanup;
	}
	err = chiaki_discovery_send(&discovery, &packet, host_addr, host_addr_len);
	if(err != CHIAKI_ERR_SUCCESS)
		CHIAKI_LOGE(log, "Failed to send discovery packet for PS4: %s", chiaki_error_string(err));
	packet.protocol_version = CHIAKI_DISCOVERY_PROTOCOL_VERSION_PS5;
	if(host_addr->sa_family == AF_INET)
		((struct sockaddr_in *)host_addr)->sin_port = htons(CHIAKI_DISCOVERY_PORT_PS5);
	else
		((struct sockaddr_in6 *)host_addr)->sin6_port = htons(CHIAKI_DISCOVERY_PORT_PS5);
	err = chiaki_discovery_send(&discovery, &packet, host_addr, host_addr_len);
	if(err != CHIAKI_ERR_SUCCESS)
		CHIAKI_LOGE(log, "Failed to send discovery packet for PS5: %s", chiaki_error_string(err));
	uint64_t timeout_ms=(timeout_sec * 1000);
	err = chiaki_thread_timedjoin(&thread.thread, NULL, timeout_ms);
	if(err != CHIAKI_ERR_SUCCESS)
	{
		if(err == CHIAKI_ERR_TIMEOUT)
		{
			CHIAKI_LOGE(log, "Discovery request timed out after timeout: %.*f seconds", 1, timeout_sec);
			chiaki_discovery_thread_stop(&thread);
		}
		goto cleanup;
	}
	chiaki_discovery_fini(&discovery);
	free(host_addr);
	return 0;

cleanup:
	chiaki_discovery_fini(&discovery);
cleanup_host_addr:
	free(host_addr);
	return 1;
}
