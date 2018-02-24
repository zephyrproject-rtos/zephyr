## Building and using mcumgr with Apache Mynewt

NOTE: The *mcumgr* library consists of functionality that is already present in
the `apache-mynewt-core` repo.  There is currently no need to use the external
*mcumgr* library with Mynewt, as the functionality is already built in to the
OS.  To use this library with a Mynewt application, you will need to remove the
duplicate functionality from your copy of the `apache-mynewt-core` repo.

### Configuration

To use *mcumgr*, your Mynewt app needs to be configured to use:
1. An mcumgr transfer encoding
2. An mcumgr transport
3. (optional) Command handlers.

This is done by adding the necessary dependencies to your app or target.  The following list of dependencies adds support for the SMP transfer encoding, the Bluetooth and shell transports, and all the built-in command handlers:

```
    - '@apache-mynewt-core/mgmt/newtmgr/transport/ble'
    - '@apache-mynewt-core/mgmt/newtmgr/transport/nmgr_shell'
    - '@mynewt-mcumgr/cmd/fs_mgmt'
    - '@mynewt-mcumgr/cmd/img_mgmt'
    - '@mynewt-mcumgr/cmd/os_mgmt'
    - '@mynewt-mcumgr/smp'
```

For an example of an app that uses mcumgr, see the `smp_svr` sample app in `samples/smp_svr/mynewt`.

### Building

With the necessary dependencies in place, your project can be built using the usual `newt build <target-name>` or `newt run <target-name>`
