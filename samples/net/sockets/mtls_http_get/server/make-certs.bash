#!/usr/bin/env bash

# Make a CA public cert and private key
openssl req 		\
  -new 			\
  -x509 		\
  -nodes 		\
  -days 365 		\
  -subj '/CN=my-ca' 	\
  -keyout ca.key 	\
  -out ca.crt

echo "subjectKeyIdentifier=hash" > exts.ext
echo "authorityKeyIdentifier=keyid,issuer" >> exts.ext

# Generate server private key and public cert signed by the CA
openssl genrsa -out server.key 2048

openssl req 		\
  -new 			\
  -key server.key	\
  -subj '/CN=localhost'	\
  -out server.csr

openssl x509		\
  -req			\
  -in server.csr	\
  -CA ca.crt		\
  -CAkey ca.key		\
  -CAcreateserial	\
  -days 365		\
  -extfile exts.ext	\
  -out server.crt

rm server.csr

# Generate client private key, and public cert signed by the CA
openssl genrsa -out client.key 2048

openssl req		\
  -new			\
  -key client.key	\
  -subj '/CN=client'	\
  -out client.csr	\

openssl x509		\
  -req			\
  -in client.csr	\
  -CA ca.crt		\
  -CAkey ca.key		\
  -CAcreateserial	\
  -days 365		\
  -extfile exts.ext	\
  -out client.crt

rm client.csr
rm exts.ext
