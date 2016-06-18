ccflags-y += -I$(CURDIR)/misc/generated/sysgen
ccflags-y += -I$(ZEPHYR_BASE)/tests/include
ccflags-$(CONFIG_SOC_QUARK_D2000) += -DSTACKSIZE=256

obj-y = main.o \
	micro_int_to_task_evt.o \
	nano_ctx_switch.o \
	nano_int_to_fiber.o \
	micro_sema_lock_release.o \
	nano_int.o \
	nano_int_to_fiber_sem.o \
	micro_int_to_task.o \
	micro_task_switch_yield.o \
	nano_int_lock_unlock.o \
	utils.o
