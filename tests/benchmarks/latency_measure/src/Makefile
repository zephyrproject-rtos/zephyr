ccflags-y += -I$(ZEPHYR_BASE)/tests/include
ccflags-$(CONFIG_SOC_QUARK_D2000) += -DSTACKSIZE=256

obj-y = main.o \
	thread_switch_yield.o \
	int_to_thread.o \
	int_to_thread_evt.o \
	sema_lock_release.o \
	coop_ctx_switch.o \
	utils.o
