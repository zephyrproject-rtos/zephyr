# vim: filetype=make

ifeq (${CONFIG_NUM_COMMAND_PACKETS},)
CONFIG_NUM_COMMAND_PACKETS=0
endif
ifeq (${CONFIG_NUM_TIMER_PACKETS},)
CONFIG_NUM_TIMER_PACKETS=0
endif
ifeq (${CONFIG_NUM_TASK_PRIORITIES},)
CONFIG_NUM_TASK_PRIORITIES=$(CONFIG_NUM_PREEMPT_PRIORITIES)
endif

ifeq ($(ARCH),x86)
TASKGROUP_SSE="  TASKGROUP SSE"
endif

define filechk_configs.c
	(echo "/* file is auto-generated, do not modify ! */"; \
	echo; \
	echo "#include <toolchain.h>"; \
	echo; \
	echo "GEN_ABS_SYM_BEGIN (_ConfigAbsSyms)"; \
	echo; \
	cat $(CURDIR)/include/generated/autoconf.h | sed \
	's/".*"/1/' | awk  \
	'/#define/{printf "GEN_ABSOLUTE_SYM(%s, %s);\n", $$2, $$3}'; \
	echo; \
	echo "GEN_ABS_SYM_END";)
endef

misc/generated/configs.c: include/config/auto.conf FORCE
	$(call filechk,configs.c)

targets := misc/generated/configs.c
targets += include/generated/generated_dts_board.h
targets += include/generated/offsets.h


always := misc/generated/configs.c
always += include/generated/generated_dts_board.h
always += include/generated/offsets.h

define rule_cc_o_c_1
	$(call echo-cmd,cc_o_c_1) $(cmd_cc_o_c_1);
endef

cmd_cc_o_c_1 = $(CC) $(KBUILD_CPPFLAGS) $(KBUILD_CFLAGS) $(ZEPHYRINCLUDE) -c -o $@ $<

arch/$(ARCH)/core/offsets/offsets.o: arch/$(ARCH)/core/offsets/offsets.c $(KCONFIG_CONFIG) \
				include/generated/generated_dts_board.h
	$(Q)mkdir -p $(dir $@)
	$(call if_changed,cc_o_c_1)


define offsetchk
	$(Q)set -e;                             \
	$(kecho) '  CHK     $@';                \
	mkdir -p $(dir $@);                     \
	$(GENOFFSET_H) -i $(1) -o $@.tmp;       \
	if [ -r $@ ] && cmp -s $@ $@.tmp; then  \
	rm -f $@.tmp;                           \
	else                                    \
	$(kecho) '  UPD     $@';                \
	mv -f $@.tmp $@;                        \
	fi
endef

include/generated/offsets.h: arch/$(ARCH)/core/offsets/offsets.o \
			     include/config/auto.conf FORCE
	$(call offsetchk,arch/$(ARCH)/core/offsets/offsets.o)

ifeq ($(CONFIG_HAS_DTS),y)
define filechk_generated_dts_board.h
	(echo "/* WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY! */"; \
		$(ZEPHYR_BASE)/scripts/extract_dts_includes.py dts/$(ARCH)/$(BOARD_NAME).dts_compiled $(ZEPHYR_BASE)/dts/$(ARCH)/yaml; \
		if test -e $(ZEPHYR_BASE)/dts/$(ARCH)/$(BOARD_NAME).fixup; then \
			echo; echo; \
			echo "/* Following definitions fixup the generated include */"; \
			echo; \
			cat $(ZEPHYR_BASE)/dts/$(ARCH)/$(BOARD_NAME).fixup; \
		fi; \
		)
endef
else
define filechk_generated_dts_board.h
	(echo "/* WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY! */";)
endef
endif


include/generated/generated_dts_board.h: include/config/auto.conf FORCE
ifeq ($(CONFIG_HAS_DTS),y)
	$(Q)$(MAKE) $(build)=dts/$(ARCH)
endif
	$(call filechk,generated_dts_board.h)

