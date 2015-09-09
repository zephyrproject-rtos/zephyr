obj-y = buf.o \
	hci_core.o

ifeq ($(CONFIG_BLUETOOTH_CONN),y)
	obj-y += conn.o \
		l2cap.o \
		att.o \
		gatt.o \
		uuid.o

	ifeq ($(CONFIG_BLUETOOTH_SMP),y)
		obj-y += smp.o \
			keys.o
	else
		obj-y += smp_null.o
	endif
endif
