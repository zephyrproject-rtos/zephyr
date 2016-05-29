obj-y += soc.o

KERNEL_HEX_NAME = $(KERNEL_NAME).hex

quiet_cmd_gen_hex = HEX     $@
      cmd_gen_hex =                                                         \
(                                                                           \
   $(OBJCOPY) -S -O ihex -R .note -R .comment -R COMMON -R .eh_frame $< $@; \
)

$(KERNEL_HEX_NAME): $(KERNEL_ELF_NAME)
	$(call cmd,gen_hex)

zephyr: $(KERNEL_HEX_NAME)
all: $(KERNEL_HEX_NAME)
