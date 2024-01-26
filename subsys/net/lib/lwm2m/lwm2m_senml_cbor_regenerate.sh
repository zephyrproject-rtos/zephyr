# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

zcbor code --default-max-qty 99 -c lwm2m_senml_cbor.cddl -e -d -t lwm2m_senml \
	--oc lwm2m_senml_cbor.c --oh lwm2m_senml_cbor.h --file-header "
Copyright (c) 2023 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0
"

git add -A
git commit -s -m"pre-patch"

git apply --reject lwm2m_senml_cbor.patch

clang-format -i \
	lwm2m_senml_cbor_decode.c lwm2m_senml_cbor_decode.h \
	lwm2m_senml_cbor_encode.c lwm2m_senml_cbor_encode.h \
	lwm2m_senml_cbor_types.h
