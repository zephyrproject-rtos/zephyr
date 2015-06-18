ccflags-y +=-I$(srctree)/arch/$(ARCH)
ccflags-y +=-I$(srctree)/arch/$(ARCH)/$(strip $(CONFIG_BSP_DIR))

obj-$(CONFIG_TIMER_RANDOM_GENERATOR) = rand32-timer.o
obj-$(CONFIG_X86_TSC_RANDOM_GENERATOR) += rand32-timestamp.o
