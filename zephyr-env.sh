#
# Copyright (c) 2015 Wind River Systems, Inc.
#
# SPDX-License-Identifier: Apache-2.0
#

# In zsh the value of $0 depends on the FUNCTION_ARGZERO option which is
# set by default. FUNCTION_ARGZERO, when it is set, sets $0 temporarily
# to the name of the function/script when executing a shell function or
# sourcing a script. POSIX_ARGZERO option, when it is set, exposes the
# original value of $0 in spite of the current FUNCTION_ARGZERO setting.
#
# Note: The version of zsh need to be 5.0.6 or above. Any versions below
# 5.0.6 maybe encounter errors when sourcing this script.
if [ -n "${ZSH_VERSION:-}" ]; then
	dir="${(%):-%N}"
	if [ $options[posixargzero] != "on" ]; then
		setopt posixargzero
		name=$(basename -- "$0")
		unsetopt posixargzero
	else
		name=$(basename -- "$0")
	fi
else
	dir="${BASH_SOURCE[0]}"
	name=$(basename -- "$0")
fi

if [ "X$name" "==" "Xzephyr-env.sh" ]; then
    echo "Source this file (do NOT execute it!) to set the Zephyr Kernel environment."
    exit
fi

# You can further customize your environment by creating a bash script called
# .zephyrrc in your home directory. It will be automatically
# run (if it exists) by this script.

if uname | grep -q "MINGW"; then
    win_build=1
    pwd_opt="-W"
else
    win_build=0
    pwd_opt=""
fi

# identify OS source tree root directory
export ZEPHYR_BASE=$( builtin cd "$( dirname "$dir" )" > /dev/null && pwd ${pwd_opt})
unset pwd_opt

scripts_path=${ZEPHYR_BASE}/scripts
if [ "$win_build" -eq 1 ]; then
    scripts_path=$(echo "/$scripts_path" | sed 's/\\/\//g' | sed 's/://')
fi
unset win_build
if ! echo "${PATH}" | grep -q "${scripts_path}"; then
    export PATH=${scripts_path}:${PATH}
fi
unset scripts_path

# enable custom environment settings
zephyr_answer_file=~/zephyr-env_install.bash
[ -f ${zephyr_answer_file} ] && {
	echo "Warning: Please rename ~/zephyr-env_install.bash to ~/.zephyrrc"
	. ${zephyr_answer_file}
}
unset zephyr_answer_file
xdg_config_home="${XDG_CONFIG_HOME:-${HOME}/.config}"
zephyr_xdg_rc="${xdg_config_home}/zephyr/zephyrrc"
if [ -f "${zephyr_xdg_rc}" ]; then
    . "${zephyr_xdg_rc}"
else
    zephyr_answer_file=~/.zephyrrc
    [ -f "${zephyr_answer_file}" ] && . "${zephyr_answer_file}"
    unset zephyr_answer_file
fi
unset zephyr_xdg_rc
unset xdg_config_home
