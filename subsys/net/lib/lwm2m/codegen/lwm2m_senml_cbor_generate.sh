# Copyright (c) 2022 GARDENA GmbH
#
# SPDX-License-Identifier: Apache-2.0
set -e

cd $(dirname -- "$0")

command -v zcbor >/dev/null 2>&1 || { echo >&2 "Please install zcbor python package: https://github.com/zephyrproject-rtos/zcbor"; exit 1; }
command -v comby >/dev/null 2>&1 || { echo >&2 "Please install comby: https://comby.dev"; exit 1; }
command -v clang-format >/dev/null 2>&1 || { echo >&2 "Please install clang-format"; exit 1; }

mkdir -p gen && cd gen

zcbor code --default-max-qty 99 -c ../lwm2m_senml_cbor.cddl -e -d -t lwm2m_senml \
	--oc lwm2m_senml_cbor.c --oh lwm2m_senml_cbor.h

FILES=$(ls -1)
for file in $FILES; do
    ex "$file" <<EOF
1 insert
/*
 * Copyright (c) $(date +"%Y") Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
.
xit
EOF
done

comby -config ../lwm2m_senml_cbor_comby.conf -in-place
clang-format -i $FILES
cp $FILES ../../generated

echo "✅ Code has been generated - review changes & commit"
