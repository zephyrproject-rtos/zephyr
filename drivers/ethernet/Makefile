ccflags-y += -I${srctree}/net/ip/contiki
ccflags-y += -I${srctree}/net/ip/contiki/os/lib
ccflags-y += -I${srctree}/net/ip/contiki/os
ccflags-y += -I${srctree}/net/ip
ccflags-y += -I${srctree}

obj-$(CONFIG_ETH_DW) += eth_dw.o
ifdef CONFIG_NET_YAIP
obj-$(CONFIG_ETH_ENC28J60) += eth_enc28j60.o
else
obj-$(CONFIG_ETH_KSDK) += eth_ksdk.o
obj-$(CONFIG_ETH_ENC28J60) += eth_enc28j60_legacy.o
endif
