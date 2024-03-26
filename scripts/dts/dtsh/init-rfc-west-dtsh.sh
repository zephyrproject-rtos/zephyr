# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0
#!/usr/bin/env sh

west_workspace=$(realpath ../../../..)
zephyr_base="$west_workspace/zephyr"
venv_root="$west_workspace/.venv"

if [ -n "$1" ]; then
	py_cmd="$1"
else
	py_cmd='python'
fi

echo `$py_cmd --version`
echo West workspace: $west_workspace
echo ZEPHYR_BASE: $zephyr_base
echo Python venv: $venv_root

echo -n 'Continue [yN]: '
read yes_no
case "$yes_no" in
    y|Y)
    ;;
    *)
        echo "Goodbye."
        exit 0
        ;;
    esac

rm -rf $venv_root
rm -rf "$west_workspace/.west"

$py_cmd -m venv $venv_root
. $venv_root/bin/activate
pip install --upgrade pip setuptools

pip install -U west

west init -l "$zephyr_base"
west update

pip install -r "$zephyr_base/scripts/requirements.txt"

pip install "$zephyr_base/scripts/dts/python-devicetree"

pip install --editable "$zephyr_base/scripts/dts/dtsh"
