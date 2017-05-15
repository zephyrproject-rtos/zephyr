ccflags-y += -I$(srctree)/subsys/bluetooth/controller/include
ccflags-y += -I$(srctree)/subsys/bluetooth/controller
ccflags-y += -I$(srctree)/subsys/bluetooth

obj-y += crypto.o ctrl.o ll.o
obj-$(CONFIG_BLUETOOTH_CONTROLLER_STATE_ADV)   += ll_adv.o
obj-$(CONFIG_BLUETOOTH_CONTROLLER_STATE_SCAN)  += ll_scan.o
obj-$(CONFIG_BLUETOOTH_CENTRAL)                += ll_master.o
