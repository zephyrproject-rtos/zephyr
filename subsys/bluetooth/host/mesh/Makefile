ccflags-y +=-I$(srctree)/subsys/bluetooth

obj-$(CONFIG_BT_MESH) += main.o \
				adv.o \
				beacon.o \
				net.o \
				transport.o \
				crypto.o \
				access.o \
				cfg.o \
				health.o \

obj-$(CONFIG_BT_MESH_LOW_POWER) += lpn.o

obj-$(CONFIG_BT_MESH_FRIEND) += friend.o

obj-$(CONFIG_BT_MESH_PROV) += prov.o

obj-$(CONFIG_BT_MESH_PROXY) += proxy.o

obj-$(CONFIG_BT_MESH_SELF_TEST) += test.o
