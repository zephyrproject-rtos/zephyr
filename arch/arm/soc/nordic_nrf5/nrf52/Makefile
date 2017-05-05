ZEPHYRINCLUDE += -I$(srctree)/arch/arm/soc/nordic_nrf5/include

ifdef CONFIG_SOC_NRF52832
soc-cflags += -DNRF52832_XXAA
soc-cflags += -DNRF52832
endif

ifdef CONFIG_SOC_NRF52840
soc-cflags += -DNRF52840_XXAA
endif

obj-y += soc.o
obj-$(CONFIG_ARM_MPU_NRF52X) += mpu_regions.o

zephyr: $(KERNEL_HEX_NAME)
all: $(KERNEL_HEX_NAME)
