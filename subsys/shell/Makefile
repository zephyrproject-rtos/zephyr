subdir-ccflags-y += -I$(srctree)/include/drivers

zephyr_sources_ifdef(
	CONFIG_CONSOLE_SHELL
	shell_service.c shell.c)

obj-$(CONFIG_CONSOLE_SHELL) += modules/
