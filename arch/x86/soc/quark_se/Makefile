arch-cflags = $(call cc-option,-march=lakemont -mtune=lakemont -msoft-float, -march=pentium)

ifeq ($(CONFIG_X86_IAMCU),y)
arch-cflags += $(call cc-option,-miamcu)
LD_TOOLCHAIN ?= -D__GCC_LINKER_CMD__ -D__IAMCU
OUTPUT_FORMAT = elf32-iamcu
OUTPUT_ARCH = iamcu:intel
else
arch-cflags += $(call cc-option,-mno-iamcu)
endif
KBUILD_CFLAGS += $(arch-cflags)
KBUILD_CXXFLAGS += $(arch-cflags)
KBUILD_AFLAGS += $(KBUILD_CFLAGS)
