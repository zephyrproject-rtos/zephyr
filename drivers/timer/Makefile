ccflags-y += -I$(srctree)/kernel/unified/include

obj-$(CONFIG_HPET_TIMER) += hpet.o
obj-$(CONFIG_LOAPIC_TIMER) += loapic_timer.o
obj-$(CONFIG_ARCV2_TIMER) += arcv2_timer0.o
obj-$(CONFIG_ALTERA_AVALON_TIMER) += altera_avalon_timer.o

_CORTEX_M_SYSTICK_AND_GDB_INFO_yy = y
obj-$(CONFIG_CORTEX_M_SYSTICK) += cortex_m_systick.o
obj-$(_CORTEX_M_SYSTICK_AND_GDB_INFO_$(CONFIG_CORTEX_M_SYSTICK)$(CONFIG_GDB_INFO)) += cortex_m_systick_gdb.o

obj-y += sys_clock_init.o
