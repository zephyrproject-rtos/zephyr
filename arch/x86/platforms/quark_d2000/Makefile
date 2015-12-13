ccflags-y +=-I$(srctree)/arch/x86
ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/drivers
asflags-y := ${ccflags-y}

obj-$(CONFIG_PLATFORM_QUARK_D2000) = platform.o
obj-$(CONFIG_PINMUX) += pinmux.o
