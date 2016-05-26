ifeq ($(CONFIG_X86_IAMCU),y)
LD_TOOLCHAIN ?= -D__GCC_LINKER_CMD__ -D__IAMCU
OUTPUT_FORMAT = elf32-iamcu
OUTPUT_ARCH = iamcu:intel
soc-cflags = $(call cc-option,-msoft-float)
endif

soc-cflags += $(call cc-option,-march=pentium)
