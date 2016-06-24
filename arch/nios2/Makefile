-include $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/Makefile

# FIXME: Disable use of global pointer for now, this isn't properly set up
# in our linker script.
arch_cflags := $(call cc-option,-G0)

# Put functions and data in their own binary sections so that ld can
# garbage collect them
arch_cflags += $(call cc-option,-ffunction-sections) \
	       $(call cc-option,-fdata-sections)

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

KERNEL_HEX_NAME = $(KERNEL_NAME).hex

quiet_cmd_gen_hex = HEX     $@
      cmd_gen_hex =                                                         \
(                                                                           \
   $(OBJCOPY) -S -O ihex -R .note -R .comment -R COMMON -R .eh_frame $< $@; \
)

$(KERNEL_HEX_NAME): $(KERNEL_ELF_NAME)
	$(call cmd,gen_hex)

zephyr: $(KERNEL_HEX_NAME)
all: $(KERNEL_HEX_NAME)
export KERNEL_HEX_NAME

QEMU_CPU_TYPE_nios2 = nios2
QEMU_FLAGS_nios2 = -machine altera_10m50_zephyr \
		   -serial stdio
QEMU_nios2 = qemu-system-nios2
