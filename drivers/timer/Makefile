ccflags-y +=-I$(srctree)/arch/$(ARCH)
ccflags-y +=-I$(srctree)/arch/$(ARCH)/$(subst $(DQUOTE),,$(CONFIG_PLATFORM))

obj-$(CONFIG_PIT) = i8253.o
obj-$(CONFIG_HPET_TIMER) += hpet.o
obj-$(CONFIG_LOAPIC_TIMER) += loapic_timer.o
obj-$(CONFIG_ARCV2_TIMER) += arcv2_timer0.o
