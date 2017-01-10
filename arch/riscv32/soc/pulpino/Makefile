soc-cflags := -I/$(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/

ifndef CONFIG_RISCV_GENERIC_TOOLCHAIN
soc-cflags += -march=IMXpulpv2
endif

KERNEL_S19_NAME = $(KERNEL_NAME).s19

quiet_cmd_gen_s19 = S19    $@
      cmd_gen_s19 =                                             \
(								\
	$(OBJCOPY) --srec-len 1 --output-target=srec $< $@;	\
)

$(KERNEL_S19_NAME): $(KERNEL_ELF_NAME)
	$(call cmd,gen_s19)

zephyr: $(KERNEL_S19_NAME)
all: $(KERNEL_S19_NAME)
export KERNEL_S19_NAME
