ccflags-y += -I$(srctree)/kernel/unified/include
asflags-y += -I$(srctree)/kernel/unified/include

ifdef CONFIG_ATOMIC_OPERATIONS_C
# Use C routines from kernel/nanokernel/atomic.c
obj-atomic=
else
# Use our own routines implmented in assembly
obj-atomic=atomic.o
endif
obj-y = ${obj-atomic} cpu_idle.o fatal.o \
	swap.o thread.o xt_zephyr.o	xtensa_context.o xtensa_intr_asm.o \
	xtensa_intr.o xtensa_overlay_os_hook.o xtensa_vectors.o irq_manage.o
obj-$(CONFIG_IRQ_OFFLOAD) += irq_offload.o
obj-$(CONFIG_SIMULATOR_XTENSA) += crt1-sim.o
obj-$(CONFIG_BOARD_XTENSA) += crt1-boards.o
# Keep this last so that vague linking works
obj-y += sw_isr_table.o
