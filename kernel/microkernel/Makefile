EXTRA_CFLAGS +=-I$(srctree)/kernel/microkernel/include

obj-y = k_task.o idle.o

obj-$(CONFIG_TASK_MONITOR)    += task_monitor.o
obj-$(CONFIG_API_MEMMAP)      += k_memmap.o
obj-$(CONFIG_API_MEMPOOL)     += k_mempool.o
obj-$(CONFIG_API_TASKIRQ)     += k_irq.o
obj-$(CONFIG_API_DONOTHING)   += k_nop.o
obj-$(CONFIG_API_OFFLD2FIBER) += k_offload.o
obj-$(CONFIG_API_EVENTS)      += k_event.o
obj-$(CONFIG_API_MAILBOX)     += k_mbox.o
obj-$(CONFIG_API_MUTEX)       += k_mutex.o
obj-$(CONFIG_API_FIFO)        += k_queue.o
obj-$(CONFIG_API_SEMAPHORES)  += k_sema.o
obj-$(CONFIG_API_TIMER)       += k_timer.o

obj-y += core/
obj-y += movedata/
obj-$(CONFIG_API_CHANNEL) += channel/
obj-$(CONFIG_API_TIMER) += timer/
