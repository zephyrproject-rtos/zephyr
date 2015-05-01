#!/bin/sh
#
# link tinymountain
#
# tinymountain is linked from the objects selected by $(KBUILD_TIMO_INIT) and
# $(KBUILD_TIMO_MAIN). Most are built-in.o files from top-level directories
# in the kernel tree, others are specified in arch/$(ARCH)/Makefile.
# Ordering when linking is important, and $(KBUILD_TIMO_INIT) must be first.
#
# tinymountain
#   ^
#   |
#   +-< $(KBUILD_TIMO_INIT)
#   |   +--< init/version.o + more
#   |
#   +--< $(KBUILD_TIMO_MAIN)
#   |    +--< drivers/built-in.o mm/built-in.o + more
#   |
#   +-< ${kallsymso} (see description in KALLSYMS section)
#
# tinymountain version (uname -v) cannot be updated during normal
# descending-into-subdirs phase since we do not yet know if we need to
# update tinymountain.
# Therefore this step is delayed until just before final link of tinymountain.
#
# System.map is generated to document addresses of all kernel symbols

# Error out on error
set -e

# Nice output in kbuild format
# Will be supressed by "make -s"
info()
{
	if [ "${quiet}" != "silent_" ]; then
		printf "  %-7s %s\n" ${1} ${2}
	fi
}

# Creates linker parameters.
# {1} output file
# {2} symbol map file
linker_params()
{
	echo "${LDFLAGS_tinymountain}" > ${1}
	echo "-Wl,-Map,./${2}" >> ${1}
	echo "-L ${objtree}/include/generated" >> ${1}
	echo "-u _OffsetAbsSyms -u _ConfigAbsSyms" >> ${1}
	echo "-Wl,--start-group ${KBUILD_TIMO_MAIN}" >> ${1}
	echo "${objtree}/include/generated/offsets.o" >> ${1}
}

#Creates linker command file
# {1} output file
# {2} optional additional link parameters
linker_command()
{
	${CC} -x assembler-with-cpp -nostdinc -undef -E -P  \
		${LDFLAG_LINKERCMD} \
		${LD_TOOLCHAIN} ${2} \
		-I${srctree}/include -I${objtree}/include/generated \
		${KBUILD_LDS} \
		-o ${1}
}

# Link of ${KERNEL_NAME}.o used for section mismatch analysis
# ${1} output file
# ${2} linker parameters file
# ${3} linker command file
initial_link()
{
	${CC} -o ${1} @${2}  -T ${3}
}

#Generates IDT and merge them into final binary
# ${1} input file (${KERNEL_NAME}.elf)
# ${2} output file (staticIdt.o)
gen_idt()
{
	${OBJCOPY} -I elf32-i386 -O binary -j intList ${1} isrList.bin
	${srctree}/host/x86-linux2/bin/genIdt -i isrList.bin -n 256 -o staticIdt.bin
	${OBJCOPY} -I binary -B i386 -O elf32-i386 --rename-section .data=staticIdt staticIdt.bin ${2}
	rm -f staticIdt.bin
	rm -f isrList.bin
}

# Link of tinymountain
# ${1} - linker params file (${KERNEL_NAME}.lnk)
# ${2} - linker command file (final-linker.cmd)
# ${3} - input file (staticIdt.o)
# ${4} - output file
tinymountain_link()
{
	${CC} -o ${4} @${1} ${3} -T ${2}
	${OBJCOPY} --set-section-flags intList=noload ${4} elf.tmp
	${OBJCOPY} -R intList elf.tmp ${4}
	rm elf.tmp
}


# Create ${2} .o file with all symbols from the ${1} object file
kallsyms()
{
	info KSYM ${2}
	local kallsymopt;

	if [ -n "${CONFIG_HAVE_UNDERSCORE_SYMBOL_PREFIX}" ]; then
		kallsymopt="${kallsymopt} --symbol-prefix=_"
	fi

	if [ -n "${CONFIG_KALLSYMS_ALL}" ]; then
		kallsymopt="${kallsymopt} --all-symbols"
	fi

	if [ -n "${CONFIG_ARM}" ] && [ -n "${CONFIG_PAGE_OFFSET}" ]; then
		kallsymopt="${kallsymopt} --page-offset=$CONFIG_PAGE_OFFSET"
	fi

	if [ -n "${CONFIG_X86_64}" ]; then
		kallsymopt="${kallsymopt} --absolute-percpu"
	fi

	local aflags="${KBUILD_AFLAGS} ${KBUILD_AFLAGS_KERNEL}               \
		      ${NOSTDINC_FLAGS} ${TIMOINCLUDE} ${KBUILD_CPPFLAGS}"

	${NM} -n ${1} | \
		scripts/kallsyms ${kallsymopt} | \
		${CC} ${aflags} -c -o ${2} -x assembler-with-cpp -
}

# Create map file with all symbols from ${1}
# See mksymap for additional details
mksysmap()
{
	${CONFIG_SHELL} "${srctree}/scripts/mksysmap" ${1} ${2}
}

sortextable()
{
	${objtree}/scripts/sortextable ${1}
}

# Delete output files in case of error
trap cleanup SIGHUP SIGINT SIGQUIT SIGTERM ERR
cleanup()
{
	rm -f .old_version
	rm -f .tmp_System.map
	rm -f .tmp_kallsyms*
	rm -f .tmp_version
	rm -f .tmp_${KERNEL_NAME}*
	rm -f System.map
	rm -f ${KERNEL_NAME}.lnk
	rm -f ${KERNEL_NAME}.map
	rm -f ${KERNEL_NAME}.elf
	rm -f staticIdt.o
	rm -f linker.cmd
	rm -f final-linker.cmd
}

#
#
# Use "make V=1" to debug this script
case "${KBUILD_VERBOSE}" in
*1*)
	set -x
	;;
esac

if [ "$1" = "clean" ]; then
	cleanup
	exit 0
fi

# We need access to CONFIG_ symbols
case "${KCONFIG_CONFIG}" in
*/*)
	. "${KCONFIG_CONFIG}"
	;;
*)
	# Force using a file from the current directory
	. "./${KCONFIG_CONFIG}"
esac

#link ${KERNEL_NAME}.o
info LD ${KERNEL_NAME}.elf
linker_params ${KERNEL_NAME}.lnk ${KERNEL_NAME}.map
linker_command linker.cmd
initial_link ${KERNEL_NAME}.elf ${KERNEL_NAME}.lnk linker.cmd

# Update version
info GEN .version
if [ ! -r .version ]; then
	rm -f .version;
	echo 1 >.version;
else
	mv .version .old_version;
	expr 0$(cat .old_version) + 1 >.version;
fi;

kallsymso=""
kallsyms_tinymountain=""
if [ -n "${CONFIG_KALLSYMS}" ]; then

	# kallsyms support
	# Generate section listing all symbols and add it into tinymountain
	# It's a three step process:
	# 1)  Link .tmp_${KERNEL_NAME} so it has all symbols and sections,
	#     but __kallsyms is empty.
	#     Running kallsyms on that gives us .tmp_kallsyms1.o with
	#     the right size
	# 2)  Link .tmp_${KERNEL_NAME}2 so it now has a __kallsyms section of
	#     the right size, but due to the added section, some
	#     addresses have shifted.
	#     From here, we generate a correct .tmp_kallsyms2.o
	# 2a) We may use an extra pass as this has been necessary to
	#     woraround some alignment related bugs.
	#     KALLSYMS_EXTRA_PASS=1 is used to trigger this.
	# 3)  The correct ${kallsymso} is linked into the final tinymountain.
	#
	# a)  Verify that the System.map from tinymountain matches the map from
	#     ${kallsymso}.

	kallsymso=.tmp_kallsyms2.o
	kallsyms_tinymountain=.tmp_${KERNEL_NAME}2

	# step 1
	tinymountain_link "" .tmp_${KERNEL_NAME}1
	kallsyms .tmp_${KERNEL_NAME}1 .tmp_kallsyms1.o

	# step 2
	tinymountain_link .tmp_kallsyms1.o .tmp_${KERNEL_NAME}2
	kallsyms .tmp_${KERNEL_NAME}2 .tmp_kallsyms2.o

	# step 2a
	if [ -n "${KALLSYMS_EXTRA_PASS}" ]; then
		kallsymso=.tmp_kallsyms3.o
		kallsyms_tinymountain=.tmp_${KERNEL_NAME}3

		tinymountain_link .tmp_kallsyms2.o .tmp_${KERNEL_NAME}3

		kallsyms .tmp_${KERNEL_NAME}3 .tmp_kallsyms3.o
	fi
fi

if [ "${SRCARCH}" = "x86" ]; then
	info SIDT ${KERNEL_NAME}.elf
	gen_idt ${KERNEL_NAME}.elf staticIdt.o
	linker_command final-linker.cmd -DFINAL_LINK
	tinymountain_link ${KERNEL_NAME}.lnk final-linker.cmd staticIdt.o ${KERNEL_NAME}.elf
fi

if [ -n "${CONFIG_BUILDTIME_EXTABLE_SORT}" ]; then
	info SORTEX ${KERNEL_NAME}
	sortextable ${KERNEL_NAME}
fi

info SYSMAP System.map
mksysmap ${KERNEL_NAME}.elf System.map

# step a (see comment above)
if [ -n "${CONFIG_KALLSYMS}" ]; then
	mksysmap ${kallsyms_tinymountain} .tmp_System.map

	if ! cmp -s System.map .tmp_System.map; then
		echo >&2 Inconsistent kallsyms data
		echo >&2 Try "make KALLSYMS_EXTRA_PASS=1" as a workaround
		cleanup
		exit 1
	fi
fi

# We made a new kernel - delete old version file
rm -f .old_version
