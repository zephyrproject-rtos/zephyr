.. _tls_credentials_shell:

TLS Credentials Shell
#####################

The TLS Credentials shell provides a command-line interface for managing installed TLS credentials.

Commands
********

.. _tls_credentials_shell_buf_cred:

Buffer Credential (``buf``)
===========================

Buffer data incrementally into the credential buffer so that it can be added using the :ref:`tls_credentials_shell_add_cred` command.

Alternatively, clear the credential buffer.

Usage
-----

To append ``<DATA>`` to the credential buffer, use:

.. code-block:: shell

   cred buf <DATA>

Use this as many times as needed to load the full credential into the credential buffer, then use the :ref:`tls_credentials_shell_add_cred` command to store it.

To clear the credential buffer, use:

.. code-block:: shell

   cred buf clear

Arguments
---------

.. csv-table::
   :header: "Argument", "Description"
   :widths: 15 85

   "``<DATA>``", "Text data to be appended to credential buffer. It can be either text, or base64-encoded binary. See :ref:`tls_credentials_shell_add_cred` and :ref:`tls_credentials_shell_data_formats` for details."

.. _tls_credentials_shell_add_cred:

Add Credential (``add``)
=========================

Add a TLS credential to the TLS Credential store.

Credential contents can be provided in-line with the call to ``cred add``, or will otherwise be sourced from the credential buffer.

Usage
-----

To add a TLS credential using the data from the credential buffer, use:

.. code-block:: shell

   cred add <SECTAG> <TYPE> <BACKEND> <FORMAT>

To add a TLS credential using data provided with the same command, use:

.. code-block:: shell

   cred add <SECTAG> <TYPE> <BACKEND> <FORMAT> <DATA>

Arguments
---------

.. csv-table::
   :header: "Argument", "Description"
   :widths: 15 85

   "``<SECTAG>``", "The sectag to use for the new credential. Can be any non-negative integer."
   "``<TYPE>``", "The type of credential to add. See :ref:`tls_credentials_shell_cred_types` for valid values."
   "``<BACKEND>``", "Reserved. Must always be ``DEFAULT`` (case-insensitive)."
   "``<FORMAT>``", "Specifies the storage format of the provided credential. See :ref:`tls_credentials_shell_data_formats` for valid values."
   "``<DATA>``", "If provided, this argument will be used as the credential data, instead of any data in the credential buffer. Can be either text, or base64-encoded binary."

Example
-------

For example, to add a credential to sectag ``1234``, of type ``CA``, using the default backend, and with the credential buffer in ``BINT`` format:

.. code-block:: shell

   cred add 1234 CA DEFAULT BINT

.. _tls_credentials_shell_del_cred:

Delete Credential (``del``)
===========================

Delete a specified credential from the credential store.

Usage
-----

To delete a credential matching a specified sectag and credential type (if it exists), use:

.. code-block:: shell

   cred del <SECTAG> <TYPE>

Arguments
---------

.. csv-table::
   :header: "Argument", "Description"
   :widths: 15 85

   "``<SECTAG>``", "The sectag of the credential to delete. Can be any non-negative integer."
   "``<TYPE>``", "The type of credential to delete. See :ref:`tls_credentials_shell_cred_types` for valid values."

Example
-------

For example, to delete the credential of type ``CA`` stored in sectag ``1234``:

.. code-block:: shell

   cred del 1234 CA

.. _tls_credentials_shell_get_cred:

Get Credential Contents (``get``)
=================================

Retrieve and print the contents of a specified credential.

Usage
-----

To retrieve and print a credential matching a specified sectag and credential type (if it exists), use:

.. code-block:: shell

   cred get <SECTAG> <TYPE> <FORMAT>

Arguments
---------

.. csv-table::
   :header: "Argument", "Description"
   :widths: 15 85

   "``<SECTAG>``", "The sectag of the credential to get. Can be any non-negative integer."
   "``<TYPE>``", "The type of credential to get. See :ref:`tls_credentials_shell_cred_types` for valid values."
   "``<BACKEND>``", "Reserved. Must always be ``DEFAULT`` (case-insensitive)."
   "``<FORMAT>``", "Specifies the retrieval format for the provided credential. See :ref:`tls_credentials_shell_data_formats` for valid values."

Example
-------

For example, to retrieve the credential of type ``CA`` stored in sectag ``1234``, assuming it was originally stored in ``BINT`` format:

.. code-block:: shell

   cred get 1234 CA BINT

Output
------

The contents of the selected credential will be output as a base-64-encoded :ref:`data block <tls_credentials_shell_data_blocks>`.

[TODO EXAMPLE]

.. _tls_credentials_shell_keygen:

Generate Private Key (``keygen``)
=================================

Description

Usage
-----

Arguments
---------

Example
-------

Output
------

.. _tls_credentials_shell_csr:

Generate Certificate Signing Request (``csr``)
==============================================

Generate a Certificate Signing Request (CSR) based on the specified private key sectag.

If supported, and the specified sectag has no private key, optionally generate a private key of the specified type at the specified sectag for this purpose.
In this case, the :ref:`storage format <tls_credentials_shell_data_formats>` needed to retrieve the stored private key will depend on the TLS Credentials backend in use.

Usage
-----

To generate a Certificate Signing Request based on an existing private key, use:

.. code-block:: shell

   cred csr <SECTAG> <BACKEND> <DNAME> EXISTING

To generate a Certificate Signing Request based on a brand new private key, use:
.. code-block:: shell

   cred csr <SECTAG> <BACKEND> <DNAME> <KEYGEN_TYPE>

Arguments
---------

.. csv-table::
   :header: "Argument", "Description"
   :widths: 15 85

   "``<SECTAG>``", "The sectag of the private key to use. Can be any non-negative integer."
   "``<BACKEND>``", "Reserved. Must always be ``DEFAULT`` (case-insensitive)."
   "``<DNAME>``", "The distinguished name to include in the CSR. Can be any RFC 4514 Distinguished Name string. For example: \"C=US,ST=CA,O=Linux Foundation,CN=Zephyr RTOS Device 1\""
   "``<KEYGEN_TYPE>``", "The type of key to generate. See :ref:`tls_credentials_shell_keygen_types` for valid values."

Output
------

The output will be a base-64-encoded :ref:`data block <tls_credentials_shell_data_blocks>` containing the generated CSR in ``PKCS#10`` / ``ASN.1`` format (see RFC 2986).

Examples
--------

For example, to generate a CSR based on the existing private key in ``1234``, with distinguished name :

.. code-block:: shell

   cred get 1234 CA BINT

For example:

[TODO EXAMPLE]

.. _tls_credentials_shell_list_cred:

List Credentials (``list``)
===========================

List TLS credentials in the credential store.

Usage
-----

To list all available credentials, use:

.. code-block:: shell

   cred list

To list all credentials with a specified sectag, use:

.. code-block:: shell

   cred list <SECTAG>

To list all credentials with a specified credential type, use:

.. code-block:: shell

   cred list any <TYPE>

To list all credentials with a specified credential type and sectag, use:

.. code-block:: shell

   cred list <SECTAG> <TYPE>


Arguments
---------

.. csv-table::
   :header: "Argument", "Description"
   :widths: 15 85

   "``<SECTAG>``", "Optional. If provided, only list credentials with this sectag. Pass ``any`` or omit to allow any sectag. Otherwise, can be any non-negative integer."
   "``<TYPE>``", "Optional. If provided, only list credentials with this credential type. Pass ``any`` or omit to allow any credential type. Otherwise, see :ref:`tls_credentials_shell_cred_types` for valid values."


Output
------

The command outputs all matching credentials in the following (CSV-compliant) format:

.. code-block:: shell

   <SECTAG>,<TYPE>,<DIGEST>,<STATUS>

Where:

.. csv-table::
   :header: "Symbol", "Value"
   :widths: 15 85

   "``<SECTAG>``", "The sectag of the listed credential. A non-negative integer."
   "``<TYPE>``", "Credential type short-code (see :ref:`tls_credentials_shell_cred_types` for details) of the listed credential."
   "``<DIGEST>``", "A string digest representing the credential contents. The exact nature of this digest may vary depending on credentials storage backend, but currently for all backends this is a base64 encoded SHA256 hash of the raw credential contents (so different storage formats for essentially identical credentials will have different digests)."
   "``<STATUS>``", "Status code indicating success or failure with generating a digest of the listed credential. 0 if successful, negative error code specific to the storage backend otherwise. Lines for which status is not zero will be printed with error formatting."

After the list is printed, a final summary of the found credentials will be printed in the form:

.. code-block:: shell

   <N> credentials found.

Where ``<N>`` is the number of credentials found, and is zero if none are found.

.. _tls_credentials_shell_cred_types:

Credential Types
****************

The following keywords (case-insensitive) may be used to specify a credential type:

.. csv-table::
   :header: "Keyword(s)", "Meaning"
   :widths: 15 85

   "``CA_CERT``, ``CA``", "A trusted CA certificate."
   "``SERVER_CERT``, ``SELF_CERT``, ``CLIENT_CERT``, ``CLIENT``, ``SELF``, ``SERV``", "Self or server certificate."
   "``PRIVATE_KEY``, ``PK``", "A private key."
   "``PRE_SHARED_KEY``, ``PSK``", "A pre-shared key."
   "``PRE_SHARED_KEY_ID``, ``PSK_ID``", "ID for pre-shared key."

.. _tls_credentials_shell_data_formats:

Storage/Retrieval Formats
*************************

The :ref:`tls_credentials <sockets_tls_credentials_subsys>` module treats stored credentials as arbitrary binary buffers.

For convenience, the TLS credentials shell offers four formats for providing and later retrieving these buffers using the shell.

These formats and their (case-insensitive) keywords are as follows:

.. csv-table::
   :header: "Keyword", "Meaning", "Behavior during storage (``cred add``)", "Behavior during retrieval (``cred get``)"
   :widths: 3, 32, 34, 34

   "``BIN``", "Credential is handled by shell as base64 and stored without NULL termination.", "Data entered into shell will be decoded from base64 into raw binary before storage. No terminator will be appended.", "Stored data will be encoded into base64 before being printed."
   "``BINT``", "Credential is handled by shell as base64 and stored with NULL termination.", "Data entered into shell will be decoded from base64 into raw binary and a NULL terminator will be appended before storage.", "NULL terminator will be truncated from stored data before said data is encoded into base64 and then printed."
   "``STR``", "Credential is handled by shell as literal string and stored without NULL termination.", "Text data entered into shell will be passed into storage as-written, without a NULL terminator.", "Stored data will be printed as text. Non-printable characters will be printed as ``?``"
   "``STRT``", "Credential is handled by shell as literal string and stored with NULL-termination.", "Text data entered into shell will be passed into storage as-written, with a NULL terminator.", "NULL terminator will be truncated from stored data before said data is printed as text. Non-printable characters will be printed as ``?``"

The ``BIN`` format can be used to install credentials of any type, since base64 can be used to encode any concievable binary buffer.
The remaining three formats are provided for convenience in special use-cases.

For example:

- To install printable pre-shared-keys, use ``STR`` to enter the PSK without first encoding it.
  This ensures it is stored without a NULL terminator.
- To install DER-formatted X.509 certificates (or other raw-binary credentials, such as non-printable PSKs) base64-encode the binary and use the ``BIN`` format.
- To install PEM-formatted X.509 certificates or certificate chains, base64 encode the full PEM string (including new-lines and ``----BEGIN X ----`` / ``----END X----`` markers), and then use the ``BINT`` format to make sure the stored string is NULL-terminated.
  This is required because Zephyr does not support multi-line strings in the shell.
  Otherwise, the ``STRT`` format could be used for this purpose without base64 encoding.
  It is possible to use ``BIN`` instead if you manually encode a NULL terminator into the base64.

.. _tls_credentials_shell_data_blocks:

Data Block Output
*****************

Large binary payloads are output in "data-blocks". These are base-64-encoded, fixed-character-width
blocks of printed text, representing an underlying binary buffer of some sort.

The represented binary buffer could be anything. For example:
- It could be raw ASN.1 binary (such as in the case of :ref:`CSR gen <tls_credentials_shell_csr>`).
- It could be plain text (still encoded as base-64).
- It could even be be ``PEM`` (a format which itself contains mostly base-64) re-encoded as base-64 (thus making the output base-64-encoded base64).

This is motivated by the desire for a consistent, machine-readable format for arbitrary binary payloads, whether or not the underlying payload may be itself machine or human-readable.

To improve machine readability on deferred serial backends, each line of the output block is always prefixed with a special start-line tag, and the completed block is concluded by an end-block tag on a new line.

Both of these tags, along with the width of the output blocks are customizable:
- The start-line tag defaults to ``#CSH: ``, and is customized by the :kconfig:option:`CONFIG_TLS_CREDENTIALS_SHELL_BLK_PREFIX` Kconfig option.
- The end-block tag defaults to ``#CSH-END``, and is customized by the :kconfig:option:`CONFIG_TLS_CREDENTIALS_SHELL_BLK_END` Kconfig option.
- The block character width is customized by the :kconfig:option:`CONFIG_TLS_CREDENTIALS_SHELL_CRED_OUTPUT_WIDTH` Kconfig option.

Here is an example of some data output by default settings:

[TODO EXAMPLE]
