EXTRA_CFLAGS +=-I$(srctree)/kernel/microkernel/include

obj-y = k_task.o
obj-y += idle.o
obj-y += global.o
obj-y += cmdPkt.o
obj-y += movedata.o
obj-y += ticker.o
obj-y += k_memmap.o
obj-y += k_mempool.o
obj-y += k_irq.o
obj-y += k_nop.o
obj-y += k_offload.o
obj-y += k_event.o
obj-y += k_mbox.o
obj-y += k_mutex.o
obj-y += k_queue.o
obj-y += k_sema.o
obj-y += k_timer.o
obj-$(CONFIG_MICROKERNEL)  += server.o
obj-$(CONFIG_TASK_MONITOR) += task_monitor.o

obj-y += channel/
