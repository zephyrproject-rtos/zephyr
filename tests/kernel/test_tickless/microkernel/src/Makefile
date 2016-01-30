ccflags-y += -I${srctree}/tests/include

obj-y =  test_tickless.o 
ifeq ($(ARCH),arm)
obj-y += timestamps.o
endif
