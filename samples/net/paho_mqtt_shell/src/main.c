#include <zephyr.h>
#include <misc/shell.h>

#include "tcp.h"
#include "mqtt_shell.h"

#define RC_MSG(rc)		(rc) == 0 ? "success" : "failure"
#define STACK_SIZE		1024

uint8_t stack[STACK_SIZE];

extern struct shell_cmd commands[];
extern struct net_context *ctx;

void fiber(void)
{
	shell_init("mqtt_shell> ", commands);
}

void main(void)
{
	net_init();
	tcp_init(&ctx);

	task_fiber_start(stack, STACK_SIZE, (nano_fiber_entry_t)fiber,
			 0, 0, 7, 0);
}
