ccflags-y += -I${ZEPHYR_BASE}/tests/include
DQUOTE = "
BUILD_ARCH = $(strip $(subst $(DQUOTE),,$(CONFIG_ARCH)))

obj-y += main.o
obj-y += asm_$(BUILD_ARCH).o
