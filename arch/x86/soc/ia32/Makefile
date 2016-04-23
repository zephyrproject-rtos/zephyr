ifeq ($(CONFIG_X86_IAMCU),y)
LD_TOOLCHAIN ?= -D__GCC_LINKER_CMD__ -D__IAMCU
OUTPUT_FORMAT = elf32-iamcu
OUTPUT_ARCH = iamcu:intel
KBUILD_CFLAGS += $(call cc-option,-msoft-float)
endif

KBUILD_CFLAGS += $(call cc-option,-march=pentium)
KBUILD_CXXFLAGS += $(call cc-option,-march=pentium)
KBUILD_AFLAGS += $(KBUILD_CFLAGS)
