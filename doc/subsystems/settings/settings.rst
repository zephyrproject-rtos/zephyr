.. _settings:

Settings subsystem with non-volatile storage
############################################

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
    This gets called when asking for a settings element value
    by its name using ``settings_get_value()``.

**h_set**
    This gets called when the value is being set using ``settings_set_value()``,
    and also when setting is loaded from persisted storage with
    ``settings_load()``.

**h_commit**
    This gets called after the settings have been loaded in full.
    Sometimes you don't want an individual setting value to take
    effect right away, for example if there are multiple settings
    which are interdependent.

**h_export**
    This gets called to write all current settings. This happens
    when ``settings_save()`` tries to save the settings or transfer to any
    user-implemented back-end.

Persistence
***********

Backend storage for the settings can be either FCB, a file in the filesystem,
or both.

You can declare multiple sources for settings; settings from
all of these are restored when ``settings_load()`` is called.

There can be only one target for writing settings; this is where
data is stored when you call ``settings_save()``, or ``settings_save_one()``.

FCB read target is registered using ``settings_fcb_src()``, and write target
using ``settings_fcb_dst()``. As a side-effect,  ``settings_fcb_src()``
initializes the FCB area, so it must be called before calling
``settings_fcb_dst()``. File read target is registered using
``settings_file_src()``, and write target by using ``settings_file_dst()``.

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

    static int foo_settings_set(int argc, char **argv, char *val)
    {
        if (argc == 1) {
            if (!strcmp(argv[0], "bar")) {
                return SETTINGS_VALUE_SET(val, SETTINGS_INT8, foo_val);
            }
        }

        return -ENOENT;
    }

    static int foo_settings_export(void (*storage_func)(char *name, char *val),
                               enum settings_export_tgt tgt)
    {
        char buf[4];

        settings_str_from_value(SETTINGS_INT8, &foo_val, buf, sizeof(buf));
        storage_func("foo/bar", buf)

        return 0;
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

    static int foo_settings_set(int argc, char **argv, char *val)
    {
        if (argc == 1) {
            if (!strcmp(argv[0], "bar")) {
                return SETTINGS_VALUE_SET(val, SETTINGS_INT8, foo_val);
            }
        }

        return -ENOENT;
    }

    static void foo_callout(struct os_event *ev)
    {
        struct os_callout *c = (struct os_callout *)ev;
        char buf[4];

        foo_val++;
        settings_str_from_value(SETTINGS_INT8, &foo_val, buf, sizeof(buf));
        settings_save_one("foo/bar", bar);

        k_sleep(1000);
        sys_reboot(SYS_REBOOT_COLD);
    }

API
***

The Settings subsystem APIs are provided by ``settings.h``:

.. doxygengroup:: settings
   :project: Zephyr

