ccflags-y += -I$(srctree)/subsys/bluetooth/controller/include
ccflags-y += -I$(srctree)/subsys/bluetooth/controller
ccflags-y += -I$(srctree)/subsys/bluetooth

obj-y += crypto.o ctrl.o ll.o
obj-$(CONFIG_BLUETOOTH_BROADCASTER)            += ll_adv.o
obj-$(CONFIG_BLUETOOTH_OBSERVER)               += ll_scan.o
obj-$(CONFIG_BLUETOOTH_CENTRAL)                += ll_master.o
