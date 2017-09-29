ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/subsys/bluetooth

obj-$(CONFIG_BT_HCI_RAW) += hci_raw.o

obj-$(CONFIG_BT_DEBUG_MONITOR) += monitor.o

obj-$(CONFIG_BT_TINYCRYPT_ECC) += hci_ecc.o

obj-$(CONFIG_BT_INTERNAL_STORAGE) += storage.o

ifeq ($(CONFIG_BT_HCI_HOST),y)
	obj-y += uuid.o hci_core.o

	ifeq ($(CONFIG_BT_CONN),y)
		obj-y += conn.o l2cap.o att.o gatt.o

		ifeq ($(CONFIG_BT_SMP),y)
			obj-y += smp.o keys.o
		else
			obj-y += smp_null.o
		endif
	endif

	obj-$(CONFIG_BT_HOST_CRYPTO) += crypto.o
endif

obj-$(CONFIG_BT_BREDR) += keys_br.o l2cap_br.o sdp.o

obj-$(CONFIG_BT_RFCOMM) += rfcomm.o

obj-$(CONFIG_BT_HFP_HF) += at.o hfp_hf.o

obj-$(CONFIG_BT_AVDTP) += avdtp.o

obj-$(CONFIG_BT_A2DP) += a2dp.o

obj-$(CONFIG_BT_MESH) += mesh/
