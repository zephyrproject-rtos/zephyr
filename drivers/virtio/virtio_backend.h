
#define VIRTIO_DEVICE_ID_virtio_device1 1
#define VIRTIO_DEVICE_ID_virtio_device2 2
#define VIRTIO_DEVICE_ID_virtio_device3 3
#define VIRTIO_DEVICE_ID_virtio_device4 4
#define VIRTIO_DEVICE_ID_virtio_device5 5
#define VIRTIO_DEVICE_ID_virtio_device6 6
#define VIRTIO_DEVICE_ID_virtio_device7 7
#define VIRTIO_DEVICE_ID_virtio_device8 8
#define VIRTIO_DEVICE_ID_virtio_device9 9
#define VIRTIO_DEVICE_ID_virtio_devicea 10
#define VIRTIO_DEVICE_ID_virtio_deviceb 11
#define VIRTIO_DEVICE_ID_virtio_devicec 12
#define VIRTIO_DEVICE_ID_virtio_deviced 13
#define VIRTIO_DEVICE_ID_virtio_devicee 14
#define VIRTIO_DEVICE_ID_virtio_devicef 15
#define VIRTIO_DEVICE_ID_virtio_device10 16
#define VIRTIO_DEVICE_ID_virtio_device11 17
#define VIRTIO_DEVICE_ID_virtio_device12 18
#define VIRTIO_DEVICE_ID_virtio_device13 19
#define VIRTIO_DEVICE_ID_virtio_device14 20
#define VIRTIO_DEVICE_ID_virtio_device15 21
#define VIRTIO_DEVICE_ID_virtio_device16 22
#define VIRTIO_DEVICE_ID_virtio_device17 23
#define VIRTIO_DEVICE_ID_virtio_device18 24
#define VIRTIO_DEVICE_ID_virtio_device19 25
#define VIRTIO_DEVICE_ID_virtio_device1a 26
#define VIRTIO_DEVICE_ID_virtio_device1b 27
#define VIRTIO_DEVICE_ID_virtio_device1c 28
#define VIRTIO_DEVICE_ID_virtio_device1d 29
#define VIRTIO_DEVICE_ID_virtio_device1e 30
#define VIRTIO_DEVICE_ID_virtio_device1f 31
#define VIRTIO_DEVICE_ID_virtio_device20 32
#define VIRTIO_DEVICE_ID_virtio_device21 33
#define VIRTIO_DEVICE_ID_virtio_device22 34
#define VIRTIO_DEVICE_ID_virtio_device23 35
#define VIRTIO_DEVICE_ID_virtio_device24 36
#define VIRTIO_DEVICE_ID_virtio_device25 37
#define VIRTIO_DEVICE_ID_virtio_device26 38
#define VIRTIO_DEVICE_ID_virtio_device27 39
#define VIRTIO_DEVICE_ID_virtio_device28 40
#define VIRTIO_DEVICE_ID_virtio_device29 41
#define VIRTIO_DEVICE_ID_virtio_device2a 42
#define VIRTIO_DEVICE_ID_virtio_device2b 43
#define VIRTIO_DEVICE_ID_virtio_device2c 44
#define VIRTIO_DEVICE_ID_virtio_device2d 45
#define VIRTIO_DEVICE_ID_virtio_device2e 46
#define VIRTIO_DEVICE_ID_virtio_device2f 47

static const char* virtio_reg_name(size_t off)
{
	switch (off) {
	case 0x000:
		return "VIRTIO_MMIO_MAGIC_VALUE";
	case 0x004:
		return "VIRTIO_MMIO_VERSION";
	case 0x008:
		return "VIRTIO_MMIO_DEVICE_ID";
	case 0x00c:
		return "VIRTIO_MMIO_VENDOR_ID";
	case 0x010:
		return "VIRTIO_MMIO_DEVICE_FEATURES";
	case 0x014:
		return "VIRTIO_MMIO_DEVICE_FEATURES_SEL";
	case 0x020:
		return "VIRTIO_MMIO_DRIVER_FEATURES";
	case 0x024:
		return "VIRTIO_MMIO_DRIVER_FEATURES_SEL";
	case 0x030:
		return "VIRTIO_MMIO_QUEUE_SEL";
	case 0x034:
		return "VIRTIO_MMIO_QUEUE_SIZE_MAX";
	case 0x038:
		return "VIRTIO_MMIO_QUEUE_SIZE";
	case 0x044:
		return "VIRTIO_MMIO_QUEUE_READY";
	case 0x050:
		return "VIRTIO_MMIO_QUEUE_NOTIFY";
	case 0x060:
		return "VIRTIO_MMIO_INTERRUPT_STATUS";
	case 0x064:
		return "VIRTIO_MMIO_INTERRUPT_ACK";
	case 0x070:
		return "VIRTIO_MMIO_STATUS";
	case 0x080:
		return "VIRTIO_MMIO_QUEUE_DESC_LOW";
	case 0x084:
		return "VIRTIO_MMIO_QUEUE_DESC_HIGH";
	case 0x090:
		return "VIRTIO_MMIO_QUEUE_AVAIL_LOW";
	case 0x094:
		return "VIRTIO_MMIO_QUEUE_AVAIL_HIGH";
	case 0x0a0:
		return "VIRTIO_MMIO_QUEUE_USED_LOW";
	case 0x0a4:
		return "VIRTIO_MMIO_QUEUE_USED_HIGH";
	case 0x0ac:
		return "VIRTIO_MMIO_SHM_SEL";
	case 0x0b0:
		return "VIRTIO_MMIO_SHM_LEN_LOW";
	case 0x0b4:
		return "VIRTIO_MMIO_SHM_LEN_HIGH";
	case 0x0b8:
		return "VIRTIO_MMIO_SHM_BASE_LOW";
	case 0x0bc:
		return "VIRTIO_MMIO_SHM_BASE_HIGH";
	case 0x0c0:
		return "VIRTIO_MMIO_QUEUE_RESET";
	case 0x0fc:
		return "VIRTIO_MMIO_CONFIG_GENERATION";
	case 0x100:
		return "VIRTIO_MMIO_CONFIG";
	default:
		return "?????";
	}
}

