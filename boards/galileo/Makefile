ccflags-y += -I$(srctree)/include/drivers
ccflags-y += -I$(srctree)/drivers
asflags-y := ${ccflags-y}

obj-y += board.o
obj-$(CONFIG_PINMUX) += pinmux.o pinmux_galileo.o
obj-$(CONFIG_PINMUX_DEV_GALILEO) += pinmux_dev.o
