PROJECT_BASE = $(shell pwd)
ARCH?=x86

export ARCH VPFILE

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
	  else if [ -x /bin/bash ]; then echo /bin/bash; \
	  else echo sh; fi ; fi)

all: FORCE mergeconfig
	$(Q)$(MAKE) -C $(TIMO_BASE)  O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE)

rm-files:= final-linker.cmd linker.cmd modules.order \
	staticIdt.o System.map *kernel.lnk \
	*kernel.map *kernel.elf
rm-dirs := arch drivers include kernel lib misc \
	scripts source

clean: FORCE
	$(shell rm $(rm-files) -f)
	$(shell rm $(rm-dirs) -r)

mrproper: clean
	$(Q)$(MAKE) -C $(TIMO_BASE) \
	PROJECT=$(PROJECT_BASE) mrproper

%config: FORCE
	$(Q)$(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) $@

qemu: FORCE
	$(Q)$(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) qemu

mergeconfig:
	$(Q)$(CONFIG_SHELL) $(TIMO_BASE)/scripts/kconfig/merge_config.sh \
	-m -O $(PROJECT_BASE) $(PROJECT_BASE)/.config $(PROJECT_BASE)/prj_$(ARCH).conf
	$(Q)yes "" | $(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) oldconfig


PHONY += FORCE
FORCE:

.PHONY: $(PHONY)
