# vim: filetype=make

ifneq ("$(wildcard $(MDEF_FILE))","")
MDEF_FILE_PATH=$(strip $(MDEF_FILE))
else
ifneq ($(MDEF_FILE),)
MDEF_FILE_PATH=$(strip $(PROJECT_BASE)/$(MDEF_FILE))
endif
endif

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
	echo "  TASKGROUP FPU_LEGACY";\
	echo $(TASKGROUP_SSE);\
	echo; \
	if test -e "$(MDEF_FILE_PATH)"; then \
		cat $(MDEF_FILE_PATH); \
	fi;)
endef

misc/generated/sysgen/prj.mdef:	$(MDEF_FILE_PATH) \
				include/config/auto.conf FORCE
	$(call filechk,prj.mdef)

sysgen_cmd=$(strip \
	$(PYTHON) $(srctree)/scripts/sysgen \
	-i $(CURDIR)/misc/generated/sysgen/prj.mdef \
	-o $(CURDIR)/misc/generated/sysgen/ \
)

misc/generated/sysgen/kernel_main.c: misc/generated/sysgen/prj.mdef \
				     $(srctree)/scripts/sysgen
	$(Q)$(sysgen_cmd)

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

ifeq ($(CONFIG_MDEF),y)
targets += misc/generated/sysgen/kernel_main.c
always += misc/generated/sysgen/kernel_main.c
endif

define rule_cc_o_c_1
	$(call echo-cmd,cc_o_c_1) $(cmd_cc_o_c_1);
endef

cmd_cc_o_c_1 = $(CC) $(KBUILD_CFLAGS) $(ZEPHYRINCLUDE) -c -o $@ $<

arch/$(ARCH)/core/offsets/offsets.o: arch/$(ARCH)/core/offsets/offsets.c $(KCONFIG_CONFIG)
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

