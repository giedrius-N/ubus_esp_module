#include <libserialport.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>

int check(enum sp_return result);

static int esp_on(struct ubus_context *ctx, struct ubus_object *obj,
		      	struct ubus_request_data *req, const char *method,
		      	struct blob_attr *msg);

static int esp_off(struct ubus_context *ctx, struct ubus_object *obj,
		      	struct ubus_request_data *req, const char *method,
		      	struct blob_attr *msg);

//void send_data(const char port, const char data, struct sp_port* tx_port);

enum {
	ESP_PORT,
	ESP_PIN,
	__ESP_MAX
};

static const struct blobmsg_policy esp_policy[] = {
	[ESP_PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[ESP_PIN] = { .name = "pin", .type =BLOBMSG_TYPE_INT32}
};

static const struct ubus_method esp_methods[] = {
	UBUS_METHOD("on", esp_on, esp_policy),
	UBUS_METHOD("off", esp_off, esp_policy)
};
 
static struct ubus_object_type esp_object_type =
	UBUS_OBJECT_TYPE("esp", esp_methods);


static struct ubus_object esp_object = {
	.name = "esp",
	.type = &esp_object_type,
	.methods = esp_methods,
	.n_methods = ARRAY_SIZE(esp_methods),
};


static int esp_on(struct ubus_context *ctx, struct ubus_object *obj,
		      	struct ubus_request_data *req, const char *method,
		      	struct blob_attr *msg)
{
	struct blob_attr *tb[__ESP_MAX];
	struct blob_buf b = {};

	blobmsg_parse(esp_policy, __ESP_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[ESP_PORT])
		return UBUS_STATUS_INVALID_ARGUMENT;
	if (!tb[ESP_PIN])
		return UBUS_STATUS_INVALID_ARGUMENT;

	char port[15];
	strncpy(port, blobmsg_get_string(tb[ESP_PORT]), 15);
	int pin = blobmsg_get_u32(tb[ESP_PIN]);

	syslog(LOG_INFO, "%s is the port", port);
	syslog(LOG_INFO, "%d is the pin", pin);

	///--------------------------------------------

	char data[40];
	sprintf(data, "{\"action\": \"on\", \"pin\": %d}", pin);
	int size = strlen(data);

	unsigned int timeout = 1000;

	int result;
	struct sp_port *tx_port = NULL;
	check(sp_get_port_by_name(port, &tx_port));
	check(sp_open(tx_port, SP_MODE_READ_WRITE));
	check(sp_set_baudrate(tx_port, 9600));
	check(sp_set_bits(tx_port, 8));
	check(sp_set_parity(tx_port, SP_PARITY_NONE));
	check(sp_set_stopbits(tx_port, 1));
	check(sp_set_flowcontrol(tx_port, SP_FLOWCONTROL_NONE));

	result = check(sp_blocking_write(tx_port, data, size, timeout));

	if (result != size)
		syslog(LOG_ERR, "Timed out, %d/%d bytes sent.\n", result, size);

	check(sp_close(tx_port));

	return 0;
}


static int esp_off(struct ubus_context *ctx, struct ubus_object *obj,
		      	struct ubus_request_data *req, const char *method,
		      	struct blob_attr *msg)
{
	struct blob_attr *tb[2];
	struct blob_buf b = {};

	blobmsg_parse(esp_policy, 2, tb, blob_data(msg), blob_len(msg));

	if (!tb[ESP_PORT])
		return UBUS_STATUS_INVALID_ARGUMENT;
	if (!tb[ESP_PIN])
		return UBUS_STATUS_INVALID_ARGUMENT;

	char port[15];
	strncpy(port, blobmsg_get_string(tb[ESP_PORT]), 15);
	int pin = blobmsg_get_u32(tb[ESP_PIN]);

	syslog(LOG_INFO, "%s is the port", port);
	syslog(LOG_INFO, "%d is the pin", pin);

	char data[40];
	sprintf(data, "{\"action\": \"off\", \"pin\": %d}", pin);
	int size = strlen(data);

	unsigned int timeout = 1000;

	int result;
	struct sp_port *tx_port = NULL;
	check(sp_get_port_by_name(port, &tx_port));
	check(sp_open(tx_port, SP_MODE_READ_WRITE));
	check(sp_set_baudrate(tx_port, 9600));
	check(sp_set_bits(tx_port, 8));
	check(sp_set_parity(tx_port, SP_PARITY_NONE));
	check(sp_set_stopbits(tx_port, 1));
	check(sp_set_flowcontrol(tx_port, SP_FLOWCONTROL_NONE));

	result = check(sp_blocking_write(tx_port, data, size, timeout));

	if (result != size)
		syslog(LOG_ERR, "Timed out, %d/%d bytes sent.\n", result, size);

	check(sp_close(tx_port));

	return 0;
}

int main(int argc, char **argv)
{
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

int check(enum sp_return result)
{
	char *error_message;

	switch (result) {
	case SP_ERR_ARG:
		syslog(LOG_ERR, "Error: Invalid argument.");
		abort();
	case SP_ERR_FAIL:
		error_message = sp_last_error_message();
		syslog(LOG_ERR, "Error: Failed: %s", error_message);
		sp_free_error_message(error_message);
		abort();
	case SP_ERR_SUPP:
		syslog(LOG_ERR, "Error: Not supported.");
		abort();
	case SP_ERR_MEM:
		syslog(LOG_ERR, "Error: Couldn't allocate memory.");
		abort();
	case SP_OK:
	default:
		return result;
	}
}