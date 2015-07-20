ccflags-y +=-I$(srctree)/include/drivers
ccflags-y +=-I$(srctree)/arch/$(ARCH)/$(subst $(DQUOTE),,$(CONFIG_PLATFORM))
asflags-y +=-I$(srctree)/arch/$(ARCH)/$(subst $(DQUOTE),,$(CONFIG_PLATFORM))

obj-${CONFIG_PIC}${CONFIG_SHUTOFF_PIC} = i8259.o

obj-$(CONFIG_PIC) += i8259_boi.o
obj-$(CONFIG_PIC) += system_pic.o
obj-$(CONFIG_LOAPIC) += system_apic.o
obj-$(CONFIG_LOAPIC) += loapic_intr.o
obj-$(CONFIG_IOAPIC) += ioapic_intr.o

obj-$(CONFIG_ARCV2_INTERRUPT_UNIT) += arcv2_irq_unit.o
