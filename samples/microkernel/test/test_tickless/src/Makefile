ccflags-y += ${PROJECTINCLUDE} -I${srctree}/samples/include

obj-y =  test_tickless.o 
ifeq ($(ARCH),arm)
obj-y += timestamps.o
endif
