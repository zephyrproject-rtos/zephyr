ccflags-y += -I$(srctree)/include/drivers
ccflags-y += -I$(srctree)/drivers
ccflags-y += -I$(srctree)/drivers/pinmux
asflags-y := ${ccflags-y}

obj-$(CONFIG_PINMUX) += pinmux.o
obj-y += board.o
