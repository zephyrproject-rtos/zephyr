# Copyright (c) 2024, Witekio
# SPDX-License-Identifier: Apache-2.0

# Generate a root CA private key
openssl ecparam \
    -name prime256v1 \
    -genkey \
    -out ca_privkey.pem

# Generate a root CA certificate using private key
openssl req \
    -new \
    -x509 \
    -days 36500 \
    -key ca_privkey.pem \
    -out ca_cert.pem \
    -subj "/O=Zephyrproject/CN=Zephyrproject Sample Development CA"
