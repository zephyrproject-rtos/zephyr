#
# Copyright (c) 2015 Wind River Systems, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# In zsh the value of $0 depends on the FUNCTION_ARGZERO option which is
# set by default. FUNCTION_ARGZERO, when it is set, sets $0 temporarily
# to the name of the function/script when executing a shell function or
# sourcing a script. POSIX_ARGZERO option, when it is set, exposes the
# original value of $0 in spite of the current FUNCTION_ARGZERO setting.
#
# Note: The version of zsh need to be 5.0.6 or above. Any versions below
# 5.0.6 maybe encoutner errors when sourcing this script.
if [ -n "$ZSH_VERSION" ]; then
	DIR="${(%):-%N}"
	if [ $options[posixargzero] != "on" ]; then
		setopt posixargzero
		NAME=$(basename -- "$0")
		setopt posixargzero
	else
		NAME=$(basename -- "$0")
	fi
else
	DIR="${BASH_SOURCE[0]}"
	NAME=$(basename -- "$0")
fi

if [ "X$NAME" "==" "Xzephyr-env.sh" ]; then
    echo "Source this file (do NOT execute it!) to set the Zephyr Kernel environment."
    exit
fi

# You can further customize your environment by creating a bash script called
# zephyr-env_install.bash in your home directory. It will be automatically
# run (if it exists) by this script.

uname | grep -q MINGW && MINGW_OPT="-W"

# identify OS source tree root directory
export ZEPHYR_BASE=$( builtin cd "$( dirname "$DIR" )" && pwd ${MINGW_OPT})

scripts_path=${ZEPHYR_BASE}/scripts
scripts_path=$(echo "/$scripts_path" | sed 's/\\/\//g' | sed 's/://')
echo "${PATH}" | grep -q "${scripts_path}"
[ $? != 0 ] && export PATH=${scripts_path}:${PATH}
unset scripts_path

# enable custom environment settings
zephyr_answer_file=~/zephyr-env_install.bash
[ -f ${zephyr_answer_file} ] && {
	echo "Warning: Please rename ~/zephyr-env_install.bash to ~/.zephyrrc";
	. ${zephyr_answer_file};
}
unset zephyr_answer_file
zephyr_answer_file=~/.zephyrrc
[ -f ${zephyr_answer_file} ] &&  . ${zephyr_answer_file};
unset zephyr_answer_file
