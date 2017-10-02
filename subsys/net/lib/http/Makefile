ccflags-$(CONFIG_HTTP_PARSER_STRICT) += -DHTTP_PARSER_STRICT

obj-$(CONFIG_HTTP_PARSER) := http_parser.o
obj-$(CONFIG_HTTP_PARSER_URL) += http_parser_url.o

ifeq ($(CONFIG_HTTP_APP),y)
	obj-$(CONFIG_HTTP_SERVER) += http_app_server.o
	obj-$(CONFIG_HTTP_CLIENT) += http_app_client.o
	obj-y += http_app.o
else
	obj-$(CONFIG_HTTP_CLIENT) += http_client.o
	obj-$(CONFIG_HTTP_SERVER) += http_server.o
endif
