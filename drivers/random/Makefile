ccflags-y +=-I$(srctree)/arch/$(ARCH)
ccflags-y +=-I$(srctree)/arch/$(ARCH)/platforms/$(strip $(CONFIG_PLATFORM))

obj-$(CONFIG_TIMER_RANDOM_GENERATOR) = rand32-timer.o
obj-$(CONFIG_X86_TSC_RANDOM_GENERATOR) += rand32-timestamp.o
