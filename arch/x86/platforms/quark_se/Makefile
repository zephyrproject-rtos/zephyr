ccflags-y +=-I$(srctree)/arch/x86
ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/drivers
asflags-y := ${ccflags-y}

obj-$(CONFIG_PLATFORM_QUARK_SE) = system.o quark_se_config.o eoi.o
obj-$(CONFIG_PINMUX) += pinmux.o
