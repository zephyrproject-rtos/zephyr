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

if [ "X$(basename -z -- "$0")" "==" "Xzephyr-env.sh" ]; then
    echo "Source this file (do NOT execute it!) to set the Zephyr Kernel environment."
    exit
fi

# You can further customize your environment by creating a bash script called
# zephyr-env_install.bash in your home directory. It will be automatically
# run (if it exists) by this script.

uname | grep -q MINGW && MINGW_OPT="-W"

# identify OS source tree root directory
export ZEPHYR_BASE=$( builtin cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd ${MINGW_OPT})

scripts_path=${ZEPHYR_BASE}/scripts
echo "${PATH}" | grep -q "${scripts_path}"
[ $? != 0 ] && export PATH=${PATH}:${scripts_path}
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
