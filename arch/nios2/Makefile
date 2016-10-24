-include $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/Makefile

ifdef CONFIG_GP_NONE
gpopt = none
else ifdef CONFIG_GP_LOCAL
gpopt = local
else ifdef CONFIG_GP_GLOBAL
gpopt = global
else ifdef CONFIG_GP_ALL_DATA
gpopt = data
endif

# Set Global Pointer option based on Kconfig.
arch_cflags += -mgpopt=$(gpopt)

# Put functions and data in their own binary sections so that ld can
# garbage collect them
arch_cflags += $(call cc-option,-ffunction-sections) \
	       $(call cc-option,-fdata-sections)

# TODO Find a way to pull this out of system.h somehow
# instead of having Kconfig for it
ifdef CONFIG_HAS_MUL_INSTRUCTION
arch_cflags += -mhw-mul
else
arch_cflags += -mno-hw-mul
endif
ifdef CONFIG_HAS_MULX_INSTRUCTION
arch_cflags += -mhw-mulx
else
arch_cflags += -mno-hw-mulx
endif
ifdef CONFIG_HAS_DIV_INSTRUCTION
arch_cflags += -mhw-div
else
arch_cflags += -mno-hw-div
endif

# Nios II CPUs are configurable and we need to pull in the generated
# headers system.h and linker.h which specify what is enabled and where
# everything is.
soc_ld_include := -I$(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/include
arch_cflags += $(soc_ld_include)
EXTRA_LINKER_CMD_OPT += $(soc_ld_include)

KBUILD_AFLAGS += $(arch_cflags)
KBUILD_CFLAGS += $(arch_cflags)
KBUILD_CXXFLAGS += $(arch_cflags)

soc-cxxflags ?= $(soc-cflags)
soc-aflags ?= $(soc-cflags)
KBUILD_CFLAGS += $(soc-cflags)
KBUILD_CXXFLAGS += $(soc-cxxflags)
KBUILD_AFLAGS += $(soc-aflags)

zephyr: $(KERNEL_HEX_NAME)
all: $(KERNEL_HEX_NAME)

QEMU_CPU_TYPE_nios2 = nios2
QEMU_FLAGS_nios2 = -machine altera_10m50_zephyr -nographic
QEMU_nios2 = qemu-system-nios2
