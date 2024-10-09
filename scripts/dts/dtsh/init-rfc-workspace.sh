#!/usr/bin/env sh
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2024 Christophe Dufaza <chris@openmarl.org>
#
# Initialize a new West workspace for hacking DTSh RFC:
#
#   workspace/
#   ├── .venv/      Dedicated Python virtual environment
#   ├── .west/      West workspace configuration
#   └── zephyr/     Manifest repository (Zephyr fork with DTSh RFC)
#

set -e

dtsh_dirname=$(dirname "$0")
dtsh_dir=$(realpath "$dtsh_dirname")

zephyr_base=$(realpath "$dtsh_dir/../../..")
workspace_dir=$(realpath "$zephyr_base/..")
west_venv="$workspace_dir/.venv"

print_usage() {
	echo 'init-rfc-workspace [-p|--python| PYTHON] [-b|--branch BRANCH] [-d|--dev]'
	echo 'PYTHON: Python runtime to use, e.g. python3.9, defaults to python'
	echo 'BRANCH: Initial RFC branch to checkout, default to rfc-dtsh-next'
}

while test $# -gt 0; do
	key=$1
	case $key in
	-h)
		print_usage
		exit 0
		;;
	-p | --python)
		arg_python=$2
		shift
		shift
		;;
	-b | --branch)
		arg_branch=$2
		shift
		shift
		;;
	-d | --dev)
		flag_dev=1
		shift
		;;
	*)
		echo "Unexpected argument: $key"
		exit 2
		;;
	esac
done

if [ -z "$arg_python" ]; then
	py_cmd=python
else
	py_cmd="$arg_python"
fi

if [ -z "$arg_branch" ]; then
	rfc_branch=rfc-dtsh-next
else
	rfc_branch="$arg_branch"
fi

continue_or_abort() {
	# -n is supported by both GNU coreutils and BSD versions of echo.
	# shellcheck disable=SC3037
	echo -n 'Continue [yN]: '
	read -r yes_no
	case "$yes_no" in
	y | Y) ;;
	*)
		echo 'Goodbye.'
		exit 0
		;;
	esac
}

# Initialize and activate Python virtual environment.
# init_venv PYTHON PATH
init_venv() {
	arg_python=$1
	arg_path=$2

	$arg_python -m venv "$arg_path"
	# shellcheck disable=SC1091
	. "$arg_path/bin/activate"
	pip install -U pip setuptools
	pip install -U west
}

echo "Workspace: $workspace_dir"
echo "ZEPHYR_BASE: $zephyr_base"
echo "Branch: $rfc_branch"
echo "Python: $py_cmd"
echo ".venv (West): $west_venv"
if [ -n "$flag_dev" ]; then
	lsp_venv="$dtsh_dir/.venv"
	echo ".venv (LSP): $lsp_venv"
else
	echo '.venv (LSP): None'
fi
continue_or_abort

if [ -d "$west_venv" ]; then
	echo "Python virual environment exists (West), will be removed !"
	continue_or_abort
	rm -r "$west_venv"
fi

if [ -d "$workspace_dir/.west" ]; then
	echo "West configuration exists, will be removed !"
	continue_or_abort
	rm -r "$workspace_dir/.west"
fi

if [ -d "$lsp_venv" ]; then
	echo "Python virual environment exists (LSP), will be removed !"
	continue_or_abort
	rm -r "$lsp_venv"
fi

echo
echo '* Initializing virtual environment'
init_venv "$py_cmd" "$west_venv"

echo
echo '* Initializing West workspace'
# see https://docs.zephyrproject.org/latest/develop/west/built-in.html#west-init
west init -l "$zephyr_base"
git -C "$zephyr_base" checkout "$rfc_branch"
cd "$workspace_dir"
west update

echo
echo '* Installing Zephyr Python requirements'
pip install -r "$zephyr_base/scripts/requirements.txt"

if [ -n "$flag_dev" ]; then
	echo
	echo '* Setup development environment'
	pip install -e "$zephyr_base/scripts/dts/dtsh"
	pip install "$zephyr_base/scripts/dts/python-devicetree"
	pip install -r "$dtsh_dir/requirements-dev.txt"
	pip install -r "$dtsh_dir/requirements-lsp.txt"
fi
