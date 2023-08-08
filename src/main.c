#include <libserialport.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include "ubus_utils.h"

static void uloop_handle_sigint(int signo)
{
    uloop_cancelled = true;
}

int main(int argc, char **argv)
{
	signal(SIGTERM, uloop_handle_sigint);
	signal(SIGINT, uloop_handle_sigint);
	signal(SIGQUIT, uloop_handle_sigint);

	struct ubus_context *ctx;

	uloop_init();

	ctx = ubus_connect(NULL);
	if (!ctx) {
		syslog(LOG_ERR, "Failed to connect to ubus");
		return -1;
	}

	ubus_add_uloop(ctx);
	ubus_add_object(ctx, &esp_object);
	uloop_run();

	ubus_free(ctx);
	uloop_done();

	return 0;
}