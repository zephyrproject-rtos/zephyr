ccflags-y += -I$(srctree)/subsys/bluetooth/controller/include
ccflags-y += -I$(srctree)/subsys/bluetooth/controller
ccflags-y += -I$(srctree)/subsys/bluetooth

ccflags-$(CONFIG_BT_CTLR_FAST_ENC) += -Ofast

obj-y += crypto.o ctrl.o ll.o ll_filter.o
obj-$(CONFIG_BT_BROADCASTER) += ll_adv.o
obj-$(CONFIG_BT_OBSERVER)    += ll_scan.o
obj-$(CONFIG_BT_CENTRAL)     += ll_master.o
obj-$(CONFIG_BT_CTLR_DTM)    += ll_test.o
