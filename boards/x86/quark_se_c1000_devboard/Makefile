ccflags-y += -I$(srctree)/include/drivers
ccflags-y += -I$(srctree)/drivers
ccflags-y += -I$(srctree)/drivers/pinmux
asflags-y := ${ccflags-y}

ccflags-$(CONFIG_TI_CC2520_LEGACY) +=	\
				-I${srctree}/net/ip/contiki
ccflags-$(CONFIG_TI_CC2520_LEGACY) +=	\
				-I${srctree}/net/ip/contiki/os/lib
ccflags-$(CONFIG_TI_CC2520_LEGACY) +=	\
				-I${srctree}/net/ip/contiki/os
ccflags-$(CONFIG_TI_CC2520_LEGACY) +=	\
				-I${srctree}/net/ip

obj-$(CONFIG_PINMUX) += pinmux.o
obj-y += board.o
