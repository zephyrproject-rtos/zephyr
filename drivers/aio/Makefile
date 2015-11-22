ccflags-y +=-I$(srctree)/arch/$(ARCH)
ccflags-y +=-I$(srctree)/arch/$(ARCH)/platforms/$(subst $(DQUOTE),,$(CONFIG_PLATFORM))
asflags-y +=-I$(srctree)/arch/$(ARCH)/platforms/$(subst $(DQUOTE),,$(CONFIG_PLATFORM))

obj-$(CONFIG_AIO_DW_COMPARATOR) += aio_dw_comparator.o aio_static_irq_stubs.o
