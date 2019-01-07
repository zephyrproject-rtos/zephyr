#
# Copyright (c) 2019 HES-SO Valais.
#
# Zephyr-License-Identifier: Apache-2.0
#

# Ensure that the file is sourced either with "." or "source".
if test "$_" != "."; and test "$_" != "source"
	echo "Source this file (do NOT execute it!) to set the Zephyr Kernel environment."
	exit
end

# Identify OS source tree root directory.
set -x ZEPHYR_BASE (cd (dirname (status -f)); and pwd)

# Add Zephyr scripts to PATH.
set scripts_path $ZEPHYR_BASE/scripts
if ! echo "$PATH" | grep -q "$scripts_path"
    set -x PATH $scripts_path:$PATH
end
set -e scripts_path

# Enable custom environment settings.
set zephyr_answer_file ~/zephyr-env_install.bash
if test -e $zephyr_answer_file
	echo "Warning: Please rename ~/zephyr-env_install.bash to ~/.zephyrrc";
	. $zephyr_answer_file
end
set -e zephyr_answer_file

set zephyr_answer_file ~/.zephyrrc
if test -e $zephyr_answer_file
	. $zephyr_answer_file;
end
set -e zephyr_answer_file
