PROJECT_BASE = $(shell pwd)
ARCH?=x86

export ARCH CFLAGS LDFLAGS KBUILD_DEFCONFIG

all: FORCE
	$(Q)$(MAKE) -C $(TIMO_BASE)  O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) VPFILE=$(VPFILE)

rm-files:= final-linker.cmd linker.cmd modules.order \
	staticIdt.o System.map tinymountain.lnk \
	tinymountain.map tinymountain.elf
rm-dirs := arch drivers include kernel lib misc \
	scripts source

clean: FORCE
	$(shell rm $(rm-files))
	$(shell rm $(rm-dirs) -r)

mrproper: clean
	$(Q)$(MAKE) -C $(TIMO_BASE) \
	PROJECT=$(PROJECT_BASE) VPFILE=$(VPFILE) mrproper

%config: FORCE
	$(Q)$(MAKE) -C $(TIMO_BASE) O=$(PROJECT_BASE) \
	PROJECT=$(PROJECT_BASE) VPFILE=$(VPFILE) $@

PHONY += FORCE
FORCE:

.PHONY: $(PHONY)
