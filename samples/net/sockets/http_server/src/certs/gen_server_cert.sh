# Copyright (c) 2024, Witekio
# SPDX-License-Identifier: Apache-2.0

# Generate a server private key
openssl ecparam \
    -name prime256v1 \
    -genkey \
    -out server_privkey.pem

# Generate a certificate signing request using server key
openssl req \
    -new \
    -sha256 \
    -key server_privkey.pem \
    -out server_csr.pem \
    -subj "/O=Zephyrproject/CN=zephyr"

# Create a file containing server CSR extensions
echo "subjectKeyIdentifier=hash" > server_csr.ext
echo "authorityKeyIdentifier=keyid,issuer" >> server_csr.ext
echo "basicConstraints=critical,CA:FALSE" >> server_csr.ext
echo "keyUsage=critical,digitalSignature" >> server_csr.ext
echo "extendedKeyUsage=serverAuth" >> server_csr.ext
echo "subjectAltName=DNS:zephyr.local,IP.1:192.0.2.1,IP.2:2001:db8::1" >> server_csr.ext

# Create a server certificate by signing the server CSR using the CA cert/key
openssl x509 \
    -req \
    -sha256 \
    -CA ca_cert.pem \
    -CAkey ca_privkey.pem \
    -days 36500 \
    -CAcreateserial \
    -CAserial ca.srl \
    -in server_csr.pem \
    -out server_cert.pem \
    -extfile server_csr.ext

# Create DER encoded versions of server certificate and private key
openssl ec \
    -outform der \
    -in server_privkey.pem \
    -out server_privkey.der

openssl x509 \
    -outform der \
    -in server_cert.pem \
    -out server_cert.der
