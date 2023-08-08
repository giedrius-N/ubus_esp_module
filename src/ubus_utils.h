#ifndef UBUS_UTILS_H
#define UBUS_UTILS_H

#include <libserialport.h>
#include <libubox/blobmsg_json.h>
#include <libubus.h>

int check(enum sp_return result);

enum {
	ESP_PORT,
	ESP_PIN,
	__ESP_MAX
};

int esp_on(struct ubus_context *ctx, struct ubus_object *obj,
		      	struct ubus_request_data *req, const char *method,
		      	struct blob_attr *msg);
int esp_off(struct ubus_context *ctx, struct ubus_object *obj,
		      	struct ubus_request_data *req, const char *method,
		      	struct blob_attr *msg);

int esp_devices(struct ubus_context *ctx, struct ubus_object *obj,
		      	struct ubus_request_data *req, const char *method,
		      	struct blob_attr *msg);

struct blobmsg_policy esp_policy[];
struct ubus_method esp_methods[];
struct ubus_object_type esp_object_type;
struct ubus_object esp_object;

int send_data(const char *port, const char *data);

#endif
