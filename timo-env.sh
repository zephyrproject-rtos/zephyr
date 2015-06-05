
if [ "X$(basename -- "$0")" == "Xtimo-env.bash" ]; then
    echo "Source this file (do NOT execute it!) to set the Zephyr OS environment."
    exit
fi

# You can further customize your environment by creating a bash script called
# timo-env_install.bash in your home directory. It will be automatically
# run (if it exists) by this script.

# identify OS source tree root directory
export TIMO_BASE=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

scripts_path=${TIMO_BASE}/scripts
echo "${PATH}" | grep -q "${scripts_path}"
[ $? != 0 ] && export PATH=${scripts_path}:${PATH}
unset scripts_path

# enable custom environment settings
timo_answer_file=~/timo-env_install.bash
[ -f ${timo_answer_file} ] && . ${timo_answer_file}
unset timo_answer_file
