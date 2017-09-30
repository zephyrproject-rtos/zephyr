BOARD ?= qemu_x86

ifeq ($(BOARD),qemu_x86)
	CFLAGS += -DTEST_FLASH_OFFSET=0
	CFLAGS += -DFLASH_AREA_NFFS_OFFSET=0
	CFLAGS += -DFLASH_AREA_NFFS_SIZE=1048576
else ifeq ($(BOARD),nrf51_pca10028)
	CONF_FILE ?= nrf5x.conf
	CFLAGS += -DTEST_FLASH_OFFSET=0x20000
else ifeq ($(BOARD),nrf52_pca10040)
	CONF_FILE ?= nrf5x.conf
	CFLAGS += -DTEST_FLASH_OFFSET=0x20000
else ifeq ($(BOARD),nrf52840_pca10056)
	CONF_FILE ?= nrf5x.conf
	CFLAGS += -DTEST_FLASH_OFFSET=0x20000
endif

CONF_FILE ?= prj.conf

ifeq ($(TEST),)
	CFLAGS += -DTEST_basic
else
	CFLAGS += -DTEST_$(TEST)
endif

include ${ZEPHYR_BASE}/Makefile.inc
