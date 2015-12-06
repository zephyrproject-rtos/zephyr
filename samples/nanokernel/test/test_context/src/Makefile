ccflags-y += -I${srctree}/samples/include
ccflags-y +=-I$(srctree)/arch/$(ARCH)/platforms/$(PLATFORM_NAME)
ccflags-$(CONFIG_PLATFORM_QUARK_D2000) += -DFIBER_STACKSIZE=256

obj-y = context.o
