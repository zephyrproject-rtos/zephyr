SOC_PATH=${XTENSA_CORE}
export SOC_PATH

flagBoardType=
ifeq ($(CONFIG_BOARD_XTENSA),y)
flagBoardType= -DXT_BOARD
endif
ifeq ($(CONFIG_SIMULATOR_XTENSA),y)
flagBoardType= -DXT_SIMULATOR
endif

# Remove compilation flags added by top makefile and which are not supported by xcc and enable.
KBUILD_CFLAGS := $(filter-out -fno-asynchronous-unwind-tables -fno-omit-frame-pointer,${KBUILD_CFLAGS})
# Add Xtensa pecific flags
KBUILD_AFLAGS += -c -xassembler-with-cpp $(flagBoardType) $(flagALongCall)
KBUILD_CFLAGS += -DPROC_$(XTENSA_CORE) -DCONFIG_$(XTENSA_CORE) -c $(flagLongCall) -nostdinc $(flagBoardType) $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,) $(call cc-option,-fms-extensions,)

include $(srctree)/arch/$(ARCH)/core/Makefile
include $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/Makefile

cflags-$(CONFIG_LTO) += $(call cc-option,-flto,)

KBUILD_CFLAGS += $(cflags-y)
KBUILD_CXXFLAGS += $(cflags-y)

QEMU_CPU_TYPE_xtensa ?= unsupported
QEMU_FLAGS_xtensa = -cpu $(QEMU_CPU_TYPE_xtensa) \
                    -machine sim -semihosting -nographic
QEMU_xtensa = qemu-system-xtensa

export QEMU_FLAGS_xtensa QEMU_xtensa
