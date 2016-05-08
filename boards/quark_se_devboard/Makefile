ccflags-y += -I$(srctree)/include/drivers
ccflags-y += -I$(srctree)/drivers
ccflags-y += -I$(srctree)/drivers/pinmux
asflags-y := ${ccflags-y}

ccflags-$(CONFIG_NETWORKING_WITH_15_4_TI_CC2520) +=	\
				-I${srctree}/net/ip/contiki
ccflags-$(CONFIG_NETWORKING_WITH_15_4_TI_CC2520) +=	\
				-I${srctree}/net/ip/contiki/os/lib
ccflags-$(CONFIG_NETWORKING_WITH_15_4_TI_CC2520) +=	\
				-I${srctree}/net/ip/contiki/os
ccflags-$(CONFIG_NETWORKING_WITH_15_4_TI_CC2520) +=	\
				-I${srctree}/net/ip

obj-$(CONFIG_PINMUX) += pinmux.o
obj-y += board.o
