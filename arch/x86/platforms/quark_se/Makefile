ccflags-y +=-I$(srctree)/arch/x86
ccflags-y +=-I$(srctree)/arch/x86/platforms/$(PLATFORM_NAME)/
ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/drivers
asflags-y := ${ccflags-y}

obj-y = system.o quark_se_config.o eoi.o
obj-$(CONFIG_PINMUX) += pinmux.o
obj-$(CONFIG_PINMUX) += curie101_pinmux.o
