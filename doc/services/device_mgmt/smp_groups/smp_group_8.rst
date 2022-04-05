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
