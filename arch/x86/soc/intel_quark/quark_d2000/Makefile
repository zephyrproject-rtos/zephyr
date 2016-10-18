ifeq ($(COMPILER),clang)
soc-cflags = $(call cc-option,-msoft-float, -march=pentium)
else
soc-cflags = $(call cc-option,-march=lakemont -mtune=lakemont -msoft-float, -march=pentium)
endif

ifeq ($(CONFIG_X86_IAMCU),y)
soc-cflags += $(call cc-option,-miamcu)
LD_TOOLCHAIN ?= -D__GCC_LINKER_CMD__ -D__IAMCU
OUTPUT_FORMAT = elf32-iamcu
OUTPUT_ARCH = iamcu:intel
else
soc-cflags += $(call cc-option,-mno-iamcu)
endif
soc-cflags += -DQM_LAKEMONT=1

SOC_SERIES = quark_d2000
