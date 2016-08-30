
ifdef CONFIG_QMSI_LIBRARY
ZEPHYRINCLUDE += -I$(CONFIG_QMSI_INSTALL_PATH)/include
LIB_INCLUDE_DIR += -L$(CONFIG_QMSI_INSTALL_PATH:"%"=%)/lib
ALL_LIBS += qmsi
endif
ifdef CONFIG_QMSI_BUILTIN
ZEPHYRINCLUDE +=-I$(srctree)/ext/hal/qmsi/include
ZEPHYRINCLUDE +=-I$(srctree)/ext/hal/qmsi/drivers/include
ifeq ($(CONFIG_ARC),y)
ZEPHYRINCLUDE +=-I$(srctree)/ext/hal/qmsi/drivers/sensor/include
endif
ZEPHYRINCLUDE +=-I$(srctree)/ext/hal/qmsi/soc/$(patsubst %_ss,%,$(SOC_SERIES))/include/
endif
