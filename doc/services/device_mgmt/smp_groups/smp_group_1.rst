.. _mcumgr_smp_group_1:

Application/software image management group
###########################################

Application/software image management management group defines following commands:

.. table::
    :align: center

    +-------------------+-----------------------------------------------+
    | ``Command ID``    | Command description                           |
    +===================+===============================================+
    | ``0``             | State of images                               |
    +-------------------+-----------------------------------------------+
    | ``1``             | Image upload                                  |
    +-------------------+-----------------------------------------------+
    | ``2``             | File                                          |
    |                   | (reserved but not supported by Zephyr)        |
    +-------------------+-----------------------------------------------+
    | ``3``             | Corelist                                      |
    |                   | (reserved but not supported by Zephyr)        |
    +-------------------+-----------------------------------------------+
    | ``4``             | Coreload                                      |
    |                   | (reserved but not supported by Zephyr)        |
    +-------------------+-----------------------------------------------+
    | ``5``             | Image erase                                   |
    +-------------------+-----------------------------------------------+

Notion of "slots" and "images" in Zephyr
****************************************

The "slot" and "image" definition comes from mcuboot where "image" would
consist of two "slots", further named "primary" and "secondary"; the application
is supposed to run from the "primary slot" and update is supposed to be
uploaded to the "secondary slot";  the mcuboot is responsible in swapping
slots on boot.
This means that pair of slots is dedicated to single upgradable application.
In case of Zephyr this gets a little bit confusing because DTS will use
"slot0_partition" and "slot1_partition", as label of ``fixed-partition`` dedicated
to single application, but will name them as "image-0" and "image-1" respectively.

Currently Zephyr supports at most two images, in which case mapping is as follows:

.. table::
    :align: center

    +-------------+-------------------+---------------+
    | Image       | Slot labels       | Slot  Names   |
    +=============+===================+===============+
    | 1           | "slot0_partition" |   "image-0"   |
    |             | "slot1_partition" |   "image-1"   |
    +-------------+-------------------+---------------+
    | 2           | "slot2_partition" |   "image-2"   |
    |             | "slot3_partition" |   "image-3"   |
    +-------------+-------------------+---------------+

State of images
***************

The command is used to set state of images and obtain list of images
with their current state.

Get state of images request
===========================

Get state of images request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``0``  | ``1``        |  ``0``         |
    +--------+--------------+----------------+

The command sends sends empty CBOR map as data.

.. _mcumgr_smp_protocol_op_1_grp_1_cmd_0:

Get state of images response
============================

Get state of images response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``1``  | ``1``        |  ``0``         |
    +--------+--------------+----------------+

.. note::
    Below definition of the response contains "image" field that has been marked
    as optional(opt): the field may not appear in response when target application
    does not support more than one image. The field is mandatory when application
    supports more than one application image to allow identifying which image
    information is listed.

A response will only contain information for valid images, if an image can not
be identified as valid it is simply skipped.

CBOR data of successful response:

.. code-block:: none

    {
        (str)"images" : [
            {
                (str,opt)"image"        : (int)
                (str)"slot"             : (int)
                (str)"version"          : (str)
                (str,opt*)"hash"        : (byte str)
                (str,opt)"bootable"     : (bool)
                (str,opt)"pending"      : (bool)
                (str,opt)"confirmed"    : (bool)
                (str,opt)"active"       : (bool)
                (str,opt)"permanent"    : (bool)
            }
            ...
        ]
        (str,opt)"splitStatus" : (int)
    }

In case of error the CBOR data takes the form:

.. code-block:: none

    {
        (str)"rc"      : (int)
        (str,opt)"rsn" : (str)
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "image"               | semi-optional image number; the field is not      |
    |                       | required when only one image is supported by      |
    |                       | running application                               |
    +-----------------------+---------------------------------------------------+
    | "slot"                | slot number within "image"; each image has two    |
    |                       | slots : primary (running one) = 0 and secondary   |
    |                       | (for DFU dual-bank purposes) = 1                  |
    +-----------------------+---------------------------------------------------+
    | "version"             | string representing image version, as set with    |
    |                       | ``imgtool``                                       |
    +-----------------------+---------------------------------------------------+
    | "hash"                | SHA256 hash of the image header and body. Note    |
    |                       | that this will not be the same as the SHA256 of   |
    |                       | the whole file, it is the field in the MCUboot    |
    |                       | TLV section that contains a hash of the data      |
    |                       | which is used for signature verification          |
    |                       | purposes. This field is optional but only         |
    |                       | optional when using MCUboot's serial recovery     |
    |                       | feature with one pair of image slots, Kconfig     |
    |                       | :kconfig:option:`CONFIG_BOOT_SERIAL_IMG_GRP_HASH` |
    |                       | can be disabled to remove support for hashes in   |
    |                       | this configuration. MCUmgr in applications must   |
    |                       | support sending hashes.                           |
    |                       |                                                   |
    |                       | .. note::                                         |
    |                       |    See ``IMAGE_TLV_SHA256`` in the MCUboot image  |
    |                       |    format documentation link below.               |
    +-----------------------+---------------------------------------------------+
    | "bootable"            | true if image has bootable flag set;              |
    |                       | this field does not have to be present if false   |
    +-----------------------+---------------------------------------------------+
    | "pending"             | true if image is set for next swap                |
    |                       | this field does not have to be present if false   |
    +-----------------------+---------------------------------------------------+
    | "confirmed"           | true if image has been confirmed                  |
    |                       | this field does not have to be present if false   |
    +-----------------------+---------------------------------------------------+
    | "active"              | true if image is currently active application     |
    |                       | this field does not have to be present if false   |
    +-----------------------+---------------------------------------------------+
    | "permanent"           | true if image is to stay in primary slot after    |
    |                       | next boot                                         |
    |                       | this field does not have to be present if false   |
    +-----------------------+---------------------------------------------------+
    | "splitStatus"         | states whether loader of split image is compatible|
    |                       | with application part; this is unused by Zephyr   |
    +-----------------------+---------------------------------------------------+
    | "rc"                  | :c:enum:`mcumgr_err_t`                            |
    |                       | only appears if non-zero (error condition).       |
    +-----------------------+---------------------------------------------------+
    | "rsn"                 | optional string that clarifies reason for an      |
    |                       | error; specifically useful for error code ``1``,  |
    |                       | unknown error                                     |
    +-----------------------+---------------------------------------------------+

.. note::
    For more information on how does image/slots function, please refer to
    the MCUBoot documentation
    https://www.mcuboot.com/documentation/design/#image-slots
    For information on MCUboot image format, please reset to the MCUboot
    documentation https://docs.mcuboot.com/design.html#image-format


Set state of image request
==========================

Set state of image request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``2``  | ``1``        |  ``0``         |
    +--------+--------------+----------------+

CBOR data of request:


.. code-block:: none

    {
        {
            (str,opt)"hash"     : (str)
            (str)"confirm"      : (bool)
        }
    }

If "confirm" is false or not provided, an image with the "hash" will be set for
test, which means that it will not be marked as permanent and upon hard reset
the previous application will be restored to the primary slot.
In case when "confirm" is true, the "hash" is optional as the currently running
application will be assumed as target for confirmation.

Set state of image response
============================

The response takes the same format as :ref:`mcumgr_smp_protocol_op_1_grp_1_cmd_0`

Image upload
************

The image upload command allows to update application image.

Image upload request
====================

The image upload request is sent for each chunk of image that is uploaded, until
complete image gets uploaded to a device.

Image upload request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``2``  | ``1``        |  ``1``         |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        {
            (str,opt)"image"    : (uint)
            (str,opt)"len"      : (uint)
            (str)"off"          : (uint)
            (str,opt)"sha"      : (byte str)
            (str,opt)"data"     : (byte str)
            (str,opt)"upgrade"  : (bool)
        }
    }

where:

.. table::
    :align: center

    +-----------------------+---------------------------------------------------+
    | "image"               | optional image number, it does not have to appear |
    |                       | in request at all, in which case it is assumed to |
    |                       | be 0; only request with "off" 0 can contain       |
    |                       | image number                                      |
    +-----------------------+---------------------------------------------------+
    | "len"                 | optional length of an image, it only appears in   |
    |                       | the first packet of request, where "off" is 0     |
    +-----------------------+---------------------------------------------------+
    | "off"                 | offset of image chunk the request carries         |
    +-----------------------+---------------------------------------------------+
    | "sha"                 | SHA256 hash of an upload; this is used to         |
    |                       | identify an upload session, for example to allow  |
    |                       | MCUmgr to continue a broken session. This must be |
    |                       | a full SHA256 of the whole image being uploaded,  |
    |                       | and is optionally used for image verification     |
    |                       | purposes. Should only be present if "off" is zero |
    +-----------------------+---------------------------------------------------+
    | "data"                | optional image data                               |
    +-----------------------+---------------------------------------------------+
    | "upgrade"             | optional flag that states that only upgrade       |
    |                       | should be allowed, so if the version of uploaded  |
    |                       | software is not higher then already on a device,  |
    |                       | the image upload will be rejected.                |
    |                       | Zephyr only compares major, minor and revision    |
    |                       | (x.y.z).                                          |
    +-----------------------+---------------------------------------------------+

.. note::
    There is no field representing size of chunk that is carried as "data" because
    that information is embedded within "data" field itself.

The MCUmgr library uses "sha" field to tag ongoing update session, to be able
to continue it in case when it gets broken, and for upload verification
purposes.
If library gets request with "off" equal zero it checks stored "sha" within its
state and if it matches it will respond to update client application with
offset that it should continue with.

Image upload response
=====================

Image upload response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``3``  | ``1``        |  ``1``         |
    +--------+--------------+----------------+

CBOR data of successful response:

.. code-block:: none

    {
        (str,opt)"off"    : (uint)
        (str,opt)"match"  : (bool)
    }

In case of error the CBOR data takes the form:

.. code-block:: none

    {
        (str)"rc"       : (int)
        (str,opt)"rsn"  : (str)
    }

where:

.. table::
    :align: center

    +-----------------------+-----------------------------------------------------+
    | "off"                 | offset of last successfully written byte of update. |
    +-----------------------+-----------------------------------------------------+
    | "match"               | indicates if the uploaded data successfully matches |
    |                       | the provided SHA256 hash or not, only sent in the   |
    |                       | final packet if                                     |
    |                       | :kconfig:option:`CONFIG_IMG_ENABLE_IMAGE_CHECK` is  |
    |                       | enabled.                                            |
    +-----------------------+-----------------------------------------------------+
    | "rc"                  | :c:enum:`mcumgr_err_t` only                         |
    |                       | appears if non-zero (error condition).              |
    +-----------------------+-----------------------------------------------------+
    | "rsn"                 | Optional string that clarifies reason for an error; |
    |                       | specifically useful for error code ``1``, unknown   |
    |                       | error.                                              |
    +-----------------------+-----------------------------------------------------+

The "off" field is only included in responses to successfully processed requests;
if "rc" is negative then "off" may not appear.

Image erase
***********

The command is used for erasing image slot on a target device.

.. note::
    This is synchronous command which means that a sender of request will not
    receive response until the command completes, which can take a long time.

Image erase request
===================

Image erase request header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``2``  | ``1``        |  ``5``         |
    +--------+--------------+----------------+

CBOR data of request:

.. code-block:: none

    {
        {
            (str,opt)"slot"     : (uint)
        }
    }

where:

.. table::
    :align: center

    +---------+-----------------------------------------------------------------+
    | "slot"  | optional slot number, it does not have to appear in the request |
    |         | at all, in which case it is assumed to be 1.                    |
    +---------+-----------------------------------------------------------------+

Image erase response
====================

Image erase response header fields:

.. table::
    :align: center

    +--------+--------------+----------------+
    | ``OP`` | ``Group ID`` | ``Command ID`` |
    +========+==============+================+
    | ``3``  | ``1``        |  ``5``         |
    +--------+--------------+----------------+

The command sends an empty CBOR map as data if successful. In case of error the
CBOR data takes the form:

.. code-block:: none

    {
        (str)"rc"       : (int)
        (str,opt)"rsn"  : (str)
    }

where:

.. table::
    :align: center

    +-----------------------+--------------------------------------------------+
    | "rc"                  | :c:enum:`mcumgr_err_t`                           |
    |                       | only appears if non-zero (error condition).      |
    +-----------------------+--------------------------------------------------+
    | "rsn"                 | Optional string that clarifies reason for an     |
    |                       | error; specifically useful when rc value is      |
    |                       | :c:enum:`MGMT_ERR_EUNKNOWN`                      |
    +-----------------------+--------------------------------------------------+

.. note::
    Response from Zephyr running device may have "rc" value of
    :c:enum:`MGMT_ERR_EBADSTATE`, which means that the secondary
    image has been marked for next boot already and may not be erased.
