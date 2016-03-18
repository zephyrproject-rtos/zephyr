ccflags-y += -I$(srctree)/include/drivers
ccflags-y += -I$(srctree)/drivers
asflags-y := ${ccflags-y}

ccflags-$(CONFIG_NETWORKING_WITH_15_4_TI_CC2520) +=	\
				-I${srctree}/net/ip/contiki
ccflags-$(CONFIG_NETWORKING_WITH_15_4_TI_CC2520) +=	\
				-I${srctree}/net/ip/contiki/os/lib
ccflags-$(CONFIG_NETWORKING_WITH_15_4_TI_CC2520) +=	\
				-I${srctree}/net/ip/contiki/os
ccflags-$(CONFIG_NETWORKING_WITH_15_4_TI_CC2520) +=	\
				-I${srctree}/net/ip

obj-y += board.o
