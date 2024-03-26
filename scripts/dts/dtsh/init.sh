# Copyright (c) 2023 Christophe Dufaza <chris@openmarl.org>
#
# SPDX-License-Identifier: Apache-2.0
#!/usr/bin/env sh

# Paths: $ZEPHYR_BASE/scripts/dts/dtsh/.venv
zephyr_base=$(realpath ../../..)
venv_root=$(realpath ./.venv)

if [ -n "$1" ]; then
	py_cmd="$1"
else
	py_cmd='python'
fi

echo `$py_cmd --version`
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

$py_cmd -m venv $venv_root

. $venv_root/bin/activate
pip install --upgrade pip setuptools

# AFAIK, easiest (less painfull) IDE/Emacs integration.
pip install --editable .
pip install -r requirements-dev.txt
pip install -r requirements-lsp.txt

# Install edtlib (aka python-devicetree), also in developer mode.
cd ../python-devicetree
pip install --editable .
cd ../dtsh
