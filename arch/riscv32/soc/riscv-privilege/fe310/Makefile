soc-cflags := -I/$(srctree)/arch/$(ARCH)/soc/$(SOC_FAMILY)/common/

obj-y += vector.o fe310_idle.o

soc_ld_include := -I$(srctree)/arch/$(ARCH)/soc/$(SOC_FAMILY)/common/ \
		  -I$(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/
EXTRA_LINKER_CMD_OPT += $(soc_ld_include)
