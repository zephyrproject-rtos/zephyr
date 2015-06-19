ccflags-y +=-I$(srctree)/kernel/microkernel/include

obj-y = k_task.o
obj-y += k_idle.o
obj-y += k_init.o
obj-y += k_command_packet.o
obj-y += k_move_data.o
obj-y += k_ticker.o
obj-y += k_memory_map.o
obj-y += k_memory_pool.o
obj-y += k_irq.o
obj-y += k_nop.o
obj-y += k_offload.o
obj-y += k_event.o
obj-y += k_mailbox.o
obj-y += k_mutex.o
obj-y += k_fifo.o
obj-y += k_semaphore.o
obj-y += k_timer.o
obj-y += k_pipe_buffer.o k_pipe.o k_pipe_get.o \
	k_pipe_put.o k_pipe_util.o k_pipe_xfer.o

obj-$(CONFIG_MICROKERNEL)  += k_server.o
obj-$(CONFIG_TASK_MONITOR) += k_task_monitor.o

