#!/bin/sh
# Generate tags or cscope files
# Usage tags.sh <mode>
#
# mode may be any of: tags, TAGS, cscope
#
# Uses the following environment variables:
# ARCH, SOC, BOARD, ZEPHYR_BASE


find_sources()
{
	find "$1" -name "$2"
}

# Looks for assembly, c and header files of selected architectures
all_sources()
{
	for dir in $ALLSOURCES
	do
		find_sources "$dir" '*.[chS]'
	done
}

all_target_sources()
{
	all_sources
}

all_kconfigs()
{
	for dir in $ALLSOURCES; do
		find_sources $dir 'Kconfig*'
	done
}

all_defconfigs()
{
	for dir in $ALLSOURCES; do
		find_sources $dir 'defconfig*'
	done
}

docscope()
{
	(echo \-k; echo \-q; all_target_sources) > cscope.files
	cscope -b -f cscope.out
}

dogtags()
{
	all_target_sources | gtags -i -f -
}

exuberant()
{
	all_target_sources | xargs $1 -a                        \
	-I __initdata,__exitdata,__initconst,			\
	-I __initdata_memblock					\
	-I __refdata,__attribute,__maybe_unused,__always_unused \
	-I __acquires,__releases,__deprecated			\
	-I __read_mostly,__aligned,____cacheline_aligned        \
	-I ____cacheline_aligned_in_smp                         \
	-I __cacheline_aligned,__cacheline_aligned_in_smp	\
	-I ____cacheline_internodealigned_in_smp                \
	-I __used,__packed,__packed2__,__must_check,__must_hold	\
	-I EXPORT_SYMBOL,EXPORT_SYMBOL_GPL,ACPI_EXPORT_SYMBOL   \
	-I DEFINE_TRACE,EXPORT_TRACEPOINT_SYMBOL,EXPORT_TRACEPOINT_SYMBOL_GPL \
	-I static,const						\
	--extra=+f --c-kinds=+px                                \
	--regex-asm='/^(ENTRY|_GLOBAL)\(([^)]*)\).*/\2/'        \
	--regex-c='/^SYSCALL_DEFINE[[:digit:]]?\(([^,)]*).*/sys_\1/' \
	--regex-c='/^COMPAT_SYSCALL_DEFINE[[:digit:]]?\(([^,)]*).*/compat_sys_\1/' \
	--regex-c++='/^TRACE_EVENT\(([^,)]*).*/trace_\1/'		\
	--regex-c++='/^TRACE_EVENT\(([^,)]*).*/trace_\1_rcuidle/'	\
	--regex-c++='/^DEFINE_EVENT\([^,)]*, *([^,)]*).*/trace_\1/'	\
	--regex-c++='/^DEFINE_EVENT\([^,)]*, *([^,)]*).*/trace_\1_rcuidle/' \
	--regex-c++='/PAGEFLAG\(([^,)]*).*/Page\1/'			\
	--regex-c++='/PAGEFLAG\(([^,)]*).*/SetPage\1/'			\
	--regex-c++='/PAGEFLAG\(([^,)]*).*/ClearPage\1/'		\
	--regex-c++='/TESTSETFLAG\(([^,)]*).*/TestSetPage\1/'		\
	--regex-c++='/TESTPAGEFLAG\(([^,)]*).*/Page\1/'			\
	--regex-c++='/SETPAGEFLAG\(([^,)]*).*/SetPage\1/'		\
	--regex-c++='/__SETPAGEFLAG\(([^,)]*).*/__SetPage\1/'		\
	--regex-c++='/TESTCLEARFLAG\(([^,)]*).*/TestClearPage\1/'	\
	--regex-c++='/__TESTCLEARFLAG\(([^,)]*).*/TestClearPage\1/'	\
	--regex-c++='/CLEARPAGEFLAG\(([^,)]*).*/ClearPage\1/'		\
	--regex-c++='/__CLEARPAGEFLAG\(([^,)]*).*/__ClearPage\1/'	\
	--regex-c++='/__PAGEFLAG\(([^,)]*).*/__SetPage\1/'		\
	--regex-c++='/__PAGEFLAG\(([^,)]*).*/__ClearPage\1/'		\
	--regex-c++='/PAGEFLAG_FALSE\(([^,)]*).*/Page\1/'		\
	--regex-c++='/TESTSCFLAG\(([^,)]*).*/TestSetPage\1/'		\
	--regex-c++='/TESTSCFLAG\(([^,)]*).*/TestClearPage\1/'		\
	--regex-c++='/SETPAGEFLAG_NOOP\(([^,)]*).*/SetPage\1/'		\
	--regex-c++='/CLEARPAGEFLAG_NOOP\(([^,)]*).*/ClearPage\1/'	\
	--regex-c++='/__CLEARPAGEFLAG_NOOP\(([^,)]*).*/__ClearPage\1/'	\
	--regex-c++='/TESTCLEARFLAG_FALSE\(([^,)]*).*/TestClearPage\1/' \
	--regex-c++='/__TESTCLEARFLAG_FALSE\(([^,)]*).*/__TestClearPage\1/' \
	--regex-c++='/_PE\(([^,)]*).*/PEVENT_ERRNO__\1/'		\
	--regex-c++='/TASK_PFA_TEST\([^,]*,\s*([^)]*)\)/task_\1/'	\
	--regex-c++='/TASK_PFA_SET\([^,]*,\s*([^)]*)\)/task_set_\1/'	\
	--regex-c++='/TASK_PFA_CLEAR\([^,]*,\s*([^)]*)\)/task_clear_\1/'\
	--regex-c='/PCI_OP_READ\((\w*).*[1-4]\)/pci_bus_read_config_\1/' \
	--regex-c='/PCI_OP_WRITE\((\w*).*[1-4]\)/pci_bus_write_config_\1/' \
	--regex-c='/DEFINE_(MUTEX|SEMAPHORE|SPINLOCK)\((\w*)/\2/v/'	\
	--regex-c='/DEFINE_(RAW_SPINLOCK|RWLOCK|SEQLOCK)\((\w*)/\2/v/'	\
	--regex-c='/DECLARE_(RWSEM|COMPLETION)\((\w*)/\2/v/'		\
	--regex-c='/DECLARE_BITMAP\((\w*)/\1/v/'			\
	--regex-c='/(^|\s)(|L|H)LIST_HEAD\((\w*)/\3/v/'			\
	--regex-c='/(^|\s)RADIX_TREE\((\w*)/\2/v/'			\
	--regex-c='/DEFINE_PER_CPU\(([^,]*,\s*)(\w*).*\)/\2/v/'		\
	--regex-c='/DEFINE_PER_CPU_SHARED_ALIGNED\(([^,]*,\s*)(\w*).*\)/\2/v/' \
	--regex-c='/DECLARE_WAIT_QUEUE_HEAD\((\w*)/\1/v/'		\
	--regex-c='/DECLARE_(TASKLET|WORK|DELAYED_WORK)\((\w*)/\2/v/'	\
	--regex-c='/DEFINE_PCI_DEVICE_TABLE\((\w*)/\1/v/'		\
	--regex-c='/(^\s)OFFSET\((\w*)/\2/v/'				\
	--regex-c='/(^\s)DEFINE\((\w*)/\2/v/'				\
	--regex-c='/DEFINE_HASHTABLE\((\w*)/\1/v/'

	all_kconfigs | xargs $1 -a                              \
	--langdef=kconfig --language-force=kconfig              \
	--regex-kconfig='/^[[:blank:]]*(menu|)config[[:blank:]]+([[:alnum:]_]+)/\2/'

	all_kconfigs | xargs $1 -a                              \
	--langdef=kconfig --language-force=kconfig              \
	--regex-kconfig='/^[[:blank:]]*(menu|)config[[:blank:]]+([[:alnum:]_]+)/CONFIG_\2/'

	all_defconfigs | xargs -r $1 -a                         \
	--langdef=dotconfig --language-force=dotconfig          \
	--regex-dotconfig='/^#?[[:blank:]]*(CONFIG_[[:alnum:]_]+)/\1/'
}

emacs()
{
	all_target_sources | xargs $1 -a                        \
	--regex='/^\(ENTRY\|_GLOBAL\)(\([^)]*\)).*/\2/'         \
	--regex='/^SYSCALL_DEFINE[0-9]?(\([^,)]*\).*/sys_\1/'   \
	--regex='/^COMPAT_SYSCALL_DEFINE[0-9]?(\([^,)]*\).*/compat_sys_\1/' \
	--regex='/^TRACE_EVENT(\([^,)]*\).*/trace_\1/'		\
	--regex='/^TRACE_EVENT(\([^,)]*\).*/trace_\1_rcuidle/'	\
	--regex='/^DEFINE_EVENT([^,)]*, *\([^,)]*\).*/trace_\1/' \
	--regex='/^DEFINE_EVENT([^,)]*, *\([^,)]*\).*/trace_\1_rcuidle/' \
	--regex='/PAGEFLAG(\([^,)]*\).*/Page\1/'			\
	--regex='/PAGEFLAG(\([^,)]*\).*/SetPage\1/'		\
	--regex='/PAGEFLAG(\([^,)]*\).*/ClearPage\1/'		\
	--regex='/TESTSETFLAG(\([^,)]*\).*/TestSetPage\1/'	\
	--regex='/TESTPAGEFLAG(\([^,)]*\).*/Page\1/'		\
	--regex='/SETPAGEFLAG(\([^,)]*\).*/SetPage\1/'		\
	--regex='/__SETPAGEFLAG(\([^,)]*\).*/__SetPage\1/'	\
	--regex='/TESTCLEARFLAG(\([^,)]*\).*/TestClearPage\1/'	\
	--regex='/__TESTCLEARFLAG(\([^,)]*\).*/TestClearPage\1/'	\
	--regex='/CLEARPAGEFLAG(\([^,)]*\).*/ClearPage\1/'	\
	--regex='/__CLEARPAGEFLAG(\([^,)]*\).*/__ClearPage\1/'	\
	--regex='/__PAGEFLAG(\([^,)]*\).*/__SetPage\1/'		\
	--regex='/__PAGEFLAG(\([^,)]*\).*/__ClearPage\1/'	\
	--regex='/PAGEFLAG_FALSE(\([^,)]*\).*/Page\1/'		\
	--regex='/TESTSCFLAG(\([^,)]*\).*/TestSetPage\1/'	\
	--regex='/TESTSCFLAG(\([^,)]*\).*/TestClearPage\1/'	\
	--regex='/SETPAGEFLAG_NOOP(\([^,)]*\).*/SetPage\1/'	\
	--regex='/CLEARPAGEFLAG_NOOP(\([^,)]*\).*/ClearPage\1/'	\
	--regex='/__CLEARPAGEFLAG_NOOP(\([^,)]*\).*/__ClearPage\1/' \
	--regex='/TESTCLEARFLAG_FALSE(\([^,)]*\).*/TestClearPage\1/' \
	--regex='/__TESTCLEARFLAG_FALSE(\([^,)]*\).*/__TestClearPage\1/' \
	--regex='/TASK_PFA_TEST\([^,]*,\s*([^)]*)\)/task_\1/'		\
	--regex='/TASK_PFA_SET\([^,]*,\s*([^)]*)\)/task_set_\1/'	\
	--regex='/TASK_PFA_CLEAR\([^,]*,\s*([^)]*)\)/task_clear_\1/'	\
	--regex='/_PE(\([^,)]*\).*/PEVENT_ERRNO__\1/'		\
	--regex='/PCI_OP_READ(\([a-z]*[a-z]\).*[1-4])/pci_bus_read_config_\1/' \
	--regex='/PCI_OP_WRITE(\([a-z]*[a-z]\).*[1-4])/pci_bus_write_config_\1/'\
	--regex='/[^#]*DEFINE_HASHTABLE(\([^,)]*\)/\1/'

	all_kconfigs | xargs $1 -a                              \
	--regex='/^[ \t]*\(\(menu\)*config\)[ \t]+\([a-zA-Z0-9_]+\)/\3/'

	all_kconfigs | xargs $1 -a                              \
	--regex='/^[ \t]*\(\(menu\)*config\)[ \t]+\([a-zA-Z0-9_]+\)/CONFIG_\3/'

	all_defconfigs | xargs -r $1 -a                         \
	--regex='/^#?[ \t]?\(CONFIG_[a-zA-Z0-9_]+\)/\1/'
}

xtags()
{
	if $1 --version 2>&1 | grep -iq exuberant; then
		exuberant $1
	elif $1 --version 2>&1 | grep -iq emacs; then
		emacs $1
	else
		all_target_sources | xargs $1 -a
	fi
}


# Used for debugging
if [ "$TAGS_VERBOSE" = "1" ]; then
	set -x
fi

# Set project base directory
if [ "${ZEPHYR_BASE}" = "" ]; then
	echo "error: run zephyr-env.sh before $0"
	exit 1
fi
tree="${ZEPHYR_BASE}/"

# List of always explored directories
COMMON_DIRS="drivers dts include kernel lib misc subsys"

# Detect if ARCH is set. If not, we look for all archs
if [ "${ARCH}" = "" ]; then
	ALLSOURCES="${COMMON_DIRS} arch boards soc"
else
	ALLSOURCES="${COMMON_DIRS} arch/${ARCH} boards/${ARCH} soc/${ARCH}"
fi

# TODO: detect if BOARD is set so we can select certain files


# Perform main action
remove_structs=
case "$1" in
	"cscope")
		docscope
		;;

	"gtags")
		dogtags
		;;

	"tags")
		rm -f tags
		xtags ctags
		remove_structs=y
		;;

	"TAGS")
		rm -f TAGS
		xtags etags
		remove_structs=y
		;;
	*)
		echo "error: incorrect parameter"
		;;
esac

# Remove structure forward declarations.
if [ -n "$remove_structs" ]; then
    LANG=C sed -i -e '/^\([a-zA-Z_][a-zA-Z0-9_]*\)\t.*\t\/\^struct \1;.*\$\/;"\tx$/d' $1
fi
