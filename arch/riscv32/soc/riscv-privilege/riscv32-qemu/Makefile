soc-cflags := -I/$(srctree)/arch/$(ARCH)/soc/$(SOC_FAMILY)/common/

obj-y += qemu_irq.o vector.o

soc_ld_include := -I$(srctree)/arch/$(ARCH)/soc/$(SOC_FAMILY)/common/ \
		  -I$(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/
EXTRA_LINKER_CMD_OPT += $(soc_ld_include)
