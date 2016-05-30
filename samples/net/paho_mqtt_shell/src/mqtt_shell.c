#include "mqtt_shell.h"

#include <stdlib.h>
#include <misc/printk.h>
#include <misc/shell.h>
#include <errno.h>

#include "config.h"
#include "tcp.h"
#include "mqtt.h"

char *client_name = "zephyr_client";
char *topic = "zephyr";
char *msg = "Hello World from Zephyr!";

struct net_context *ctx;

struct shell_cmd commands[] = {
	/* do not move 'repeat_until_ok' and 'repeat' */
	{ "repeat_until_ok", shell_repeat_until_ok },
	{ "repeat", shell_repeat },
	{ "connect", shell_connect },
	{ "disconnect", shell_disconnect },
	{ "subscribe", shell_subs },
	{ "ping", shell_ping },
	{ "publish", shell_pub },
	{ "read", shell_read },
	{ NULL, NULL }
};

int find_cb(int (**ptr)(int, char **), char *action)
{
	int i;

	*ptr = NULL;
	for (i = 2; commands[i].cmd_name; i++) {
		if (strcmp(commands[i].cmd_name, action) == 0) {
			*ptr = commands[i].cb;
		}
	}

	return ptr == NULL ? -EINVAL : 0;
}

int shell_repeat_until_ok(int argc, char *argv[])
{
	int (*ptr)(int, char **) = NULL;

	if (argc < 2) {
		printk("Usage: repeat_until_ok command\n");
	}
	if (find_cb(&ptr, argv[1]) != 0) {
		printk("Unable to execute action\n");
		return -EINVAL;
	}

	while (ptr(argc-1, argv+1) != 0) {
	}

	return 0;
}

int shell_repeat(int argc, char *argv[])
{
	int (*ptr)(int, char **) = NULL;
	char *action;
	int i;
	int n;

	if (argc < 3) {
		printk("Usage: repeat n command\n");
	}

	action = argv[2];
	if (find_cb(&ptr, argv[2]) != 0) {
		printk("Unable to execute action\n");
		return -EINVAL;
	}

	n = atoi(argv[1]);
	for (i = 0; i < n; i++) {
		ptr(argc-2, argv+2);
	}

	return 0;
}

int shell_connect(int argc, char *argv[])
{
	int rc;

	if (argc < 2) {
		printk("Using default client name '%s'.\n"
		      "Usage: connect: connect client_name\n", client_name);
		rc = mqtt_connect(ctx, client_name);
	} else {
		rc = mqtt_connect(ctx, argv[1]);
	}

	if (rc != 0) {
		printk("Connect: failure\n");
		return rc;
	}

	return 0;
}

int shell_disconnect(int argc, char *argv[])
{
	int rc;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	rc = mqtt_disconnect(ctx);
	if (rc != 0) {
		printk("Disconnect: failure\n");
		return rc;
	}

	return 0;
}

int shell_subs(int argc, char *argv[])
{
	int rc;

	if (argc < 2) {
		printk("Using default topic '%s'.\n"
		      "Usage: subscribe topic\n", topic);
		rc = mqtt_subscribe(ctx, topic);
	} else {
		rc = mqtt_subscribe(ctx, argv[1]);
	}

	if (rc != 0) {
		printk("Subscribe: failure\n");
		return rc;
	}

	return 0;
}

int shell_ping(int argc, char *argv[])
{
	int rc;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	rc = mqtt_pingreq(ctx);
	if (rc != 0) {
		printk("Ping: failure\n");
		return rc;
	}

	return 0;
}

int shell_pub(int argc, char *argv[])
{
	int rc;

	if (argc < 3) {
		printk("Publishing to default topic '%s'.\n"
		      "Usage: publish topic msg\n", topic);
		rc = mqtt_publish(ctx, topic, msg);
	} else {
		rc = mqtt_publish(ctx, argv[1], argv[2]);
	}

	if (rc != 0) {
		printk("Publish: failure\n");
		return rc;
	}

	return 0;
}

int shell_read(int argc, char *argv[])
{
	char received_topic[32];
	char received_msg[64];
	int rc;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	rc = mqtt_publish_read(ctx, received_topic, received_msg);
	if (rc == 0) {
		printk("topic: %s, msg: %s\n", received_topic, received_msg);
		return 0;
	}

	return rc;
}
