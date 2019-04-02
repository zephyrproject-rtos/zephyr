.. _settings:

Settings
########

The settings subsystem gives modules a way to store persistent
per-device configuration and runtime state.

Settings items are stored as key-value pair strings.  By convention,
the keys can be organized by the package and subtree defining the key,
for example the key ``id/serial`` would define the ``serial`` configuration
element for the package ``id``.

Convenience routines are provided for converting a key value to
and from a string type.

Handlers
********

Settings handlers for subtree implement a set of handler functions.
These are registered using a call to ``settings_register()``.

**h_get**
    This gets called when asking for a settings element value by its name using
    ``settings_runtime_get()`` from the runtime backend.

**h_set**
    This gets called when the value is loaded from persisted storage with
    ``settings_load()``, or when using ``settings_runtime_set()`` from the
    runtime backend.

**h_commit**
    This gets called after the settings have been loaded in full.
    Sometimes you don't want an individual setting value to take
    effect right away, for example if there are multiple settings
    which are interdependent.

**h_export**
    This gets called to write all current settings. This happens
    when ``settings_save()`` tries to save the settings or transfer to any
    user-implemented back-end.

Backends
********

Backends are meant to load and save data to/from setting handlers, and
implement a set of handler functions. These are registered using a call to
``settings_src_register()`` for backends that can load data, and/or
``settings_dst_register()`` for backends that can save data. The current
implementation allows for multiple source backends but only a single destination
backend.

**csi_load**
    This gets called when loading values from persistent storage using
    ``settings_load()``.

**csi_save**
    This gets called when a saving a single setting to persistent storage using
    ``settings_save_one()``.

**csi_save_start**
    This gets called when starting a save of all current settings using
    ``settings_save()``.

**csi_save_end**
    This gets called after having saved of all current settings using
    ``settings_save()``.

Zephyr Storage Backends
***********************

Zephyr has two existing backend storages which can be a Flash Circular Buffer
(:option:`CONFIG_SETTINGS_FCB`) or a file in the filesystem
(:option:`CONFIG_SETTINGS_FS`).

You can declare multiple sources for settings; settings from
all of these are restored when ``settings_load()`` is called.

There can be only one target for writing settings; this is where
data is stored when you call ``settings_save()``, or ``settings_save_one()``.

FCB read target is registered using ``settings_fcb_src()``, and write target
using ``settings_fcb_dst()``. As a side-effect,  ``settings_fcb_src()``
initializes the FCB area, so it must be called before calling
``settings_fcb_dst()``. File read target is registered using
``settings_file_src()``, and write target by using ``settings_file_dst()``.

Loading data from persisted storage
***********************************

A call to ``settings_load()`` uses an ``h_set`` implementation
to load settings data from storage to volatile memory.
For both FCB and filesystem back-end the most
recent key values are guaranteed by traversing all stored content
and (potentially) overwriting older key values with newer ones.
After all data is loaded, the ``h_commit`` handler is issued,
signalling the application that the settings were successfully
retrieved.

Example: Device Configuration
*****************************

This is a simple example, where the settings handler only implements ``h_set``
and ``h_export``. ``h_set`` is called when the value is restored from storage
(or when set initially), and ``h_export`` is used to write the value to
storage thanks to ``storage_func()``. The user can also implement some other
export functionality, for example, writing to the shell console).

.. code-block:: c

    static int8 foo_val;

    struct settings_handler my_conf = {
        .name = "foo",
        .h_set = foo_settings_set,
        .h_export = foo_settings_export
    };

    static int foo_settings_set(int argc, char **argv, settings_read_cb read_cb,
                                void *cb_arg)
    {
        if (argc == 1) {
            if (!strcmp(argv[0], "bar")) {
                return read_cb(cb_arg, &foo_val, sizeof(foo_val));
            }
        }

        return -ENOENT;
    }

    static int foo_settings_export(int (*storage_func)(const char *name,
                                                       void *value,
                                                       size_t val_len))
    {
        return storage_func("foo/bar", &foo_val, sizeof(foo_val));
    }

Example: Persist Runtime State
******************************

This is a simple example showing how to persist runtime state. In this example,
only ``h_set`` is defined, which is used when restoring value from
persisted storage.

In this example, the ``foo_callout`` function increments ``foo_val``, and then
persists the latest number. When the system restarts, the application calls
``settings_load()`` while initializing, and ``foo_val`` will continue counting
up from where it was before restart.

.. code-block:: c

    static int8 foo_val;

    struct settings_handler my_conf = {
        .name = "foo",
        .h_set = foo_settings_set
    };

    static int foo_settings_set(int argc, char **argv, settings_read_cb read_cb,
                                void *cb_arg)
    {
        if (argc == 1) {
            if (!strcmp(argv[0], "bar")) {
                return read_cb(cb_arg, &foo_val, sizeof(foo_val));
            }
        }

        return -ENOENT;
    }

    static void foo_callout(struct os_event *ev)
    {
        struct os_callout *c = (struct os_callout *)ev;

        foo_val++;
        settings_save_one("foo/bar", &foo_val, sizeof(foo_val));

        k_sleep(1000);
        sys_reboot(SYS_REBOOT_COLD);
    }

Example: Custom Backend Implementation
**************************************

This is a simple example showing how to register a simple custom backend
handler (:option:`CONFIG_SETTINGS_CUSTOM`).

.. code-block:: c

    static int settings_custom_load(struct settings_store *cs)
    {
        //...
    }

    static int settings_custom_save(struct settings_store *cs, const char *name,
                                    const char *value, size_t val_len)
    {
        //...
    }

    static struct settings_store_itf settings_custom_itf = {
        .csi_load = settings_custom_load,
        .csi_save = settings_custom_save,
    };

    int settings_backend_init(void)
    {
        settings_dst_register(&settings_custom_itf);
        settings_src_register(&settings_custom_itf);
        return 0;
    }

API Reference
*************

The Settings subsystem APIs are provided by ``settings.h``:

.. doxygengroup:: settings
   :project: Zephyr

