#!/usr/bin/env bash
#
# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
#
# Examples how to create the certificate files if necessary.

set -e

CURVE=prime256v1

echo "=== Generating EC CA key ==="
openssl ecparam -name $CURVE -genkey -noout -out ca.key

echo "=== Generating CA certificate ==="
openssl req -x509 -new \
	-key ca.key \
	-sha256 \
	-days 3650 \
	-out ca.crt \
	-subj "/CN=Test-EC-CA"

echo "=== Generating server EC key ==="
openssl ecparam -name $CURVE -genkey -noout -out server.key

echo "=== Generating server CSR ==="
openssl req -new \
	-key server.key \
	-out server.csr \
	-subj "/CN=localhost"

cat > server.ext <<EOF
basicConstraints=CA:FALSE
keyUsage = digitalSignature
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
IP.1 = 127.0.0.1
EOF

echo "=== Signing server certificate ==="
openssl x509 -req \
	-in server.csr \
	-CA ca.crt \
	-CAkey ca.key \
	-CAcreateserial \
	-out server.crt \
	-days 3650 \
	-sha256 \
	-extfile server.ext

echo "=== Generating client EC key ==="
openssl ecparam -name $CURVE -genkey -noout -out client.key

echo "=== Generating client CSR ==="
openssl req -new \
	-key client.key \
	-out client.csr \
	-subj "/CN=test-client"

cat > client.ext <<EOF
basicConstraints=CA:FALSE
keyUsage = digitalSignature
extendedKeyUsage = clientAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = test-client
EOF

echo "=== Signing client certificate ==="
openssl x509 -req \
	-in client.csr \
	-CA ca.crt \
	-CAkey ca.key \
	-CAcreateserial \
	-out client.crt \
	-days 3650 \
	-sha256 \
	-extfile client.ext

echo "=== Generating intermediate CA EC key ==="
openssl ecparam -name $CURVE -genkey -noout -out intermediate.key

echo "=== Generating intermediate CA CSR ==="
openssl req -new \
	-key intermediate.key \
	-out intermediate.csr \
	-subj "/CN=Test-EC-Intermediate-CA"

cat > intermediate.ext <<EOF
basicConstraints=critical,CA:TRUE,pathlen:0
keyUsage = critical, keyCertSign, cRLSign
EOF

echo "=== Signing intermediate CA certificate ==="
openssl x509 -req \
	-in intermediate.csr \
	-CA ca.crt \
	-CAkey ca.key \
	-CAcreateserial \
	-out intermediate.crt \
	-days 3650 \
	-sha256 \
	-extfile intermediate.ext

echo "=== Generating chain server EC key ==="
openssl ecparam -name $CURVE -genkey -noout -out chain_server.key

echo "=== Generating chain server CSR ==="
openssl req -new \
	-key chain_server.key \
	-out chain_server.csr \
	-subj "/CN=localhost"

cp server.ext chain_server.ext

echo "=== Signing chain server certificate with the intermediate CA ==="
openssl x509 -req \
	-in chain_server.csr \
	-CA intermediate.crt \
	-CAkey intermediate.key \
	-CAcreateserial \
	-out chain_server.crt \
	-days 3650 \
	-sha256 \
	-extfile chain_server.ext

echo "=== Verifying certificates ==="
openssl verify -CAfile ca.crt server.crt
openssl verify -CAfile ca.crt client.crt
openssl verify -CAfile ca.crt -untrusted intermediate.crt chain_server.crt

echo ""
echo "Certificates generated:"
echo "CA:      ca.crt"
echo "Server:  server.crt / server.key"
echo "Client:  client.crt / client.key"
echo "Intermediate CA: intermediate.crt / intermediate.key"
echo "Server signed by the intermediate CA: chain_server.crt / chain_server.key"
