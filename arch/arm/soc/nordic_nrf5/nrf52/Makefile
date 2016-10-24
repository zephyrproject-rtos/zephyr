ZEPHYRINCLUDE += -I$(srctree)/arch/arm/soc/nordic_nrf5/include

ifdef CONFIG_SOC_SERIES_NRF52X
soc-cflags += -DNRF52
endif

obj-y += soc.o

zephyr: $(KERNEL_HEX_NAME)
all: $(KERNEL_HEX_NAME)
