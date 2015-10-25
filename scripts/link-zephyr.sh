#!/bin/sh
#
# link the kernel
#
# The kernel is linked from the objects selected by $(KBUILD_ZEPHYR_MAIN).
# Most are built-in.o files from top-level directories
# in the kernel tree, others are specified in arch/$(ARCH)/Makefile.
#
# microkernel/nanokernel
#   ^
#   +--< $(KBUILD_ZEPHYR_MAIN)
#        +--< drivers/built-in.o + more

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
	LIBS=""
	for tcl in ${ALL_LIBS}; do  LIBS="${LIBS} -l${tcl}"; done
	echo "${LDFLAGS_zephyr}" > ${1}
	echo "-Map ./${2}" >> ${1}
	echo "-L ${objtree}/include/generated" >> ${1}
	echo "-u _OffsetAbsSyms -u _ConfigAbsSyms" >> ${1}
	echo "-e __start" >> ${1}
	if [ -n "${CONFIG_LINK_WHOLE_ARCHIVE}" -a -n "${KBUILD_ZEPHYR_APP}" ]; then
		echo "--whole-archive ${KBUILD_ZEPHYR_APP} --no-whole-archive" >> ${1}
	fi
	echo "--start-group ${KBUILD_ZEPHYR_MAIN} ${KBUILD_ZEPHYR_APP}" >> ${1}
	echo "${objtree}/arch/${SRCARCH}/core/offsets/offsets.o" >> ${1}
	echo "--end-group" >> ${1}
	echo "${LIB_INCLUDE_DIR} ${LIBS}" >> ${1}
}

#Creates linker command file
# {1} output file
# {2} optional additional link parameters
linker_command()
{
	${CC} -x assembler-with-cpp -nostdinc -undef -E -P \
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
	${LD}  -T ${3} @${2} -o ${1}
}

#Generates IDT and merge them into final binary
# ${1} input file (${KERNEL_NAME}.elf)
# ${2} output file (staticIdt.o)
# ${3} output file (int_vector_alloc)
# ${4} output file (irq_int_vector_map)
gen_idt()
{
	test -z $OUTPUT_FORMAT && OUTPUT_FORMAT=elf32-i386
	test -z $OUTPUT_ARCH && OUTPUT_ARCH=i386
	${OBJCOPY} -I $OUTPUT_FORMAT  -O binary -j intList ${1} isrList.bin
	${GENIDT} -i isrList.bin -n ${CONFIG_IDT_NUM_VECTORS:-256} \
              -o staticIdt.bin -b ${3}.bin -m ${4}.bin
	${OBJCOPY} -I binary -B $OUTPUT_ARCH -O $OUTPUT_FORMAT \
               --rename-section .data=staticIdt staticIdt.bin ${2}
	${OBJCOPY} -I binary -B $OUTPUT_ARCH -O $OUTPUT_FORMAT \
               --rename-section .data=${3} ${3}.bin ${3}.o
	${OBJCOPY} -I binary -B $OUTPUT_ARCH -O $OUTPUT_FORMAT \
               --rename-section .data=${4} ${4}.bin ${4}.o
	rm -f staticIdt.bin
	rm -f ${3}.bin
	rm -f ${4}.bin
	rm -f isrList.bin
}

# Linking the kernel
# ${1} - linker params file (${KERNEL_NAME}.lnk)
# ${2} - linker command file (final-linker.cmd)
# ${3} - input file (staticIdt.o)
# ${4} - output file
# ${5} - additional input file if applicable
# ${6} - additional input file if applicable
zephyr_link()
{
	${LD} -T ${2} @${1} ${3} ${5} ${6} -o ${4}
	${OBJCOPY} --set-section-flags intList=noload ${4} elf.tmp
	${OBJCOPY} -R intList elf.tmp ${4}
	rm elf.tmp
}

zephyr_bin_strip()
{
	${OBJDUMP} -S ${1} >${2}
	${OBJCOPY} -S -O binary -R .note -R .comment -R COMMON -R .eh_frame ${1} ${3}
	${STRIP} -s -o ${4} ${1}
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
	rm -f .tmp_version
	rm -f .tmp_*
	rm -f System.map
	rm -f *.lnk
	rm -f *.map
	rm -f *.elf
	rm -f *.lst
	rm -f *.bin
	rm -f *.strip
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

if [ "${SRCARCH}" = "x86" ]; then
	info SIDT ${KERNEL_NAME}.elf
	gen_idt ${KERNEL_NAME}.elf staticIdt.o int_vector_alloc irq_int_vector_map
	linker_command final-linker.cmd -DFINAL_LINK
	zephyr_link ${KERNEL_NAME}.lnk final-linker.cmd staticIdt.o \
	            ${KERNEL_NAME}.elf int_vector_alloc.o irq_int_vector_map.o
fi

info BIN ${KERNEL_NAME}.bin
zephyr_bin_strip ${KERNEL_NAME}.elf ${KERNEL_NAME}.lst ${KERNEL_NAME}.bin ${KERNEL_NAME}.strip

if [ -n "${CONFIG_BUILDTIME_EXTABLE_SORT}" ]; then
	info SORTEX ${KERNEL_NAME}
	sortextable ${KERNEL_NAME}
fi

info SYSMAP System.map
mksysmap ${KERNEL_NAME}.elf System.map

# We made a new kernel - delete old version file
rm -f .old_version
