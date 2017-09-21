ccflags-y += -I$(srctree)/subsys/net/lib/dns

obj-y := dns_pack.o
obj-$(CONFIG_DNS_RESOLVER) += resolve.o

ccflags-$(CONFIG_MDNS_RESPONDER) += -I$(srctree)/subsys/net/ip
obj-$(CONFIG_MDNS_RESPONDER) += mdns_responder.o
