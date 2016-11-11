ccflags-y += -I$(CURDIR)/misc/generated/sysgen
ccflags-y += -I$(ZEPHYR_BASE)/tests/legacy/benchmark/latency_measure/src \
	-I${ZEPHYR_BASE}/tests/include

obj-y := fifo_b.o mailbox_b.o master.o mempool_b.o \
	pipe_r.o sema_r.o event_b.o \
	fifo_r.o mailbox_r.o  memmap_b.o  mutex_b.o \
	pipe_b.o  receiver.o  sema_b.o
