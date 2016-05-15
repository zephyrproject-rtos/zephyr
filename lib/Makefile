
ifdef CONFIG_TINYCRYPT
ZEPHYRINCLUDE += -I$(srctree)/lib/crypto/tinycrypt/include
endif

ifdef CONFIG_QMSI_LIBRARY
LIB_INCLUDE_DIR += -L$(CONFIG_QMSI_INSTALL_PATH:"%"=%)/lib
ALL_LIBS += qmsi
endif

ifdef CONFIG_MINIMAL_LIBC
ZEPHYRINCLUDE += -I$(srctree)/lib/libc/minimal/include
endif

ifdef CONFIG_NEWLIB_LIBC
ZEPHYRINCLUDE += $(TOOLCHAIN_CFLAGS)
ALL_LIBS += m c
endif
