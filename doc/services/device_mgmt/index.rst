.. _device_mgmt:

Device Management
#################

.. toctree::
    :maxdepth: 1

    mcumgr.rst
    mcumgr_handlers.rst
    mcumgr_callbacks.rst
    mcumgr_backporting.rst
    smp_protocol.rst
    smp_transport.rst
    dfu.rst
    ota.rst
    ec_host_cmd.rst

SMP Groups
==========

Note that in MCUmgr group documentation, strings that have quote marks are ``tstr`` (text string)
entities. The rest of the format follows CDDL-style formatting, whereby a ``?`` indicates an
optional field, a ``/`` indicates different possible variable types.

Note that The text view is designed to make it easy to understand queries rather than being 100%
valid CDDL code.

.. toctree::
    :maxdepth: 1

    smp_groups/smp_group_0.rst
    smp_groups/smp_group_1.rst
    smp_groups/smp_group_2.rst
    smp_groups/smp_group_3.rst
    smp_groups/smp_group_8.rst
    smp_groups/smp_group_9.rst
