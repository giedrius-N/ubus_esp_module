#include "ubus_utils.h"
#include <libserialport.h>

#include <libubox/blobmsg_json.h>
#include <libubus.h>
#include <syslog.h>
#include <cJSON.h>

struct blobmsg_policy esp_policy[] = {
	[ESP_PORT] = { .name = "port", .type = BLOBMSG_TYPE_STRING },
	[ESP_PIN] = { .name = "pin", .type =BLOBMSG_TYPE_INT32}
};

struct ubus_method esp_methods[] = {
	UBUS_METHOD("on", esp_on, esp_policy),
	UBUS_METHOD("off", esp_off, esp_policy),
	UBUS_METHOD_NOARG("devices", esp_devices)
};

struct ubus_object_type esp_object_type = UBUS_OBJECT_TYPE("esp", esp_methods);

struct ubus_object esp_object = {
	.name = "esp",
	.type = &esp_object_type,
	.methods = esp_methods,
	.n_methods = ARRAY_SIZE(esp_methods),
};

void print_status(char *message, struct ubus_context *ctx, struct ubus_request_data *req) {
	struct blob_buf resp_buf = {};
    blob_buf_init(&resp_buf, 0);
    blobmsg_add_string(&resp_buf, "status", message);
    ubus_send_reply(ctx, req, resp_buf.head);

    blob_buf_free(&resp_buf);
}

int esp_off(struct ubus_context *ctx, struct ubus_object *obj,
		      	struct ubus_request_data *req, const char *method,
		      	struct blob_attr *msg)
{
	struct blob_attr *tb[__ESP_MAX];
	struct blob_buf b = {};

	blobmsg_parse(esp_policy, __ESP_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[ESP_PORT] && !tb[ESP_PIN]) {
		blob_buf_free(&b);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
		
	char port[15];
	strncpy(port, blobmsg_get_string(tb[ESP_PORT]), 15);
	int pin = blobmsg_get_u32(tb[ESP_PIN]);

	syslog(LOG_INFO, "%s is the port", port);
	syslog(LOG_INFO, "%d is the pin", pin);

	char data[40];
	sprintf(data, "{\"action\": \"off\", \"pin\": %d}", pin);

	if (send_data(port, data)) {
		syslog(LOG_ERR, "Unable to send data to %s", port);
		blob_buf_free(&b);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	blob_buf_free(&b);

	char *message[30];
	sprintf(message, "Port %s pin %d turned off", port, pin);

	print_status(message, ctx, req);

	return 0;
}

int esp_on(struct ubus_context *ctx, struct ubus_object *obj,
		      	struct ubus_request_data *req, const char *method,
		      	struct blob_attr *msg)
{
	struct blob_attr *tb[__ESP_MAX];
	struct blob_buf b = {};

	blobmsg_parse(esp_policy, __ESP_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[ESP_PORT] && !tb[ESP_PIN]) {
		blob_buf_free(&b);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}

	char port[15];
	strncpy(port, blobmsg_get_string(tb[ESP_PORT]), 15);
	int pin = blobmsg_get_u32(tb[ESP_PIN]);

	syslog(LOG_INFO, "%s is the port", port);
	syslog(LOG_INFO, "%d is the pin", pin);

	char data[40];
	sprintf(data, "{\"action\": \"on\", \"pin\": %d}", pin);
	
	if (send_data(port, data)) {
		syslog(LOG_ERR, "Unable to send data to %s", port);
		blob_buf_free(&b);
		return UBUS_STATUS_INVALID_ARGUMENT;
	}
	blob_buf_free(&b);

	char *message[30];
	sprintf(message, "Port %s pin %d turned on", port, pin);

	print_status(message, ctx, req);

	return 0;
}

int esp_devices(struct ubus_context *ctx, struct ubus_object *obj,
		      	struct ubus_request_data *req, const char *method,
		      	struct blob_attr *msg)
{
	struct sp_port **port_list;

	enum sp_return result = sp_list_ports(&port_list);

	if (result != SP_OK) {
		syslog(LOG_ERR, "sp_list_ports() failed!");
		return __UBUS_STATUS_LAST;
	}

	struct blob_buf b = {};
	
	blob_buf_init(&b, 0);

	struct blob_attr *array = blobmsg_open_array(&b, "devices");

	int count = 0;
	for (int i = 0; port_list[i] != NULL; i++) {
		struct sp_port *port = port_list[i];

		int vid, pid;
		char *port_name = sp_get_port_name(port);
		if (sp_get_port_usb_vid_pid(port, &vid, &pid) != SP_OK){
			syslog(LOG_ERR, "Failed. Cannot get %s vendor and product ids", port_name);
			continue;
		}

		char usb_vid[5];
		char usb_pid[5];
		sprintf(usb_pid, "%04X", pid);
		sprintf(usb_vid, "%04X", vid);
		
		struct blob_attr *entry = blobmsg_open_table(&b, NULL);
		blobmsg_add_string(&b, "port", port_name);
		blobmsg_add_string(&b, "vendor_id", usb_vid);
    	blobmsg_add_string(&b, "product_id", usb_pid);
		blobmsg_close_table(&b, entry);
		count++;
	}
	
	blobmsg_close_array(&b, array);
	if (count != 0)
		ubus_send_reply(ctx, req, b.head);
	else {
		print_status("no devices found", ctx, req);
	}

	blob_buf_free(&b);

	sp_free_port_list(port_list);

	return 0;
}

int check(enum sp_return result)
{
	char *error_message;

	switch (result) {
	case SP_ERR_ARG:
		syslog(LOG_ERR, "Error: Invalid argument.");
		return;
	case SP_ERR_FAIL:
		error_message = sp_last_error_message();
		syslog(LOG_ERR, "Error: Failed: %s", error_message);
		sp_free_error_message(error_message);
		return;
	case SP_ERR_SUPP:
		syslog(LOG_ERR, "Error: Not supported.");
		return;
	case SP_ERR_MEM:
		syslog(LOG_ERR, "Error: Couldn't allocate memory.");
		return;
	case SP_OK:
	default:
		return result;
	}
}

int send_data(const char *port, const char *data)
{
	int size = strlen(data);

	unsigned int timeout = 1200;

	int result;
	struct sp_port *serial_port = NULL;
	check(sp_get_port_by_name(port, &serial_port));
	if (serial_port == NULL){
		return -1;
	}
	check(sp_open(serial_port, SP_MODE_READ_WRITE));
	check(sp_set_baudrate(serial_port, 9600));
	check(sp_set_bits(serial_port, 8));
	check(sp_set_parity(serial_port, SP_PARITY_NONE));
	check(sp_set_stopbits(serial_port, 1));
	check(sp_set_flowcontrol(serial_port, SP_FLOWCONTROL_NONE));

	result = check(sp_blocking_write(serial_port, data, size, timeout));

	char response_buffer[100];

    int bytes_read = check(sp_blocking_read(serial_port, response_buffer, sizeof(response_buffer), timeout));

    check(sp_close(serial_port));
	sp_free_port(serial_port);

    if (result != size) {
        syslog(LOG_ERR, "Timed out, %d/%d bytes sent.", result, size);
        return -1;
    }

    if (bytes_read <= 0) {
        syslog(LOG_ERR, "No response received from ESP.");
        return -1;
    }

    return response(response_buffer);
}

int response(char responseJSON[]) 
{	
	cJSON *root = cJSON_Parse(responseJSON);

	if (root == NULL) {
        syslog(LOG_ERR, "Failed to parse JSON.");
        return 1;
    }

    cJSON *responseObj = cJSON_GetObjectItem(root, "response");
    if (responseObj == NULL || !cJSON_IsNumber(responseObj)) {
        syslog(LOG_ERR, "No 'response' field found in JSON or invalid type.");
        cJSON_Delete(root);
        return 1;
    }

    int responseValue = responseObj->valueint;

    cJSON_Delete(root);

	return responseValue;
}