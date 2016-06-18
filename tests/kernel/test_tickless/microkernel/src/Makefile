ccflags-y += -I${ZEPHYR_BASE}/tests/include

obj-y =  test_tickless.o 
ifeq ($(ARCH),arm)
obj-y += timestamps.o
endif
