ZEPHYRINCLUDE += -I$(srctree)/arch/arm/soc/st_stm32/common

obj-y += soc.o

obj-$(CONFIG_GPIO) += soc_gpio.o
obj-$(CONFIG_PINMUX) += soc_pinmux.o
obj-$(CONFIG_STM32_ARM_MPU_ENABLE) += arm_mpu_regions.o

zephyr: $(KERNEL_HEX_NAME)
all: $(KERNEL_HEX_NAME)
