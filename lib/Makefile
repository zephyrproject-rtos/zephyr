ifdef CONFIG_MINIMAL_LIBC
ZEPHYRINCLUDE += -I$(srctree)/lib/libc/minimal/include
endif

ifdef CONFIG_NEWLIB_LIBC
ZEPHYRINCLUDE += $(TOOLCHAIN_CFLAGS)
ALL_LIBS += m c
endif

include $(srctree)/lib/iot/Makefile
