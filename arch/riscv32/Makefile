-include $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/Makefile

# Put functions and data in their own binary sections so that ld can
# garbage collect them
arch_cflags += $(call cc-option,-ffunction-sections) \
	       $(call cc-option,-fdata-sections)

KBUILD_AFLAGS += $(arch_cflags)
KBUILD_CFLAGS += $(arch_cflags)
KBUILD_CXXFLAGS += $(arch_cflags)

soc-cxxflags ?= $(soc-cflags)
soc-aflags ?= $(soc-cflags)
KBUILD_CFLAGS += $(soc-cflags)
KBUILD_CXXFLAGS += $(soc-cxxflags)
KBUILD_AFLAGS += $(soc-aflags)

soc_ld_include := -I$(srctree)/arch/$(ARCH)/soc \
		  -I$(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/
EXTRA_LINKER_CMD_OPT += $(soc_ld_include)
