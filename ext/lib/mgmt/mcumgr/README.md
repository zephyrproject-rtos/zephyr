# mcumgr

This is mcumgr, version 0.0.1

mcumgr is a management library for 32-bit MCUs.   The goal of mcumgr is to
define a common management infrastructure with pluggable transport and encoding
components.  In addition, mcumgr provides definitions and handlers for some
core commands: image management, file system management, and OS managment.

mcumgr is operating system and hardware independent.  It relies on hardware
porting layers from the operating system it runs on.  Currently, mcumgr runs on
both the Apache Mynewt and Zephyr operating systems.

## Getting started

For tips on using mcumgr with your particular OS, see the appropriate file from
the list below:

* [Mynewt](README-mynewt.md)
* [Zephyr](README-zephyr.md)

## Dependencies

To use mcumgr's image management support, your device must be running version
1.1.0 or later of the [MCUboot boot
loader](https://github.com/runtimeco/mcuboot).  The other mcumgr features do
not require MCUboot.

## Command line tool

The `mcumgr` command line tool is available at:
https://github.com/apache/mynewt-mcumgr-cli.  The command line tool requires [Go
1.7 or later](https://golang.org/dl/).  Once Go is installed and set up on your
system, you can install the mcumgr CLI tool by issuing the following `go get`
command:

```
$ go get github.com/apache/mynewt-mcumgr-cli/mcumgr
```

The `mcumgr` tool allows you to manage devices running an mcumgr server.

## Architecture

The mcumgr stack has the following layout:

```
+---------------------+---------------------+
|             <command handlers>            |
+---------------------+---------------------+
|                   mgmt                    |
+---------------------+---------------------+
|           <transfer encoding(s)>          |
+---------------------+---------------------+
|               <transport(s)>              |
+---------------------+---------------------+
```

Items enclosed in angled brackets represent generic components that can be plugged into mcumgr.  The items in this stack diagram are defined below:
* *Command handler*: Processes incoming mcumgr requests and generates corresponding responses.  A command handler is associated with a single command type, defined by a (group ID, command ID) pair.
* *mgmt*: The core of mcumgr; facilitates the passing of requests and responses between the generic command handlers and the concrete transports and transfer encodings.
* *Transfer encoding*: Defines how mcumgr requests and responses are encoded on the wire.
* *Transport*: Sends and receives mcumgr packets over a particular medium.

Each transport is configured with a single transfer encoding.

As an example, the sample application `smp_svr` uses the following components:

* Command handlers:
    * Image management (`img_mgmt`)
    * File system management (`fs_mgmt`)
    * Log management (`log_mgmt`)
    * OS management (`os_mgmt`)
* Transfer/Transports protocols:
    * SMP/Bluetooth
    * SMP/Shell

yielding the following stack diagram:

```
+----------+----------+----------+----------+
| img_mgmt |  fs_mgmt | log_mgmt |  os_mgmt |
+----------+----------+----------+----------+
|                   mgmt                    |
+---------------------+---------------------+
|         SMP         |         SMP         |
+---------------------+---------------------+
|      Bluetooth      |        Shell        |
+---------------------+---------------------+
```

## Command definition

An mcumgr request or response consists of the following two components:
* mcumgr header
* CBOR key-value map 

How these two components are encoded and parsed depends on the transfer
encoding used.

The mcumgr header structure is defined in `mgmt/include/mgmt/mgmt.h` as
`struct mgmt_hdr`.

The contents of the CBOR key-value map are specified per command type.

## Supported transfer encodings

Mcumgr comes with one built-in transfer encoding: Simple Management Protocol
(SMP).  SMP requests and responses have a very basic structure.  For details,
see the comments at the top of `smp/include/smp/smp.h`.

## Supported transports

The mcumgr project defines two transports:
    * SMP/Console
    * SMP/Bluetooth

Particulars of these transports are specified in the following documents:
    * SMP/Console: `transports/smp-console.md`
    * SMP/Bluetooth: `transports/smp-bluetooth.md`

Implementations, being hardware- and OS-specified, are not included.

## Browsing

Information and documentation for mcumgr is stored within the source.

For more information in the source, here are some pointers:

- [cborattr](https://github.com/apache/mynewt-mcumgr/tree/master/cborattr): Used for parsing incoming mcumgr requests.  Destructures mcumgr packets and populates corresponding field variables.
- [cmd](https://github.com/apache/mynewt-mcumgr/tree/master/cmd): Built-in command handlers for the core mcumgr commands.
- [ext](https://github.com/apache/mynewt-mcumgr/tree/master/ext): Third-party libraries that mcumgr depends on.
- [mgmt](https://github.com/apache/mynewt-mcumgr/tree/master/mgmt): Code implementing the `mgmt` layer of mcumgr.
- [samples](https://github.com/apache/mynewt-mcumgr/tree/master/samples): Sample applications utilizing mcumgr.
- [smp](https://github.com/apache/mynewt-mcumgr/tree/master/smp): The built-in transfer encoding: Simple management protocol.

## Joining

Developers welcome!

* Our Slack channel: https://mynewt.slack.com/messages/C7Y3K0C2J
