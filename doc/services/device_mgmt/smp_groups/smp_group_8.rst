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
    | ``3``             | Supported file hash/checksum types            |
    +-------------------+-----------------------------------------------+
    | ``4``             | File close                                    |
    +-------------------+-----------------------------------------------+

File download
*************

Command allows to download contents of an existing file from specified path
of a target device. Client applications must keep track of data they have
already downloaded and where their position in the file is (MCUmgr will cache
these also), and issue subsequent requests, with modified offset, to gather
the entire file.
Request does not carry size of requested chunk, the size is specified
by application itself.
Note that file handles will remain open for consecutive requests (as long as
an idle timeout has not been reached and another transport does not make use
of uploading/downloading files using fs_mgmt), but files are not exclusively
owned by MCUmgr, for the time of download session, and may change between
requests or even be removed.

.. note::

    By default, all file upload/download requests are unconditionally allowed.
    However, if the Kconfig option
    :kconfig:option:`CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK` is enabled, then an
    application can register a callback handler for
    :c:enumerator:`MGMT_EVT_OP_FS_MGMT_FILE_ACCESS` (see
    :ref:`MCUmgr callbacks <mcumgr_callbacks>`), which allows for allowing or
    declining access to reading/writing a particular file, or for rewriting the
    path supplied by the client.

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
        (str,opt)"len"  : (uint)
    }

In case of error the CBOR data takes the form:

.. tabs::

   .. group-tab:: SMP version 2

      .. code-block:: none

          {
              (str)"err" : {
                  (str)"group"    : (uint)
                  (str)"rc"       : (uint)
              }
          }

   .. group-tab:: SMP version 1 (and non-group SMP version 2)

      .. code-block:: none

          {
              (str)"rc"       : (int)
          }

where:

.. table::
    :align: center

    +------------------+-------------------------------------------------------------------------+
    | "off"            | offset the response is for.                                             |
    +------------------+-------------------------------------------------------------------------+
    | "data"           | chunk of data read from file; it is CBOR encoded stream of bytes with   |
    |                  | embedded size; "data" appears only in responses where "rc" is 0.        |
    +------------------+-------------------------------------------------------------------------+
    | "len"            | length of file, this field is only mandatory when "off" is 0.           |
    +------------------+-------------------------------------------------------------------------+
    | "err" -> "group" | :c:enum:`mcumgr_group_t` group of the group-based error code. Only      |
    |                  | appears if an error is returned when using SMP version 2.               |
    +------------------+-------------------------------------------------------------------------+
    | "err" -> "rc"    | contains the index of the group-based error code. Only appears if       |
    |                  | non-zero (error condition) when using SMP version 2.                    |
    +------------------+-------------------------------------------------------------------------+
    | "rc"             | :c:enum:`mcumgr_err_t` only appears if non-zero (error condition) when  |
    |                  | using SMP version 1 or for SMP errors when using SMP version 2.         |
    +------------------+-------------------------------------------------------------------------+

File upload
***********

Allows to upload a file to a specified location. Command will automatically overwrite
existing file or create a new one if it does not exist at specified path.
The protocol supports stateless upload where each requests carries different chunk
of a file and it is client side responsibility to track progress of upload.

Note that file handles will remain open for consecutive requests (as long as
an idle timeout has not been reached, but files are not exclusively owned by
MCUmgr, for the time of download session, and may change between requests or
even be removed. Note that file handles will remain open for consecutive
requests (as long as an idle timeout has not been reached and another transport
does not make use of uploading/downloading files using fs_mgmt), but files are
not exclusively owned by MCUmgr, for the time of download session, and may
change between requests or even be removed.

.. note::
    Weirdly, the current Zephyr implementation is half-stateless as is able to hold
    single upload context, holding information on ongoing upload, that consists
    of bool flag indicating in-progress upload, last successfully uploaded offset
    and total length only.

.. note::

    By default, all file upload/download requests are unconditionally allowed.
    However, if the Kconfig option
    :kconfig:option:`CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK` is enabled, then an
    application can register a callback handler for
    :c:enumerator:`MGMT_EVT_OP_FS_MGMT_FILE_ACCESS` (see
    :ref:`MCUmgr callbacks <mcumgr_callbacks>`), which allows for allowing or
    declining access to reading/writing a particular file, or for rewriting the
    path supplied by the client.

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
    | "off"                 | offset to start/continue upload at.               |
    +-----------------------+---------------------------------------------------+
    | "data"                | chunk of data to write to the file;               |
    |                       | it is CBOR encoded with length embedded.          |
    +-----------------------+---------------------------------------------------+
    | "name"                | absolute path to a file.                          |
    +-----------------------+---------------------------------------------------+
    | "len"                 | length of file, this field is only mandatory      |
    |                       | when "off" is 0.                                  |
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

CBOR data of successful response:

.. code-block:: none

    {
        (str)"off"      : (uint)
    }

In case of error the CBOR data takes the form:

.. .. tabs::

   .. group-tab:: SMP version 2

      .. code-block:: none

          {
              (str)"err" : {
                  (str)"group"    : (uint)
                  (str)"rc"       : (uint)
              }
          }

   .. group-tab:: SMP version 1 (and non-group SMP version 2)

      .. code-block:: none

          {
              (str)"rc"       : (int)
          }

where:

.. table::
    :align: center

    +------------------+-------------------------------------------------------------------------+
    | "off"            | offset of last successfully written data.                               |
    +------------------+-------------------------------------------------------------------------+
    | "err" -> "group" | :c:enum:`mcumgr_group_t` group of the group-based error code. Only      |
    |                  | appears if an error is returned when using SMP version 2.               |
    +------------------+-------------------------------------------------------------------------+
    | "err" -> "rc"    | contains the index of the group-based error code. Only appears if       |
    |                  | non-zero (error condition) when using SMP version 2.                    |
    +------------------+-------------------------------------------------------------------------+
    | "rc"             | :c:enum:`mcumgr_err_t` only appears if non-zero (error condition) when  |
    |                  | using SMP version 1 or for SMP errors when using SMP version 2.         |
    +------------------+-------------------------------------------------------------------------+

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
    | "name"                | absolute path to a file.                          |
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

.. tabs::

   .. group-tab:: SMP version 2

      .. code-block:: none

          {
              (str)"err" : {
                  (str)"group"    : (uint)
                  (str)"rc"       : (uint)
              }
          }

   .. group-tab:: SMP version 1 (and non-group SMP version 2)

      .. code-block:: none

          {
              (str)"rc"       : (int)
          }

where:

.. table::
    :align: center

    +------------------+-------------------------------------------------------------------------+
    | "len"            | length of file (in bytes).                                              |
    +------------------+-------------------------------------------------------------------------+
    | "err" -> "group" | :c:enum:`mcumgr_group_t` group of the group-based error code. Only      |
    |                  | appears if an error is returned when using SMP version 2.               |
    +------------------+-------------------------------------------------------------------------+
    | "err" -> "rc"    | contains the index of the group-based error code. Only appears if       |
    |                  | non-zero (error condition) when using SMP version 2.                    |
    +------------------+-------------------------------------------------------------------------+
    | "rc"             | :c:enum:`mcumgr_err_t` only appears if non-zero (error condition) when  |
    |                  | using SMP version 1 or for SMP errors when using SMP version 2.         |
    +------------------+-------------------------------------------------------------------------+

File hash/checksum
******************

Command allows to generate a hash/checksum of an existing file at a specified
path on a target device. Note that kernel heap memory is required for buffers to
be allocated for this to function, and large stack memory buffers are required
for generation of the output hash/checksum.
Requires :kconfig:option:`CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH` to be enabled for
the base functionality, supported hash/checksum are opt-in with
:kconfig:option:`CONFIG_MCUMGR_GRP_FS_CHECKSUM_IEEE_CRC32` or
:kconfig:option:`CONFIG_MCUMGR_GRP_FS_HASH_SHA256`.

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
    | "name"                | absolute path to a file.                          |
    +-----------------------+---------------------------------------------------+
    | "type"                | type of hash/checksum to perform                  |
    |                       | :ref:`mcumgr_group_8_hash_checksum_types` or omit |
    |                       | to use default.                                   |
    +-----------------------+---------------------------------------------------+
    | "off"                 | offset to start hash/checksum calculation at      |
    |                       | (optional, 0 if not provided).                    |
    +-----------------------+---------------------------------------------------+
    | "len"                 | maximum length of data to read from file to       |
    |                       | generate hash/checksum with (optional, full file  |
    |                       | size if not provided).                            |
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

In case of error the CBOR data takes the form:

.. tabs::

   .. group-tab:: SMP version 2

      .. code-block:: none

          {
              (str)"err" : {
                  (str)"group"    : (uint)
                  (str)"rc"       : (uint)
              }
          }

   .. group-tab:: SMP version 1 (and non-group SMP version 2)

      .. code-block:: none

          {
              (str)"rc"       : (int)
          }

where:

.. table::
    :align: center

    +------------------+-------------------------------------------------------------------------+
    | "type"           | type of hash/checksum that was performed                                |
    |                  | :ref:`mcumgr_group_8_hash_checksum_types`.                              |
    +------------------+-------------------------------------------------------------------------+
    | "off"            | offset that hash/checksum calculation started at (only present if not   |
    |                  | 0).                                                                     |
    +------------------+-------------------------------------------------------------------------+
    | "len"            | length of input data used for hash/checksum generation (in bytes).      |
    +------------------+-------------------------------------------------------------------------+
    | "output"         | output hash/checksum.                                                   |
    +------------------+-------------------------------------------------------------------------+
    | "err" -> "group" | :c:enum:`mcumgr_group_t` group of the group-based error code. Only      |
    |                  | appears if an error is returned when using SMP version 2.               |
    +------------------+-------------------------------------------------------------------------+
    | "err" -> "rc"    | contains the index of the group-based error code. Only appears if       |
    |                  | non-zero (error condition) when using SMP version 2.                    |
    +------------------+-------------------------------------------------------------------------+
    | "rc"             | :c:enum:`mcumgr_err_t` only appears if non-zero (error condition) when  |
    |                  | using SMP version 1 or for SMP errors when using SMP version 2.         |
    +------------------+-------------------------------------------------------------------------+

Supported file hash/checksum types
**********************************

Command allows listing which hash and checksum types are available on a device.
Requires Kconfig :kconfig:option:`CONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH_SUPPORTED_CMD`
to be enabled.

Supported file hash/checksum types request
==========================================

Supported file hash/checksum types request header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``8``        |  ``3``         |
    +--------+--------------+----------------+

The command sends empty CBOR map as data.

Supported file hash/checksum types response
===========================================

Supported file hash/checksum types response header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``8``        |  ``3``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str)"types" : {
            (str)<hash_checksum_name> : {
                (str)"format"       : (uint)
                (str)"size"         : (uint)
            }
            ...
        }
    }

In case of error the CBOR data takes form:

.. tabs::

   .. group-tab:: SMP version 2

      .. code-block:: none

          {
              (str)"err" : {
                  (str)"group"    : (uint)
                  (str)"rc"       : (uint)
              }
          }

   .. group-tab:: SMP version 1 (and non-group SMP version 2)

      .. code-block:: none

          {
              (str)"rc"       : (int)
          }

where:

.. table::
    :align: center

    +----------------------+-------------------------------------------------------------------------+
    | <hash_checksum_name> | name of the hash/checksum type                                          |
    |                      | :ref:`mcumgr_group_8_hash_checksum_types`.                              |
    +----------------------+-------------------------------------------------------------------------+
    | "format"             | format that the hash/checksum returns where 0 is for numerical and 1 is |
    |                      | for byte array.                                                         |
    +----------------------+-------------------------------------------------------------------------+
    | "size"               | size (in bytes) of output hash/checksum response.                       |
    +----------------------+-------------------------------------------------------------------------+
    | "err" -> "group"     | :c:enum:`mcumgr_group_t` group of the group-based error code. Only      |
    |                      | appears if an error is returned when using SMP version 2.               |
    +----------------------+-------------------------------------------------------------------------+
    | "err" -> "rc"        | contains the index of the group-based error code. Only appears if       |
    |                      | non-zero (error condition) when using SMP version 2.                    |
    +----------------------+-------------------------------------------------------------------------+
    | "rc"                 | :c:enum:`mcumgr_err_t` only appears if non-zero (error condition) when  |
    |                      | using SMP version 1 or for SMP errors when using SMP version 2.         |
    +----------------------+-------------------------------------------------------------------------+

File close
**********

Command allows closing any open file handles held by fs_mgmt upload/download
requests that might have stalled or be incomplete.

File close request
==================

File close request header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``2``  | ``8``        |  ``4``         |
    +--------+--------------+----------------+

The command sends empty CBOR map as data.

File close response
===================

File close response header:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``3``  | ``8``        |  ``4``         |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data if successful.
In case of error the CBOR data takes the form:

.. tabs::

   .. group-tab:: SMP version 2

      .. code-block:: none

          {
              (str)"err" : {
                  (str)"group"    : (uint)
                  (str)"rc"       : (uint)
              }
          }

   .. group-tab:: SMP version 1 (and non-group SMP version 2)

      .. code-block:: none

          {
              (str)"rc"       : (int)
          }

where:

.. table::
    :align: center

    +------------------+-------------------------------------------------------------------------+
    | "err" -> "group" | :c:enum:`mcumgr_group_t` group of the group-based error code. Only      |
    |                  | appears if an error is returned when using SMP version 2.               |
    +------------------+-------------------------------------------------------------------------+
    | "err" -> "rc"    | contains the index of the group-based error code. Only appears if       |
    |                  | non-zero (error condition) when using SMP version 2.                    |
    +------------------+-------------------------------------------------------------------------+
    | "rc"             | :c:enum:`mcumgr_err_t` only appears if non-zero (error condition) when  |
    |                  | using SMP version 1 or for SMP errors when using SMP version 2.         |
    +------------------+-------------------------------------------------------------------------+
