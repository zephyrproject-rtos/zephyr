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

function get_ostype {
    case "$OSTYPE" in
	darwin*) echo "Darwin" ;;
	linux*) echo "Linux" ;;
	msys*) echo "Windows_NT" ;;
	cygwin*) echo "Windows_NT" ;;
	*) echo "unknown" ;;
    esac
}
readonly get_ostype

readonly PLATFORM_HAL=modules/hal/atmosic/ATM33xx-5
readonly PLATFORM_HAL_LIB=openair/modules/hal_atmosic/ATM33xx-5
readonly SPE=openair/samples/spe
readonly MCUBOOT=bootloader/mcuboot/boot/zephyr
readonly ATMISP=modules/hal/atmosic_lib/tools/atm_arch/bin/$(get_ostype)/atm_isp
readonly CMAKE_OBJCOPY=${ZEPHYR_SDK_INSTALL_DIR}/arm-zephyr-eabi/bin/arm-zephyr-eabi-objcopy

################################################################################

function cmake_def {
    printf -- "-D%s=$1\n" ${@:2}
}
readonly cmake_def

################################################################################


function usage {
    cat <<EOF

Environment variables:
  ZEPHYR_SDK_INSTALL_DIR: The directory of Zephyr SDK.
  ZEPHYR_TOOLCHAIN_VARIANT: The toolchain and the default setting is Zephyr.
  DEBUG_LOG: Set if enable log, 0: disable, 1: enabled, 2: by application (default), 3: by application(debug).
  PM_SETTING: Set if enable power management, 0: disable, 1: enabled, 2: by application (default).

usage: ${0##*/} [+-ha APP] [+-bcdefgijl FLAV] [+-mnp BPTH] [+-r BDRT] [+-s SER] [+-uw FLAV] [--] BOARD

  BOARD    Atmosic board (passed as -b to west build)

Options:
  -h       Help (this message)
  -a APP   Application path relative to west_topdir
  -b       Build only (skip flashing)
  -c       Enable secure debug (not applicable when -n is set)
  -d       Build/flash not just the app but the dependencies and generating atm file as well
           (MCUboot/SPE/ATMWSTK)
  -e       Erase flash
  -f       Flash only
  -g       Enable fast load
  -j       J-Link
  -l FLAV  Use statically linked controller library flavor (mutually exclusive with -w)
  -m       Merge SPE/NSPE (only with -n)
  -n       No MCUboot
  -p BPTH  Build path to existing build directory, (only applicable with -f)
  -r BDRT  BOARD_ROOT path to find out-of-tree board definitions
           (ignored if -f is set)
  -s SER   Device serial
  -u       DFU in Flash (not applicable when -n is set)
  -w FLAV  Use fixed ATMWSTK flavor (not applicable when -b is set, mutually exclusive with -l)
  -x OPT   Extra CMake option for the SPE; can be specified multiple times
  -J JVAL  Parallel build -- value for -j option passed to "west build -o=-j<JVAL>"
EOF
}

# Ensure these options are not set from the environment
unset APP BUILD_ONLY ATM_SECURE_DEBUG DEPENDENCIES ERASE_FLASH FLASH_ONLY FAST_LOAD GEN_ARCH JLINK ATMWSTKLIB MERGE_SPE_NSPE NO_MCUBOOT BPTH BOARD_ROOT FTDI_SERIAL JLINK_SERIAL DFU_IN_FLASH ATMWSTK X_SPE_OPTS PARALLEL_BUILD_JVAL
declare -a X_SPE_OPTS

while getopts :ha:bcdefgjl:mnp:r:s:uw:x:J: OPT; do
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
	c|+c)
	    ATM_SECURE_DEBUG=1
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
	g|+g)
	    FAST_LOAD=1
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
	p|+p)
	    BPTH="$OPTARG"
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
	x|+x)
	    X_SPE_OPTS+=("$OPTARG")
	    ;;
	J|+J)
	    PARALLEL_BUILD_JVAL="$OPTARG"
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

[[ -n $ZEPHYR_TOOLCHAIN_VARIANT ]] || export ZEPHYR_TOOLCHAIN_VARIANT=zephyr

readonly SETTING_DISABLED=0
readonly SETTING_ENABLED=1
readonly SETTING_DEFAULT=2
readonly SETTING_DEBUG=3
[[ -n $DEBUG_LOG ]] || export DEBUG_LOG=$SETTING_DEFAULT
[[ -n $PM_SETTING ]] || export PM_SETTING=$SETTING_DEFAULT

echo '--------------------------------------------'
echo 'ZEPHYR_SDK_INSTALL_DIR:   ' $ZEPHYR_SDK_INSTALL_DIR
echo 'ZEPHYR_TOOLCHAIN_VARIANT: ' $ZEPHYR_TOOLCHAIN_VARIANT
echo 'DEBUG_LOG:  ' $DEBUG_LOG
echo 'PM_SETTING: ' $PM_SETTING
echo '--------------------------------------------'

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
DTS_BL_EXTRAS=""

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

is_unset ATM_SECURE_DEBUG || {
    [[ -z $NO_MCUBOOT ]] || die 'USE_ATM_SECURE_DEBUG and NO_MCUBOOT cannot both be set'
    BL_EXTRAS="$BL_EXTRAS -DCONFIG_ATM_MCUBOOT_SECURE_DEBUG=y"
    DTS_BL_EXTRAS="$DTS_BL_EXTRAS;-DUSE_ATM_SECURE_DEBUG"
}

is_set DEPENDENCIES && [[ -n $APP ]] && [[ -z $FLASH_ONLY ]] && {
    GEN_ARCH=1
    [[ -e $ATMISP ]] || die "$ATMISP not exist"
    [[ -z $ATMWSTK ]] || {
	[[ -e $CMAKE_OBJCOPY ]] || die "$CMAKE_OBJCOPY not exist"
    }
    WARCH=(west atm_arch -atm_isp_path $ATMISP -o ${BOARD}_arch.atm)
}

if [ $BUILD -ne 0 ]
then
    WBUILD=(west build -p)
    if [[ -n $BOARD_ROOT ]]
    then
	CMAKE_DEF_BOARD_ROOT="$(cmake_def $BOARD_ROOT BOARD_ROOT)"
	WBUILD_CMAKE_OPTS+=("$CMAKE_DEF_BOARD_ROOT")
    fi
    if [ $DEBUG_LOG -eq $SETTING_ENABLED ]
    then WBUILD_CMAKE_OPTS+=($(cmake_def y ${CMAKE_CONF_LOG[@]}))
    elif [ $DEBUG_LOG -eq $SETTING_DISABLED ]
    then WBUILD_CMAKE_OPTS+=($(cmake_def n ${CMAKE_CONF_LOG[@]}))
    elif [ $DEBUG_LOG -eq $SETTING_DEBUG ]
    then NS_EXTRAS="$NS_EXTRAS -DFILE_SUFFIX=debug"
    fi
    if [ $PM_SETTING -eq $SETTING_DISABLED ]
    then WBUILD_CMAKE_OPTS+=($(cmake_def n CONFIG_PM))
    elif [ $PM_SETTING -eq $SETTING_ENABLED ]
    then WBUILD_CMAKE_OPTS+=($(cmake_def y CONFIG_PM))
    fi
    is_unset PARALLEL_BUILD_JVAL || WBUILD+=(-o=-j$PARALLEL_BUILD_JVAL)
    if is_unset NO_MCUBOOT
    then
	if is_set DEPENDENCIES
	then
	    ${WBUILD[@]} -s $MCUBOOT -b $BOARD@mcuboot -d build/$BOARD/$MCUBOOT -- $CMAKE_DEF_BOARD_ROOT -DCONFIG_BOOT_SIGNATURE_TYPE_ECDSA_P256=y -DCONFIG_BOOT_ECDSA_MICRO_ECC=y -DCONFIG_BOOT_SHA2_ATM=y -DCONFIG_DEBUG=n ${BL_EXTRAS} -DDTC_OVERLAY_FILE="${BOARD_ROOT:-$WEST_TOPDIR/zephyr}/boards/atmosic/atm33evk/${BOARD}_mcuboot_bl.overlay" -DDTS_EXTRA_CPPFLAGS=${DTS_EXTRAS}${DTS_BL_EXTRAS}
	    [[ $GEN_ARCH -eq 1 ]] && WARCH+=(--mcuboot_file build/$BOARD/$MCUBOOT/zephyr/zephyr.bin)
	    ${WBUILD[@]} -s $SPE -b ${BOARD}@mcuboot -d build/${BOARD}/$SPE -- $CMAKE_DEF_BOARD_ROOT \
			 -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_GENERATE_UNSIGNED_IMAGE=n \
			 -DDTS_EXTRA_CPPFLAGS=${DTS_EXTRAS} \
			 ${X_SPE_OPTS[@]}\

	fi
	[[ -z $APP ]] || \
	    ${WBUILD[@]} -s $APP -b ${BOARD}@mcuboot//ns -d build/${BOARD}_ns/$APP -- $CMAKE_DEF_BOARD_ROOT \
			 -DCONFIG_BOOTLOADER_MCUBOOT=y -DCONFIG_MCUBOOT_SIGNATURE_KEY_FILE=\"bootloader/mcuboot/root-ec-p256.pem\" \
			 ${NS_EXTRAS} \
			 -DDTS_EXTRA_CPPFLAGS=${DTS_EXTRAS} -DCONFIG_SPE_PATH=\"${WEST_TOPDIR}/build/$BOARD/$SPE\" \
			 ${WBUILD_CMAKE_OPTS[@]}
	    [[ $GEN_ARCH -eq 1 ]] && WARCH+=(--app_file build/${BOARD}_ns/$APP/zephyr/zephyr.signed.bin)
	    [[ $GEN_ARCH -eq 1 ]] && WARCH+=(-p build/${BOARD}_ns/$APP/zephyr/partition_info.map.merge)
    else
	is_unset DEPENDENCIES || ${WBUILD[@]} -s $SPE -b $BOARD -d build/$BOARD/$SPE -- $CMAKE_DEF_BOARD_ROOT -DDTS_EXTRA_CPPFLAGS=${DTS_EXTRAS} ${X_SPE_OPTS[@]}
	WBUILD_APP=(${WBUILD[@]} -s $APP -b ${BOARD}//ns -d build/${BOARD}_ns/$APP -- -DCONFIG_SPE_PATH=\"${WEST_TOPDIR}/build/$BOARD/$SPE\" ${NS_EXTRAS} \
	    -DDTS_EXTRA_CPPFLAGS=${DTS_EXTRAS} ${WBUILD_CMAKE_OPTS[@]})
	if [[ -z $APP ]]
	then echo 'No application to build'
	elif is_set MERGE_SPE_NSPE
	then
	    ${WBUILD_APP[@]} -DCONFIG_MERGE_SPE_NSPE=y
	    [[ $GEN_ARCH -eq 1 ]] && WARCH+=(--app_file build/${BOARD}_ns/$APP/zephyr/zephyr.bin)
	else
	    ${WBUILD_APP[@]}
	    [[ $GEN_ARCH -eq 1 ]] && WARCH+=(--spe_file build/$BOARD/$SPE/zephyr/zephyr.bin)
	    [[ $GEN_ARCH -eq 1 ]] && WARCH+=(--app_file build/${BOARD}_ns/$APP/zephyr/zephyr.bin)
	fi
	[[ $GEN_ARCH -eq 1 ]] && WARCH+=(-p build/${BOARD}_ns/$APP/zephyr/partition_info.map.merge)
    fi
    if [[ $GEN_ARCH -eq 1 ]]
    then
	if [[ -n $ATMWSTK ]]
	then
	    ${CMAKE_OBJCOPY} -O binary $PLATFORM_HAL_LIB/drivers/ble/atmwstk_${ATMWSTK}.elf $PLATFORM_HAL_LIB/drivers/ble/atmwstk_${ATMWSTK}.bin
	    WARCH+=(--atmwstk_file $PLATFORM_HAL_LIB/drivers/ble/atmwstk_${ATMWSTK}.bin)
	fi
	${WARCH[@]}
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
is_unset FAST_LOAD || WFLASH+=(--fast_load)
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

# 3) Program the tag data file
[[ -n $BPTH ]] || BPTH=build/${BOARD}_ns/$APP
SETTINGS_FILE="build/${BOARD}_ns/${APP}/zephyr/zephyr_settings.hex"
FACTORY_FILE="build/${BOARD}_ns/${APP}/zephyr/zephyr_factory.hex"
if [ -e "$SETTINGS_FILE" ]; then
    WFLASH_SETTINGS=(${WFLASH[@]} -d $BPTH --hex-file $SETTINGS_FILE)
    [[ -z $APP ]] || WFLASH_SETTINGS+=(--noreset)
    ${WFLASH_SETTINGS[@]}
fi
if [ -e "$FACTORY_FILE" ]; then
    WFLASH_FACTORY=(${WFLASH[@]} -d $BPTH --hex-file $FACTORY_FILE)
    [[ -z $APP ]] || WFLASH_FACTORY+=(--noreset)
    ${WFLASH_FACTORY[@]}
fi

# 4) Program the app
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

WFLASH_APP=(${WFLASH[@]} -d $BPTH)
if is_unset MERGE_SPE_NSPE
then ${WFLASH_APP[@]}
elif is_set ERASE_FLASH && [[ -z $BOOTLOADER ]] && [[ -z $ATMWSTK ]]
then ${WFLASH_APP[@]} --erase_flash
else ${WFLASH_APP[@]}
fi
