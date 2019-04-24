#include "log.h"
#define LOG_MODULE_NAME demo_shell
#define LOG_LEVEL CONFIG_DEMO_SHELL_LOG_LEVEL

LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <zephyr.h>
#include <misc/printk.h>
#include <shell/shell.h>
#include <stdlib.h>
#include <version.h>

#include <net/http_client.h>

#define HTTP_CONTENT_RANGE	"Content-Range"
#define HTTP_VERSION "V1.0.0"
#define APP_BUFF_MAX_SIZE	4096

static struct http_ctx ctx;
char app_buf[APP_BUFF_MAX_SIZE];
int app_buf_len;

static int cmd_version(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_fprintf(shell, SHELL_NORMAL,
			"http version %s\n", HTTP_VERSION);
	return 0;
}


static int cmd_http_init(const struct shell *shell, size_t argc, char **argv)
{
	shell_fprintf(shell, SHELL_NORMAL,
			"Create New Http client, %s:%s\n", argv[1], argv[2]);
	http_client_init(&ctx, argv[1], argv[2], false);

	return 0;
}

static int cmd_http_post(const struct shell *shell, size_t argc, char **argv)
{
	return 0;
}

void http_resp_callback(struct http_ctx *pctx,
					const char *body,
					int body_len,
					enum http_final_call final_data)
{
	/** body can't receive commplete*/
	if (final_data == HTTP_DATA_MORE) {
		/** copy the body to app buff*/
		if ((app_buf_len + body_len) < APP_BUFF_MAX_SIZE) {
			memcpy(&app_buf[app_buf_len], body, body_len);
			app_buf_len += body_len;
			printk("Got Http Body (%d) bytes, except (%d) bytes.\n",
				app_buf_len, pctx->rsp.content_length);
		} else {
			printk("app_buff was not enough, (app_buf_len+body_len)=%d\n",
				(app_buf_len + body_len));
		}
	} else {
		/** the body of http response has received complete*/
		if (pctx->rsp.message_complete) {
			printk("*****Response Body Received Complete******\n");
			/**printk("%s\r\n", app_buf);*/
		}
	}
}

void http_fv_callback(struct http_ctx *pctx,
					const char *at,
					int at_len,
					enum http_header_field_val f_v)
{
	int len;
	static bool http_has_content_range;
	char http_content_range_val[32];
	int file_len;
	char *p;

	if (f_v == HTTP_HEADER_FIELD) {
		len = strlen(HTTP_CONTENT_RANGE);
		if (at_len >= len && memcmp(at, HTTP_CONTENT_RANGE, len) == 0) {
			http_has_content_range = true;
		}
	} else {
		if (http_has_content_range) {
			memcpy(http_content_range_val, at, at_len);
			http_content_range_val[at_len] = 0;

			p = strstr(http_content_range_val, "/");
			file_len = strtol(p + 1, NULL, 10);
			http_has_content_range = false;
			printk("http request resource size: (%zd)\n", file_len);
		}
	}
}

static int cmd_http_get(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	shell_fprintf(shell, SHELL_NORMAL,
			"Http Get: %s\n", argv[1]);

	if (argc > 3) {
		http_add_header_field(&ctx, argv[3]);
		shell_fprintf(shell, SHELL_NORMAL,
			"Http header fields: %s\n", argv[3]);
	}

	memset(app_buf, 0, APP_BUFF_MAX_SIZE);
	app_buf_len = 0;

	http_set_resp_callback(&ctx, http_resp_callback);

	http_add_header_field(&ctx, "Range: bytes=0-99\r\n");
	http_set_fv_callback(&ctx, http_fv_callback);

	ret = http_client_get(&ctx, argv[1], true, NULL);
	LOG_INF("http_client_get ret=%d", ret);

	return 0;
}

static int cmd_http_close(const struct shell *shell, size_t argc, char **argv)
{
	return http_client_close(&ctx);
}

SHELL_CREATE_STATIC_SUBCMD_SET(sub_app)
{
	SHELL_CMD(version, NULL, "Show http version.", cmd_version),
	SHELL_CMD(new, NULL, "http new client.\n"
			"Usage: new <host> <port>", cmd_http_init),
	SHELL_CMD(post, NULL, "http post requestion.\n"
			"Usage: post <host> <url_path>", cmd_http_post),
	SHELL_CMD(get, NULL, "http get requestion.\n"
			"Usage: get <url_path>", cmd_http_get),
	SHELL_CMD(close, NULL, "http close client.\n"
			"Usage: close ", cmd_http_close),
	SHELL_SUBCMD_SET_END /* Array terminated. */
};

/* Creating root (level 0) command "app" */
SHELL_CMD_REGISTER(http, &sub_app, "Function test commands", NULL);
