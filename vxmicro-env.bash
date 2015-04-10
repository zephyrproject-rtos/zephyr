
if [ "X$(basename -- "$0")" == "Xvxmicro-env.bash" ]; then
    echo "Source this file (do NOT execute it!) to set the VxMicro environment."
    exit
fi

# You can further customize your environment by creating a bash script called
# vxmicro-env_install.bash in your home directory. It will be automatically
# run (if it exists) by this script.

# identify OS source tree root directory
export VXMICRO_BASE=$(pwd)

# prepend VxMicro build system tools to PATH, if not already present
vxmicro_linux_bin=${VXMICRO_BASE}/host/x86-linux2/bin
echo "${PATH}" | grep -q "${vxmicro_linux_bin}"
[ $? != 0 ] && export PATH=${vxmicro_linux_bin}:${PATH}
unset vxmicro_linux_bin

# enable custom environment settings
vxmicro_answer_file=~/vxmicro-env_install.bash
[ -f ${vxmicro_answer_file} ] && . ${vxmicro_answer_file}
unset vxmicro_answer_file

# examples of custom settings to place in vxmicro-env_install.bash
#
# 1) create useful aliases
# alias GCC='export VXMICRO_TOOL=gcc'
# alias ICC='export VXMICRO_TOOL=icc'
# alias VIP='export | grep VXMICRO'
#
# 2) specify toolchain location
# declare -x VXMICRO_GCC_DIR="<path>/CodeSourcery/Sourcery_CodeBench_Lite_for_IA32_ELF"
# declare -x VXMICRO_ICC_DIR="<path>/icc/composer_xe_2013.1.117"
