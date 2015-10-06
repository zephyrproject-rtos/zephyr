# vim: filetype=make

ifneq ($(strip $(CONFIG_MAX_NUM_TASK_IRQS)),)
ifneq (${CONFIG_MAX_NUM_TASK_IRQS},0)
TASK_IRQS=y
endif
endif

ifneq ("$(wildcard $(MDEF_FILE))","")
MDEF_FILE_PATH=$(MDEF_FILE)
else
ifneq ($(MDEF_FILE),)
MDEF_FILE_PATH=$(PROJECT_BASE)/$(MDEF_FILE)
endif
endif

define filechk_prj.mdef
	(echo "% WARNING. THIS FILE IS AUTO-GENERATED. DO NOT MODIFY!"; \
	echo; \
	echo "% CONFIG NUM_COMMAND_PACKETS NUM_TIMER_PACKETS NUM_TASK_PRIORITIES"; \
	echo "% ============================================================="; \
	echo "  CONFIG ${CONFIG_NUM_COMMAND_PACKETS}             ${CONFIG_NUM_TIMER_PACKETS}               ${CONFIG_NUM_TASK_PRIORITIES}"; \
	echo; \
	echo "% TASKGROUP NAME";\
	echo "% ==============";\
	echo "  TASKGROUP EXE";\
	echo "  TASKGROUP SYS";\
	echo "  TASKGROUP FPU";\
	echo "  TASKGROUP SSE";\
	echo; \
	if test "$(TASK_IRQS)" = "y"; then \
		echo "% Task IRQ objects";\
		echo "% EVENT    NAME              HANDLER"; \
		echo "% ======================================="; \
		i=0; \
		while [ $$i -lt $(CONFIG_MAX_NUM_TASK_IRQS) ]; do \
			echo "  EVENT    _TaskIrqEvt$$i     NULL"; \
		i=$$(($$i+1));\
		done; \
	fi; \
	if test -e "$(MDEF_FILE_PATH)"; then \
		cat $(MDEF_FILE_PATH); \
	fi;)
endef

misc/generated/sysgen/prj.mdef:	$(MDEF_FILE_PATH) \
				include/config/auto.conf FORCE
	$(call filechk,prj.mdef)

misc/generated/sysgen/kernel_main.c: misc/generated/sysgen/prj.mdef
	$(Q)$(srctree)/scripts/sysgen $(CURDIR)/misc/generated/sysgen/prj.mdef $(CURDIR)/misc/generated/sysgen/

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
targets += include/generated/offsets.h


always := misc/generated/configs.c
always += include/generated/offsets.h

ifeq ($(CONFIG_MICROKERNEL),y)
targets += misc/generated/sysgen/kernel_main.c
always += misc/generated/sysgen/kernel_main.c
endif

define rule_cc_o_c_1
	$(call echo-cmd,cc_o_c_1) $(cmd_cc_o_c_1);
endef

OFFSETS_INCLUDE = $(strip \
		-include $(CURDIR)/include/generated/autoconf.h \
		-I $(srctree)/include \
		-I $(CURDIR)/include/generated \
		-I $(srctree)/kernel/microkernel/include \
		-I $(srctree)/kernel/nanokernel/include \
		-I $(srctree)/lib/libc/minimal/include \
		-I $(srctree)/arch/${SRCARCH}/include )

cmd_cc_o_c_1 = $(CC) $(KBUILD_CFLAGS) $(OFFSETS_INCLUDE) -c -o $@ $<

arch/$(SRCARCH)/core/offsets/offsets.o: arch/$(SRCARCH)/core/offsets/offsets.c
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

include/generated/offsets.h: $(GENOFFSET_H) arch/$(SRCARCH)/core/offsets/offsets.o \
					include/config/auto.conf FORCE
	$(call offsetchk,arch/$(SRCARCH)/core/offsets/offsets.o)

