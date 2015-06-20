
if [ "X$(basename -- "$0")" == "Xzephyr-env.sh" ]; then
    echo "Source this file (do NOT execute it!) to set the Zephyr Kernel environment."
    exit
fi

# You can further customize your environment by creating a bash script called
# zephyr-env_install.bash in your home directory. It will be automatically
# run (if it exists) by this script.

# identify OS source tree root directory
export ZEPHYR_BASE=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

scripts_path=${ZEPHYR_BASE}/scripts
echo "${PATH}" | grep -q "${scripts_path}"
[ $? != 0 ] && export PATH=${scripts_path}:${PATH}
unset scripts_path

# enable custom environment settings
zephyr_answer_file=~/zephyr-env_install.bash
[ -f ${zephyr_answer_file} ] && . ${zephyr_answer_file}
unset zephyr_answer_file
