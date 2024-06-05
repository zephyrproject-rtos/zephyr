.. _mcumgr_callbacks:

MCUmgr Callbacks
################

Overview
********

MCUmgr has a customisable callback/notification system that allows application
(and module) code to receive callbacks for MCUmgr events that they are
interested in and react to them or return a status code to the calling function
that provides control over if the action should be allowed or not. An example
of this is with the fs_mgmt group, whereby file access can be gated, the
callback allows the application to inspect the request path and allow or deny
access to said file, or it can rewrite the provided path to a different path
for transparent file redirection support.

Implementation
**************

Enabling
========

The base callback/notification system can be enabled using
:kconfig:option:`CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS` which will compile the
registration and notification system into the code. This will not provide any
callbacks by default as the callbacks that are supported by a build must also
be selected by enabling the Kconfig's for the required callbacks (see
:ref:`mcumgr_cb_events` for further details). A callback function with the
:c:type:`mgmt_cb` type definition can then be declared and registered by
calling :c:func:`mgmt_callback_register` for the desired event inside of a
:c:struct`mgmt_callback` structure. Handlers are called in the order that they
were registered.

With the system enabled, a basic handler can be set up and defined in
application code as per:

.. code-block:: c

    #include <zephyr/kernel.h>
    #include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
    #include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>

    struct mgmt_callback my_callback;

    enum mgmt_cb_return my_function(uint32_t event, enum mgmt_cb_return prev_status,
                                    int32_t *rc, uint16_t *group, bool *abort_more,
                                    void *data, size_t data_size)
    {
        if (event == MGMT_EVT_OP_CMD_DONE) {
            /* This is the event we registered for */
        }

        /* Return OK status code to continue with acceptance to underlying handler */
        return MGMT_CB_OK;
    }

    int main()
    {
        my_callback.callback = my_function;
        my_callback.event_id = MGMT_EVT_OP_CMD_DONE;
        mgmt_callback_register(&my_callback);
    }

This code registers a handler for the :c:enumerator:`MGMT_EVT_OP_CMD_DONE`
event, which will be called after a MCUmgr command has been processed and
output generated, note that this requires that
:kconfig:option:`CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS` be enabled to receive
this callback.

Multiple callbacks can be setup to use a single function as a common callback,
and many different functions can be used for each event by registering each
group once, or all notifications for a whole group can be enabled by using one
of the ``MGMT_EVT_OP_*_ALL`` events, alternatively a handler can setup for
every notification by using :c:enumerator:`MGMT_EVT_OP_ALL`. When setting up
handlers, events can be combined that are in the same group only, for example
5 img_mgmt callbacks can be setup with a single registration call, but to also
setup a callback for an os_mgmt callback, this must be done as a separate
registration. Group IDs are numerical increments, event IDs are bitmask values,
hence the restriction.

As an example, the following registration is allowed, which will register for 3
SMP events with a single callback function in a single registration:

.. code-block:: c

    my_callback.callback = my_function;
    my_callback.event_id = (MGMT_EVT_OP_CMD_RECV |
                            MGMT_EVT_OP_CMD_STATUS |
                            MGMT_EVT_OP_CMD_DONE);
    mgmt_callback_register(&my_callback);

The following code is not allowed, and will cause undefined operation, because
it mixes the IMG management group with the OS management group whereby the
group is **not** a bitmask value, only the event is:

.. code-block:: c

    my_callback.callback = my_function;
    my_callback.event_id = (MGMT_EVT_OP_IMG_MGMT_DFU_STARTED |
                            MGMT_EVT_OP_OS_MGMT_RESET);
    mgmt_callback_register(&my_callback);

.. _mcumgr_cb_events:

Events
======

Events can be selected by enabling their corresponding Kconfig option:

 - :kconfig:option:`CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS`
    MCUmgr command status (:c:enumerator:`MGMT_EVT_OP_CMD_RECV`,
    :c:enumerator:`MGMT_EVT_OP_CMD_STATUS`,
    :c:enumerator:`MGMT_EVT_OP_CMD_DONE`)
 - :kconfig:option:`CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK`
    fs_mgmt file access (:c:enumerator:`MGMT_EVT_OP_FS_MGMT_FILE_ACCESS`)
 - :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK`
    img_mgmt upload check (:c:enumerator:`MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK`)
 - :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS`
    img_mgmt upload status (:c:enumerator:`MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED`,
    :c:enumerator:`MGMT_EVT_OP_IMG_MGMT_DFU_STARTED`,
    :c:enumerator:`MGMT_EVT_OP_IMG_MGMT_DFU_PENDING`,
    :c:enumerator:`MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED`)
 - :kconfig:option:`CONFIG_MCUMGR_GRP_OS_RESET_HOOK`
    os_mgmt reset check (:c:enumerator:`MGMT_EVT_OP_OS_MGMT_RESET`)
 - :kconfig:option:`CONFIG_MCUMGR_GRP_SETTINGS_ACCESS_HOOK`
    settings_mgmt access (:c:enumerator:`MGMT_EVT_OP_SETTINGS_MGMT_ACCESS`)

Actions
=======

Some callbacks expect a return status to either allow or disallow an operation,
an example is the fs_mgmt access hook which allows for access to files to be
allowed or denied. With these handlers, the first non-OK error code returned
by a handler will be returned to the MCUmgr client.

An example of selectively denying file access:

.. code-block:: c

    #include <zephyr/kernel.h>
    #include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
    #include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>
    #include <string.h>

    struct mgmt_callback my_callback;

    enum mgmt_cb_return my_function(uint32_t event, enum mgmt_cb_return prev_status,
                                    int32_t *rc, uint16_t *group, bool *abort_more,
                                    void *data, size_t data_size)
    {
        /* Only run this handler if a previous handler has not failed */
        if (event == MGMT_EVT_OP_FS_MGMT_FILE_ACCESS && prev_status == MGMT_CB_OK) {
            struct fs_mgmt_file_access *fs_data = (struct fs_mgmt_file_access *)data;

            /* Check if this is an upload and deny access if it is, otherwise check
             * the path and deny if is matches a name
             */
            if (fs_data->access == FS_MGMT_FILE_ACCESS_WRITE) {
                /* Return an access denied error code to the client and abort calling
                 * further handlers
                 */
                *abort_more = true;
                *rc = MGMT_ERR_EACCESSDENIED;

                return MGMT_CB_ERROR_RC;
            } else if (strcmp(fs_data->filename, "/lfs1/false_deny.txt") == 0) {
                /* Return a no entry error code to the client, call additional handlers
                 * (which will have failed set to true)
                 */
                *rc = MGMT_ERR_ENOENT;

                return MGMT_CB_ERROR_RC;
            }
        }

        /* Return OK status code to continue with acceptance to underlying handler */
        return MGMT_CB_OK;
    }

    int main()
    {
        my_callback.callback = my_function;
        my_callback.event_id = MGMT_EVT_OP_FS_MGMT_FILE_ACCESS;
        mgmt_callback_register(&my_callback);
    }

This code registers a handler for the
:c:enumerator:`MGMT_EVT_OP_FS_MGMT_FILE_ACCESS` event, which will be called
after a fs_mgmt file read/write command has been received to check if access to
the file should be allowed or not, note that this requires that
:kconfig:option:`CONFIG_MCUMGR_GRP_FS_FILE_ACCESS_HOOK` be enabled to receive
this callback.
Two types of errors can be returned, the ``rc`` parameter can be set to an
:c:enum:`mcumgr_err_t` error code and :c:enumerator:`MGMT_CB_ERROR_RC`
can be returned, or a group error code (introduced with version 2 of the MCUmgr
protocol) can be set by setting the ``group`` value to the group and ``rc``
value to the group error code and returning :c:enumerator:`MGMT_CB_ERROR_ERR`.

MCUmgr Command Callback Usage/Adding New Event Types
====================================================

To add a callback to a MCUmgr command, :c:func:`mgmt_callback_notify` can be
called with the event ID and, optionally, a data struct to pass to the callback
(which can be modified by handlers). If no data needs to be passed back,
``NULL`` can be used instead, and size of the data set to 0.

An example MCUmgr command handler:

.. code-block:: c

    #include <zephyr/kernel.h>
    #include <zcbor_common.h>
    #include <zcbor_encode.h>
    #include <zephyr/mgmt/mcumgr/smp/smp.h>
    #include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
    #include <zephyr/mgmt/mcumgr/mgmt/callbacks.h>

    #define MGMT_EVT_GRP_USER_ONE MGMT_EVT_GRP_USER_CUSTOM_START

    enum user_one_group_events {
        /** Callback on first post, data is test_struct. */
        MGMT_EVT_OP_USER_ONE_FIRST  = MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_USER_ONE, 0),

        /** Callback on second post, data is test_struct. */
        MGMT_EVT_OP_USER_ONE_SECOND = MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_USER_ONE, 1),

        /** Used to enable all user_one events. */
        MGMT_EVT_OP_USER_ONE_ALL    = MGMT_DEF_EVT_OP_ALL(MGMT_EVT_GRP_USER_ONE),
    };

    struct test_struct {
        uint8_t some_value;
    };

    static int test_command(struct mgmt_ctxt *ctxt)
    {
        int rc;
        int err_rc;
        uint16_t err_group;
        zcbor_state_t *zse = ctxt->cnbe->zs;
        bool ok;
        struct test_struct test_data = {
            .some_value = 8,
        };

        rc = mgmt_callback_notify(MGMT_EVT_OP_USER_ONE_FIRST, &test_data,
                                  sizeof(test_data), &err_rc, &err_group);

        if (rc != MGMT_CB_OK) {
            /* A handler returned a failure code */
            if (rc == MGMT_CB_ERROR_RC) {
                /* The failure code is the RC value */
                return err_rc;
            }

            /* The failure is a group and ID error value */
            ok = smp_add_cmd_err(zse, err_group, (uint16_t)err_rc);
            goto end;
        }

        /* All handlers returned success codes */
        ok = zcbor_tstr_put_lit(zse, "output_value") &&
             zcbor_int32_put(zse, 1234);

    end:
        rc = (ok ? MGMT_ERR_EOK : MGMT_ERR_EMSGSIZE);

        return rc;
    }

If no response is required for the callback, the function call be called and
casted to void.

.. _mcumgr_cb_migration:

Migration
*********

If there is existing code using the previous callback system(s) in Zephyr 3.2
or earlier, then it will need to be migrated to the new system. To migrate
code, the following callback registration functions will need to be migrated
to register for callbacks using :c:func:`mgmt_callback_register` (note that
:kconfig:option:`CONFIG_MCUMGR_MGMT_NOTIFICATION_HOOKS` will need to be set to
enable the new notification system in addition to any migrations):

 * mgmt_evt
    Using :c:enumerator:`MGMT_EVT_OP_CMD_RECV`,
    :c:enumerator:`MGMT_EVT_OP_CMD_STATUS`, or
    :c:enumerator:`MGMT_EVT_OP_CMD_DONE` as drop-in replacements for events of
    the same name, where the provided data is :c:struct:`mgmt_evt_op_cmd_arg`.
    :kconfig:option:`CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS` needs to be set.
 * fs_mgmt_register_evt_cb
    Using :c:enumerator:`MGMT_EVT_OP_FS_MGMT_FILE_ACCESS` where the provided
    data is :c:struct:`fs_mgmt_file_access`. Instead of returning true to allow
    the action or false to deny, a MCUmgr result code needs to be returned,
    :c:enumerator:`MGMT_ERR_EOK` will allow the action, any other return code
    will disallow it and return that code to the client
    (:c:enumerator:`MGMT_ERR_EACCESSDENIED` can be used for an access denied
    error). :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS` needs to be
    set.
 * img_mgmt_register_callbacks
    Using :c:enumerator:`MGMT_EVT_OP_IMG_MGMT_DFU_STARTED` if
    ``dfu_started_cb`` was used,
    :c:enumerator:`MGMT_EVT_OP_IMG_MGMT_DFU_STOPPED` if ``dfu_stopped_cb`` was
    used, :c:enumerator:`MGMT_EVT_OP_IMG_MGMT_DFU_PENDING` if
    ``dfu_pending_cb`` was used or
    :c:enumerator:`MGMT_EVT_OP_IMG_MGMT_DFU_CONFIRMED` if ``dfu_confirmed_cb``
    was used. These callbacks do not have any return status.
    :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_STATUS_HOOKS` needs to be set.
 * img_mgmt_set_upload_cb
    Using :c:enumerator:`MGMT_EVT_OP_IMG_MGMT_DFU_CHUNK` where the provided
    data is :c:struct:`img_mgmt_upload_check`. Instead of returning true to
    allow the action or false to deny, a MCUmgr result code needs to be
    returned, :c:enumerator:`MGMT_ERR_EOK` will allow the action, any other
    return code will disallow it and return that code to the client
    (:c:enumerator:`MGMT_ERR_EACCESSDENIED` can be used for an access denied
    error). :kconfig:option:`CONFIG_MCUMGR_GRP_IMG_UPLOAD_CHECK_HOOK` needs to
    be set.
 * os_mgmt_register_reset_evt_cb
    Using :c:enumerator:`MGMT_EVT_OP_OS_MGMT_RESET`.  Instead of returning
    true to allow the action or false to deny, a MCUmgr result code needs to be
    returned, :c:enumerator:`MGMT_ERR_EOK` will allow the action, any other
    return code will disallow it and return that code to the client
    (:c:enumerator:`MGMT_ERR_EACCESSDENIED` can be used for an access denied
    error). :kconfig:option:`CONFIG_MCUMGR_SMP_COMMAND_STATUS_HOOKS` needs to
    be set.

API Reference
*************

.. doxygengroup:: mcumgr_callback_api
