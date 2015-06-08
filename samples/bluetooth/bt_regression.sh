#!/bin/bash

# Import common sanity check definitions
#
if [ -z ${TIMO_BASE} ]; then
	echo "shell variables required to build Zephyr OS are not set"
	exit 1
fi
if [ ! -d ${TIMO_BASE} ] ; then
	echo "directory ${TIMO_BASE} not found"
	exit 1
fi

source ${TIMO_BASE}/scripts/sanity_chk/common.defs

# Location of master project directory
#
PRJ_PATH=${TIMO_BASE}/samples

PRJ_LIST="\
nanokernel/apps/bluetooth/init               <n> pentium4        \n\
nanokernel/apps/bluetooth/init               <n> quark           \n\
#  nanokernel/apps/bluetooth/init               <n> atom            \n\
nanokernel/apps/bluetooth/init               <n> minuteia        \n\
nanokernel/apps/bluetooth/init               <n> fsl_frdm_k64f   \n\
nanokernel/apps/bluetooth/init               <n> ti_lm3s6965     \n\
nanokernel/apps/bluetooth/shell              <n> pentium4        \n\
nanokernel/apps/bluetooth/shell              <n> ti_lm3s6965     \n\
nanokernel/apps/bluetooth/shell              <n> fsl_frdm_k64f   \n\
nanokernel/test/test_bluetooth              <nq> pentium4!       \n\
nanokernel/test/test_bluetooth              <nq> quark           \n\
#  nanokernel/test/test_bluetooth              <nq> atom            \n\
nanokernel/test/test_bluetooth              <nq> minuteia!       \n\
nanokernel/test/test_bluetooth              <nq> ti_lm3s6965!    \n\
nanokernel/test/test_bluetooth              <nq> fsl_frdm_k64f   \n\
bluetooth/peripheral                         <u> pentium4        \n\
bluetooth/peripheral                         <u> ti_lm3s6965     \n\
microkernel/test/test_bluetooth             <uq> pentium4!       \n\
microkernel/test/test_bluetooth             <uq> quark           \n\
#  microkernel/test/test_bluetooth             <uq> atom           \n\
microkernel/test/test_bluetooth             <uq> minuteia!       \n\
microkernel/test/test_bluetooth             <uq> ti_lm3s6965!    \n\
microkernel/test/test_bluetooth             <uq> fsl_frdm_k64f   "

main() {
	# set up default behaviors
	build_only=0
	BSP_NAME=""
	ARCH_NAME=""
	KEEP_LOGS=0

	# set up environment info used to build projects
	build_info_set

	# set up project info used to build projects
	proj_info_set

	# build (and optionally execute) projects
	PRJ_CLASS="regression"
	let cur_proj=0
	let build_proj=0
	let run_proj=0
	while [ ${cur_proj} -lt ${NUM_PROJ} ] ; do
		let cur_proj++
		print_header

		# build project
		build_project ${cur_proj}
		let build_proj++

		# skip non-executable projects (or when doing "build only")
		if [ ${build_only} = 1 -o x${BSP_FLAG[${cur_proj}]} = x ] ; then
			continue
		fi

		# execute project
		qemu_project ${cur_proj}
		let run_proj++
	done

	${ECHO}
        print_header
	${ECHO} "${SCRIPT_NAME} completed successfully "\
	        "(built ${build_proj} projects and ran ${run_proj} of them)"
}

main $*

# success exit
exit 0
