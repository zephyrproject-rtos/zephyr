SOC_PATH=${XTENSA_CORE}
export SOC_PATH

flagBoardType=
ifeq ($(CONFIG_BOARD_XTENSA),y)
flagBoardType= -DXT_BOARD
endif
ifeq ($(CONFIG_SIMULATOR_XTENSA),y)
flagBoardType= -DXT_SIMULATOR
endif

# XCC emits an annoying warning if this is used even though the
# $(call cc-option,) test in toplevel Makefile passes.
KBUILD_CFLAGS := $(filter-out -fno-omit-frame-pointer, \
			      ${KBUILD_CFLAGS})

# Put functions and data in their own binary sections so that ld can
# garbage collect them
KBUILD_CFLAGS += $(call cc-option,-ffunction-sections,) \
		 $(call cc-option,-fdata-sections,)

KBUILD_AFLAGS += $(flagBoardType)
KBUILD_CFLAGS += -DPROC_$(XTENSA_CORE) \
		 -DCONFIG_$(XTENSA_CORE) \
		 $(flagBoardType) \
		 $(call cc-option,-fms-extensions,)

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
