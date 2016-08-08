ccflags-y +=-I$(srctree)/include/drivers

obj-$(CONFIG_BLUETOOTH_STACK_HCI) = \
	hci_core.o \
	uuid.o

obj-$(CONFIG_BLUETOOTH_DEBUG) += log.o

obj-$(CONFIG_BLUETOOTH_DEBUG_MONITOR) += monitor.o

obj-$(CONFIG_BLUETOOTH_TINYCRYPT_ECC) += hci_ecc.o

ifeq ($(CONFIG_BLUETOOTH_CONN),y)
	obj-y += conn.o l2cap.o att.o gatt.o

	ifeq ($(CONFIG_BLUETOOTH_SMP),y)
		obj-y += smp.o keys.o
	else
		obj-y += smp_null.o
	endif

	obj-$(CONFIG_BLUETOOTH_BREDR) += keys_br.o l2cap_br.o
endif
