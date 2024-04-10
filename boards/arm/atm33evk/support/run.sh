#! /usr/bin/env bash

# Copyright (c) 2024 Atmosic
#
# SPDX-License-Identifier: Apache-2.0

################################################################################
# Provides an abstraction over west for programming an atm33 board.  It both
# builds and flashes an application, which is its common use case, but it can
# also build the "dependencies" for an application -- MCUboot/SPE -- and flash
# them as well as ATMWSTK if needed.  The idea is to save the user some headache
# from having to construct and deal with lengthy west build and flash commands
# and to prevent mistakes such as forgetting a build/flash step or doing them
# out of order, or passing --noreset/--erase_flash at the wrong time.
################################################################################

set -e
set -o pipefail

function die {
    >&2 echo $@
    exit 1
}
readonly die

function is_unset {
    eval "[[ -z \$$1 ]] || [[ \$$1 -eq 0 ]]"
}
readonly is_unset

function is_set {
    eval "[[ -n \$$1 ]] && [[ \$$1 -ne 0 ]]"
}
readonly is_set

function is_undef {
    eval "[[ -z \$$1 ]]"
}
readonly is_undef

readonly PLATFORM_HAL=modules/hal/atmosic/ATM33xx-5
readonly PLATFORM_HAL_LIB=modules/hal/atmosic_lib/ATM33xx-5
readonly SPE=$PLATFORM_HAL/examples/spe
readonly MCUBOOT=bootloader/mcuboot/boot/zephyr

################################################################################

function cmake_def {
    printf -- "-D%s=$1\n" ${@:2}
}
readonly cmake_def

################################################################################


function usage {
    cat <<EOF
usage: ${0##*/} [+-ha APP] [+-bdefjl FLAV] [+-mns SER] [+-uw FLAV] [--] BOARD

  BOARD    Atmosic board (passed as -b to west build)

Options:
  -h       Help (this message)
  -a APP   Application path relative to west_topdir
  -b       Build only (skip flashing)
  -d       Build/flash not just the app but the dependencies
           (MCUboot/SPE/ATMWSTK)
  -e       Erase flash
  -f       Flash only
  -j       J-Link
  -l FLAV  Use statically linked controller library flavor (mutually exclusive with -w)
  -m       Merge SPE/NSPE (only with -n)
  -n       No MCUboot
  -r BDRT  BOARD_ROOT path to find out-of-tree board definitions
           (ignored if -f is set)
  -s SER   Device serial
  -u       DFU in Flash (not applicable when -n is set)
  -w FLAV  Use fixed ATMWSTK flavor (not applicable when -b is set, mutually exclusive with -l)
EOF
}

while getopts :ha:bdefjlmnr:s:uw: OPT; do
    case $OPT in
	h|+h)
	    usage
	    exit 0
	    ;;
	a|+a)
	    APP="$OPTARG"
	    ;;
	b|+b)
	    BUILD_ONLY=1
	    ;;
	d|+d)
	    DEPENDENCIES=1
	    ;;
	e|+e)
	    ERASE_FLASH=1
	    ;;
	f|+f)
	    FLASH_ONLY=1
	    ;;
	j|+j)
	    JLINK=1
	    ;;
	l|+l)
	    ATMWSTKLIB="$OPTARG"
	    ;;
	m|+m)
	    MERGE_SPE_NSPE=1
	    ;;
	n|+n)
	    NO_MCUBOOT=1
	    ;;
	r|+r)
	    if [[ $OPTARG == /* ]]
	    then BOARD_ROOT=$OPTARG
	    else BOARD_ROOT=$PWD/$OPTARG
	    fi
	    ;;
	s|+s)
	    if [[ -z $JLINK ]]
	    then FTDI_SERIAL="$OPTARG"
	    else JLINK_SERIAL="$OPTARG"
	    fi
	    ;;
	u|+u)
	    DFU_IN_FLASH=1
	    ;;
	w|+w)
	    ATMWSTK="$OPTARG"
	    ;;
	*)
	    >&2 usage
	    exit 2
    esac
done
shift $(( OPTIND - 1 ))
OPTIND=1

if [[ $# -gt 0 ]]
then BOARD=$1; shift
fi

################################################################################

[[ -n $BOARD ]] || die 'No board specified'

BUILD=1
FLASH=1
is_unset FLASH_ONLY || BUILD=0
is_unset BUILD_ONLY || FLASH=0

[ $FLASH -eq 0 ] || [[ -n $JLINK_SERIAL ]] || [[ -n $FTDI_SERIAL ]] || \
    die 'Neither JLINK_SERIAL nor FTDI_SERIAL set'

################################################################################

[[ -n $ZEPHYR_SDK_INSTALL_DIR ]] || die 'ZEPHYR_SDK_INSTALL_DIR not set'
[[ -n $ZEPHYR_TOOLCHAIN_VARIANT ]] || export ZEPHYR_TOOLCHAIN_VARIANT=zephyr

if [[ -n $WEST_TOPDIR ]]
then cd $WEST_TOPDIR
elif [[ -z $OS ]]
then WEST_TOPDIR=$PWD
elif [[ $OS == Windows_NT ]]
then WEST_TOPDIR=`cygpath -w $PWD`; WEST_TOPDIR=${WEST_TOPDIR//\\//}
else die "Unknown OS '$OS'"
fi

[[ -z $BOARD_ROOT ]] || [[ -e $BOARD_ROOT ]] || \
    die "BOARD_ROOT '$BOARD_ROOT' does not exist."

is_unset DEPENDENCIES_ONLY || {
    DEPENDENCIES=1
    APP=
}

is_unset ATMWSTK_ONLY || {
    [[ -n $ATMWSTK ]] || die 'No ATMWSTK specified in ATMWSTK_ONLY mode'
    BUILD=0
    FLASH=1
    DEPENDENCIES=1
    APP=
}

is_set DEPENDENCIES || [[ -n $APP ]] || \
    die 'Have neither any dependencies nor an app to build or program.'

readonly -a CMAKE_CONF_LOG=(
    CONFIG_LOG
    CONFIG_LOG_MODE_IMMEDIATE
)

DTS_EXTRAS=""

is_undef ATMWSTKLIB || {
    [[ -z $ATMWSTK ]] || die 'ATMWSTK and ATMWSTKLIB cannot both be set'
    NS_EXTRAS="$NS_EXTRAS -DCONFIG_USE_ATMWSTK=n -DCONFIG_ATMWSTKLIB=\"$ATMWSTKLIB\""
}

is_undef ATMWSTK || {
    NS_EXTRAS="$NS_EXTRAS -DCONFIG_USE_ATMWSTK=y -DCONFIG_ATMWSTK=\"$ATMWSTK\""
    DTS_EXTRAS="$DTS_EXTRAS;-DATMWSTK=$ATMWSTK;"
}

is_unset DFU_IN_FLASH || {
    [[ -z $NO_MCUBOOT ]] || die 'DFU_IN_FLASH and NO_MCUBOOT cannot both be set'
    DTS_EXTRAS="$DTS_EXTRAS;-DDFU_IN_FLASH"
}

if [ $BUILD -ne 0 ]
then
    WBUILD=(west build -p)
    if [[ -n $BOARD_ROOT ]]
    then
	CMAKE_DEF_BOARD_ROOT="$(cmake_def $BOARD_ROOT BOARD_ROOT)"
	WBUILD_CMAKE_OPTS+=("$CMAKE_DEF_BOARD_ROOT")
    fi
    if is_set DEBUG_LOG
    then WBUILD_CMAKE_OPTS+=($(cmake_def y ${CMAKE_CONF_LOG[@]}))
    else WBUILD_CMAKE_OPTS+=($(cmake_def n ${CMAKE_CONF_LOG[@]}))
    fi
    if is_set NO_PM
    then WBUILD_CMAKE_OPTS+=($(cmake_def n CONFIG_PM))
    else WBUILD_CMAKE_OPTS+=($(cmake_def y CONFIG_PM))
    fi
    is_unset PARALLEL_BUILD || WBUILD+=(-o=-j4)
    if is_unset NO_MCUBOOT
    then
	if is_set DEPENDENCIES
	then
	    ${WBUILD[@]} -s $MCUBOOT -b $BOARD@mcuboot -d build/$BOARD/$MCUBOOT -- $CMAKE_DEF_BOARD_ROOT -DCONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y -DCONFIG_DEBUG=n -DDTC_OVERLAY_FILE="${BOARD_ROOT:-$WEST_TOPDIR/zephyr}/boards/arm/atm33evk/${BOARD}_mcuboot_bl.overlay" -DDTS_EXTRA_CPPFLAGS=${DTS_EXTRAS}
	    ${WBUILD[@]} -s $SPE -b ${BOARD}@mcuboot -d build/${BOARD}/$SPE -- $CMAKE_DEF_BOARD_ROOT \
			 -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE=n \
			 -DDTS_EXTRA_CPPFLAGS=${DTS_EXTRAS}
	fi
	[[ -z $APP ]] || \
	    ${WBUILD[@]} -s $APP -b ${BOARD}_ns@mcuboot -d build/${BOARD}_ns/$APP -- $CMAKE_DEF_BOARD_ROOT \
			 -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\" \
			 ${NS_EXTRAS} \
			 -DDTS_EXTRA_CPPFLAGS=${DTS_EXTRAS} -DCONFIG_SPE_PATH=\"${WEST_TOPDIR}/build/$BOARD/$SPE\" \
			 ${WBUILD_CMAKE_OPTS[@]}
    else
	is_unset DEPENDENCIES || ${WBUILD[@]} -s $SPE -b $BOARD -d build/$BOARD/$SPE -- $CMAKE_DEF_BOARD_ROOT -DDTS_EXTRA_CPPFLAGS=${DTS_EXTRAS}
	WBUILD_APP=(${WBUILD[@]} -s $APP -b ${BOARD}_ns -d build/${BOARD}_ns/$APP -- -DCONFIG_SPE_PATH=\"${WEST_TOPDIR}/build/$BOARD/$SPE\" ${NS_EXTRAS} \
	    -DDTS_EXTRA_CPPFLAGS=${DTS_EXTRAS} ${WBUILD_CMAKE_OPTS[@]})
	if [[ -z $APP ]]
	then echo 'No application to build'
	elif is_set MERGE_SPE_NSPE
	then ${WBUILD_APP[@]} -DCONFIG_MERGE_SPE_NSPE=y
	else ${WBUILD_APP[@]}
	fi
    fi
elif [ $FLASH -eq 0 ]
then die 'Nothing to build or flash'
fi

[ $FLASH -ne 0 ] || {
    echo 'Skipping flash'
    exit 0
}

WFLASH=(west flash)
is_set NO_SKIP_REBUILD || WFLASH+=(--skip-rebuild)
is_set NO_VERIFY || WFLASH+=(--verify)
if [[ -n $JLINK_SERIAL ]]
then WFLASH+=(--device=$JLINK_SERIAL --jlink)
else WFLASH+=(--device=$FTDI_SERIAL)
fi

if is_set DEPENDENCIES
then
    if is_set NO_BOOTLOADER
    then echo 'Skipping bootloader programming'
    elif [[ -n $BOOTLOADER ]]
    then echo "Using user-specified bootloader '$BOOTLOADER'"
    elif is_unset NO_MCUBOOT
    then BOOTLOADER=$MCUBOOT
    elif is_unset MERGE_SPE_NSPE
    then BOOTLOADER=$SPE
    elif [[ -n $APP ]]
    then echo "Programming a non-MCUboot app '$APP' with a merged SPE"
    fi
    # 1) Program the bootloader, if any
    if [[ -z $BOOTLOADER ]] && [[ -z $ATMWSTK ]]
    then die 'Neither a bootloader nor atmwstk dependency to program'
    fi
    if [[ -n $BOOTLOADER ]] && is_unset ATMWSTK_ONLY && is_unset MERGE_SPE_NSPE
    then
	WFLASH_BOOTLOADER=(${WFLASH[@]} -d build/$BOARD/$BOOTLOADER)
	is_unset ERASE_FLASH || WFLASH_BOOTLOADER+=(--erase_flash)
	if [[ -n $APP ]] || [[ -n $ATMWSTK ]]
	then WFLASH_BOOTLOADER+=(--noreset)
	fi
	${WFLASH_BOOTLOADER[@]}
    fi
    # 2) Program the atmwstk if any
    if [[ -z $ATMWSTK ]]
    then echo 'No ATMWSTK to program'
    elif [[ -z $BOOTLOADER ]] && is_unset MERGE_SPE_NSPE
    then die 'No bootloader specified for programming ATMWSTK (need its build-dir)'
    else
	WFLASH_ATMWSTK=(${WFLASH[@]})
	if [[ -n $BOOTLOADER ]]
	then WFLASH_ATMWSTK+=(-d build/$BOARD/$BOOTLOADER)
	elif is_set MERGE_SPE_NSPE
	then WFLASH_ATMWSTK+=(-d build/$BOARD/$SPE)
	fi
	WFLASH_ATMWSTK+=(--use-elf --elf-file $PLATFORM_HAL_LIB/drivers/ble/atmwstk_${ATMWSTK}.elf)
	[[ -z $APP ]] || WFLASH_ATMWSTK+=(--noreset)
	[[ -n $BOOTLOADER ]] || WFLASH_ATMWSTK+=(--erase_flash)
	${WFLASH_ATMWSTK[@]}
    fi
else echo 'Skipping dependency programming'
fi

# 3) Program the app
[[ -n $APP ]] || {
    is_set DEPENDENCIES || die 'Nothing to do'
    echo 'No app to program'
    exit 0
}
if is_unset DEPENDENCIES && is_set ERASE_FLASH && is_set MERGE_SPE_NSPE
then
    if [[ -z $ATMWSTK ]]
    then echo 'Warning: erasing flash without programming the dependencies'
    else die "Cannot erase flash without at least programming ATMWSTK=$LL"
    fi
fi

WFLASH_APP=(${WFLASH[@]} -d build/${BOARD}_ns/$APP)
if is_unset MERGE_SPE_NSPE
then ${WFLASH_APP[@]}
elif is_set ERASE_FLASH && [[ -z $BOOTLOADER ]] && [[ -z $ATMWSTK ]]
then ${WFLASH_APP[@]} --erase_flash
else ${WFLASH_APP[@]}
fi
