ifdef CONFIG_NEWLIB_LIBC
ZEPHYRINCLUDE += $(TOOLCHAIN_CFLAGS)
ALL_LIBS += m c

ifdef CONFIG_NEWLIB_LIBC_FLOAT_PRINTF
LDFLAGS += -u _printf_float
endif

ifdef CONFIG_NEWLIB_LIBC_FLOAT_SCANF
LDFLAGS += -u _scanf_float
endif

else
ZEPHYRINCLUDE += -I$(srctree)/lib/libc/minimal/include
endif
