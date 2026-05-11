# Copyright (c) 2024, Witekio
# SPDX-FileCopyrightText: © 2025-2026 Tenstorrent USA, Inc.
# SPDX-License-Identifier: Apache-2.0

OPENSSL=${1:-openssl}

# Generate a root CA private key
$OPENSSL ecparam \
    -name prime256v1 \
    -genkey \
    -out ca_privkey.pem

# Generate a root CA certificate using private key
$OPENSSL req \
    -new \
    -x509 \
    -days 36500 \
    -key ca_privkey.pem \
    -out ca_cert.pem \
    -subj "/O=Zephyrproject/CN=Zephyrproject Sample Development CA"
