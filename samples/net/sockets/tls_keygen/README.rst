.. zephyr:code-sample:: tls-keygen
   :name: TLS on-device key generation
   :relevant-api: tls_credentials tls_credentials_keygen secure_sockets_options

   Generate a TLS device key and a CSR on the device, and use the key for a TLS connection.

Overview
********

This sample shows how a device gets a client certificate for a private key that never leaves the
device. It uses the :ref:`TLS credentials on-device key generation <sockets_tls_credentials_subsys>`
API.

The sample does the following:

#. Generates an ECDSA P-256 key for the security tag :kconfig:option:`CONFIG_NET_SAMPLE_SEC_TAG`,
   if the security tag has no key yet. The key is persistent, so a restart uses the same key.
#. Prints the public key and a Certificate Signing Request (CSR) in PEM format.
#. Waits until the device certificate and the CA certificate are added for the same security tag
   with the :ref:`TLS credentials shell <tls_credentials_shell>`.
#. Connects to a TLS server that requires a client certificate, sends a message and prints the
   response.

A certificate authority (CA) signs the CSR. In this sample, the CA and the server run on the host
with OpenSSL.

Requirements
************

- :zephyr:board:`native_sim`. The sample uses the
  :ref:`offloaded sockets <nsim_per_offloaded_sockets>` of the host, so it can connect to a server
  on the host without other network setup.
- OpenSSL on the host.

Building and Running
********************

Build the sample:

.. zephyr-app-commands::
   :zephyr-app: samples/net/sockets/tls_keygen
   :board: native_sim
   :goals: build
   :compact:

Create the CA and the server certificate on the host. The common name of the server certificate
must match :kconfig:option:`CONFIG_NET_SAMPLE_SERVER_HOSTNAME`:

.. code-block:: console

   openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-256 -nodes \
           -keyout ca.key -out ca.pem -subj "/CN=Test CA" -days 30
   openssl req -newkey ec -pkeyopt ec_paramgen_curve:P-256 -nodes \
           -keyout server.key -out server.csr -subj "/CN=localhost"
   openssl x509 -req -in server.csr -CA ca.pem -CAkey ca.key -out server.pem -days 30

Run the sample. The shell is on the standard input and output:

.. code-block:: console

   ./build/zephyr/zephyr.exe -uart_stdinout

The sample prints the CSR:

.. code-block:: console

   <inf> tls_keygen_sample: Generating a key for sec tag 42
   -----BEGIN PUBLIC KEY-----
   ...
   -----END PUBLIC KEY-----
   -----BEGIN CERTIFICATE REQUEST-----
   ...
   -----END CERTIFICATE REQUEST-----
   <inf> tls_keygen_sample: Waiting for the certificates. Add them in the shell:
   <inf> tls_keygen_sample:   cred add 42 CLIENT_CERT DEFAULT BIN <base64 DER>
   <inf> tls_keygen_sample:   cred add 42 CA_CERT DEFAULT BIN <base64 DER>

Copy the CSR, including the ``BEGIN`` and ``END`` lines, to :file:`device.csr` on the host, and
sign it with the CA:

.. code-block:: console

   openssl x509 -req -in device.csr -CA ca.pem -CAkey ca.key -out device.pem -days 30

Start the TLS server on the host. ``-Verify 1`` requires a client certificate, and ``-rev`` sends
back each received line in reverse order:

.. code-block:: console

   openssl s_server -accept 4433 -cert server.pem -key server.key -CAfile ca.pem -Verify 1 -rev

Print the device certificate and the CA certificate as Base64-encoded DER:

.. code-block:: console

   openssl x509 -in device.pem -outform der | openssl base64 -A
   openssl x509 -in ca.pem -outform der | openssl base64 -A

Add them in the shell of the sample:

.. code-block:: console

   uart:~$ cred add 42 CLIENT_CERT DEFAULT BIN <device certificate>
   uart:~$ cred add 42 CA_CERT DEFAULT BIN <CA certificate>

The shell reports the device certificate as type ``SERVER_CERT``. This is expected:
``CLIENT_CERT`` and ``SERVER_CERT`` are the same credential type,
:c:enumerator:`TLS_CREDENTIAL_PUBLIC_CERTIFICATE`.

The sample then connects to the server:

.. code-block:: console

   <inf> tls_keygen_sample: Connecting to 127.0.0.1:4433
   <inf> tls_keygen_sample: TLS handshake done
   <inf> tls_keygen_sample: Received: ryhpeZ morf olleH
   <inf> tls_keygen_sample: Done

The server prints ``Peer certificate: CN = zephyr-device`` and ``Verification: OK``.

The key and the certificates are kept in :file:`flash.bin` in the current directory, so the next
run connects without new certificates. To start again with a new key, delete :file:`flash.bin`.
