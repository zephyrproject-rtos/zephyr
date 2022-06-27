.. _mcumgr_smp_group_8:

File management
###############

The file management group provides commands that allow to upload and download files
to/from a device.

File management group defines following commands:

.. table::
    :align: center

    +-------------------+-----------------------------------------------+
    | ``Command ID``    | Command description                           |
    +===================+===============================================+
    | ``0``             | File download/upload                          |
    +-------------------+-----------------------------------------------+
    | ``1``             | File status                                   |
    +-------------------+-----------------------------------------------+
    | ``2``             | File hash/checksum                            |
    +-------------------+-----------------------------------------------+

File download
*************

Command allows to download contents of an existing file from specified path
of a target device. The command is stateless and mcumgr does not hold file
in open state after response to the command is issued, instead a client
application is supposed to keep track of data it has already downloaded,
and issue subsequent requests, with modified offset, to gather entire file.
Request does not carry size of requested chunk, the size is specified
by application itself.
Mcumgr server side re-opens a file for each subsequent request, and current
specification does not provide means to identify subsequent requests as
belonging to specified download session. This means that the file is not
locked in any way or exclusively owned by mcumgr, for the time of download
session, and may change between requests or even be removed.

.. note::
    By default, all file upload requests are unconditionally allowed. However,
    if the Kconfig option :kconfig:option:`FS_MGMT_FILE_ACCESS_HOOK` is enabled,
    then an application can register a callback handler for ``fs_mgmt_on_evt_cb``
    by calling ``fs_mgmt_register_evt_cb()`` with the handler supplied. This can
    be used to allow or decline access to reading from or writing to a
    particular file, or for rewriting the path supplied by the client.

File download request
=====================

File download request header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``8``        |  ``0``         |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str)"off" :  (uint)
        (str)"name" : (str)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "off"                 | offset to start download at                       |
    +-----------------------+---------------------------------------------------+
    | "name"                | absolute path to a file                           |
    +-----------------------+---------------------------------------------------+

File download response
======================

File download response header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``8``        |  ``0``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"off"      : (uint)
        (str)"data"     : (byte str)
        (str)"rc"       : (int)
        (str,opt)"len"  : (uint)
    }

In case of error the CBOR data takes form:

.. code-block:: none

    {
        (str)"rc"           : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "off"                 | offset the response is for                        |
    +-----------------------+---------------------------------------------------+
    | "data"                | chunk of data read from file; it is CBOR encoded  |
    |                       | stream of bytes with embedded size;               |
    |                       | "data" appears only in responses where "rc" is 0  |
    +-----------------------+---------------------------------------------------+
    | "len"                 | length of file, this field is only mandatory      |
    |                       | when "off" is 0                                   |
    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes`           |
    +-----------------------+---------------------------------------------------+

In case when "rc" is not 0, success, the other fields will not appear.

File upload
***********

Allows to upload a file to a specified location. Command will automatically overwrite
existing file or create a new one if it does not exist at specified path.
The protocol supports stateless upload where each requests carries different chunk
of a file and it is client side responsibility to track progress of upload.

Mcumgr server side re-opens a file for each subsequent request, and current
specification does not provide means to identify subsequent requests as
belonging to specified upload session. This means that the file is not
locked in any way or exclusively owned by mcumgr, for the time of upload
session, and may change between requests or even be removed.

.. note::
    Weirdly, the current Zephyr implementation is half-stateless as is able to hold
    single upload context, holding information on ongoing upload, that consists
    of bool flag indicating in-progress upload, last successfully uploaded offset
    and total length only.

.. note::
    By default, all file upload requests are unconditionally allowed. However,
    if the Kconfig option :kconfig:option:`FS_MGMT_FILE_ACCESS_HOOK` is enabled,
    then an application can register a callback handler for ``fs_mgmt_on_evt_cb``
    by calling ``fs_mgmt_register_evt_cb()`` with the handler supplied. This can
    be used to allow or decline access to reading from or writing to a
    particular file, or for rewriting the path supplied by the client.

File upload request
===================

File upload request header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``2``  | ``8``        |  ``0``         |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str)"off"      : (uint)
        (str)"data"     : (str)
        (str)"name"     : (str)
        (str,opt)"len"  : (uint)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "off"                 | offset to start/continue upload at                |
    +-----------------------+---------------------------------------------------+
    | "data"                | chunk of data to write to the file;               |
    |                       | it is CBOR encoded with length embedded           |
    +-----------------------+---------------------------------------------------+
    | "name"                | absolute path to a file                           |
    +-----------------------+---------------------------------------------------+
    | "len"                 | length of file, this field is only mandatory      |
    |                       | when "off" is 0                                   |
    +-----------------------+---------------------------------------------------+

File upload response
====================

File upload response header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``3``  | ``8``        |  ``0``         |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str,opt)"off"      : (uint)
        (str)"rc"           : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "off"                 | offset of last successfully written data;         |
    |                       | appears only when "rc" is 0                       |
    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes`           |
    +-----------------------+---------------------------------------------------+

File status
***********

Command allows to retrieve status of an existing file from specified path
of a target device.

File status request
===================

File status request header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``8``        |  ``1``         |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str)"name" : (str)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "name"                | absolute path to a file                           |
    +-----------------------+---------------------------------------------------+

File status response
====================

File status response header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``8``        |  ``1``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"len"      : (uint)
    }

In case of error the CBOR data takes form:

.. code-block:: none

    {
        (str)"rc"       : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "len"                 | length of file (in bytes)                         |
    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes` (only     |
    |                       | present if an error occurred)                     |
    +-----------------------+---------------------------------------------------+

In case when "rc" is not 0, success, the other fields will not appear.

File hash/checksum
******************

Command allows to generate a hash/checksum of an existing file at a specified
path on a target device. Note that kernel heap memory is required for buffers to
be allocated for this to function, and large stack memory buffers are required
for generation of the output hash/checksum.

File hash/checksum request
==========================

File hash/checksum request header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``8``        |  ``2``         |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        (str)"name"     : (str)
        (str,opt)"type" : (str)
        (str,opt)"off"  : (uint)
        (str,opt)"len"  : (uint)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "name"                | absolute path to a file                           |
    +-----------------------+---------------------------------------------------+
    | "type"                | type of hash/checksum to perform                  |
    |                       | :ref:`mcumgr_group_8_hash_checksum_types` or omit |
    |                       | to use default                                    |
    +-----------------------+---------------------------------------------------+
    | "off"                 | offset to start hash/checksum calculation at      |
    |                       | (optional, 0 if not provided)                     |
    +-----------------------+---------------------------------------------------+
    | "len"                 | maximum length of data to read from file to       |
    |                       | generate hash/checksum with (optional, full file  |
    |                       | size if not provided)                             |
    +-----------------------+---------------------------------------------------+

.. _mcumgr_group_8_hash_checksum_types:

Hash/checksum types
===================

.. table::
    :align: center

    +-------------+--------------------------------------+-------------+--------------+
    | String name | Hash/checksum                        | Byte string | Size (bytes) |
    +=============+======================================+=============+==============+
    | ``crc32``   | IEEE CRC32 checksum                  | No          | 4            |
    +-------------+--------------------------------------+-------------+--------------+
    | ``sha256``  | SHA256 (Secure Hash Algorithm)       | Yes         | 32           |
    +-------------+--------------------------------------+-------------+--------------+

Note that the default type will be crc32 if it is enabled, or sha256 if crc32 is
not enabled.

File hash/checksum response
===========================

File hash/checksum response header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``8``        |  ``2``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"type"     : (str)
        (str,opt)"off"  : (uint)
        (str)"len"      : (uint)
        (str)"output"   : (uint or bstr)
    }

In case of error the CBOR data takes form:

.. code-block:: none

    {
        (str)"rc"       : (int)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "rc"                  | :ref:`mcumgr_smp_protocol_status_codes` (only     |
    |                       | present if an error occurred)                     |
    +-----------------------+---------------------------------------------------+
    | "type"                | type of hash/checksum that was performed          |
    |                       | :ref:`mcumgr_group_8_hash_checksum_types`         |
    +-----------------------+---------------------------------------------------+
    | "off"                 | offset that hash/checksum calculation started at  |
    |                       | (only present if off is not 0)                    |
    +-----------------------+---------------------------------------------------+
    | "len"                 | length of input data used for hash/checksum       |
    |                       | generation (in bytes)                             |
    +-----------------------+---------------------------------------------------+
    | "output"              | output hash/checksum                              |
    +-----------------------+---------------------------------------------------+

In case when "rc" is not 0, success, the other fields will not appear.
