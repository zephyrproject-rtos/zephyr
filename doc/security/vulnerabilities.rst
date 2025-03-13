.. _vulnerabilities:

Vulnerabilities
###############

This page collects all of the vulnerabilities that are discovered and
fixed in each release.  It will also often have more details than is
available in the releases.  Some vulnerabilities are deemed to be
sensitive, and will not be publicly discussed until there is
sufficient time to fix them.  Because the release notes are locked to
a version, the information here can be updated after the embargo is
lifted.

CVE-2017
========

:cve:`2017-14199`
-----------------

Buffer overflow in :code:`getaddrinfo()`.

- `Zephyr project bug tracker ZEPSEC-12
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-12>`_

- `PR6158 fix for 1.11.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/6158>`_

:cve:`2017-14201`
-----------------

The shell DNS command can cause unpredictable results due to misuse of
stack variables.

Use After Free vulnerability in the Zephyr shell allows a serial or
telnet connected user to cause denial of service, and possibly remote
code execution.

This has been fixed in release v1.14.0.

- `Zephyr project bug tracker ZEPSEC-17
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-17>`_

- `PR13260 fix for v1.14.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/13260>`_

:cve:`2017-14202`
-----------------

The shell implementation does not protect against buffer overruns
resulting in unpredictable behavior.

Improper Restriction of Operations within the Bounds of a Memory
Buffer vulnerability in the shell component of Zephyr allows a serial
or telnet connected user to cause a crash, possibly with arbitrary
code execution.

This has been fixed in release v1.14.0.

- `Zephyr project bug tracker ZEPSEC-18
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-18>`_

- `PR13048 fix for v1.14.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/13048>`_

CVE-2019
========

:cve:`2019-9506`
----------------

The Bluetooth BR/EDR specification up to and including version 5.1
permits sufficiently low encryption key length and does not prevent an
attacker from influencing the key length negotiation. This allows
practical brute-force attacks (aka "KNOB") that can decrypt traffic
and inject arbitrary ciphertext without the victim noticing.

- `Zephyr project bug tracker ZEPSEC-20
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-20>`_

- `PR18702 fix for v1.14.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/18702>`_

- `PR18659 fix for v2.0.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/18659>`_

CVE-2020
========

:cve:`2020-10019`
-----------------

Buffer Overflow vulnerability in USB DFU of zephyr allows a USB
connected host to cause possible remote code execution.

This has been fixed in releases v1.14.2, v2.2.0, and v2.1.1.

- `Zephyr project bug tracker ZEPSEC-25
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-25>`_

- `PR23460 fix for 1.14.x
  <https://github.com/zephyrproject-rtos/zephyr/pull/23460>`_

- `PR23457 fix for 2.1.x
  <https://github.com/zephyrproject-rtos/zephyr/pull/23457>`_

- `PR23190 fix in 2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23190>`_

:cve:`2020-10021`
-----------------

Out-of-bounds write in USB Mass Storage with unaligned sizes

Out-of-bounds Write in the USB Mass Storage memoryWrite handler with
unaligned Sizes.

See NCC-ZEP-024, NCC-ZEP-025, NCC-ZEP-026

This has been fixed in releases v1.14.2, and v2.2.0.

- `Zephyr project bug tracker ZEPSEC-26
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-26>`_

- `PR23455 fix for v1.14.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/23455>`_

- `PR23456 fix for the v2.1 branch
  <https://github.com/zephyrproject-rtos/zephyr/pull/23456>`_

- `PR23240 fix for v2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23240>`_

:cve:`2020-10022`
-----------------

UpdateHub Module Copies a Variable-Size Hash String Into a Fixed-Size Array

A malformed JSON payload that is received from an UpdateHub server may
trigger memory corruption in the Zephyr OS. This could result in a
denial of service in the best case, or code execution in the worst
case.

See NCC-ZEP-016

This has been fixed in the below pull requests for main, branch from
v2.1.0, and branch from v2.2.0.

- `Zephyr project bug tracker ZEPSEC-28
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-28>`_

- `PR24154 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/24154>`_

- `PR24065 fix for branch from v2.1.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/24065>`_

- `PR24066 fix for branch from v2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/24066>`_

:cve:`2020-10023`
-----------------

Shell Subsystem Contains a Buffer Overflow Vulnerability In
shell_spaces_trim

The shell subsystem contains a buffer overflow, whereby an adversary
with physical access to the device is able to cause a memory
corruption, resulting in denial of service or possibly code execution
within the Zephyr kernel.

See NCC-ZEP-019

This has been fixed in releases v1.14.2, v2.2.0, and in a branch from
v2.1.0,

- `Zephyr project bug tracker ZEPSEC-29
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-29>`_

- `PR23646 fix for v1.14.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/23646>`_

- `PR23649 fix for branch from v2.1.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23649>`_

- `PR23304 fix for v2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23304>`_

:cve:`2020-10024`
-----------------

ARM Platform Uses Signed Integer Comparison When Validating Syscall
Numbers

The arm platform-specific code uses a signed integer comparison when
validating system call numbers. An attacker who has obtained code
execution within a user thread is able to elevate privileges to that
of the kernel.

See NCC-ZEP-001

This has been fixed in releases v1.14.2, and v2.2.0, and in a branch
from v2.1.0,

- `Zephyr project bug tracker ZEPSEC-30
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-30>`_

- `PR23535 fix for v1.14.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/23535>`_

- `PR23498 fix for branch from v2.1.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23498>`_

- `PR23323 fix for v2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23323>`_

:cve:`2020-10027`
-----------------

ARC Platform Uses Signed Integer Comparison When Validating Syscall
Numbers

An attacker who has obtained code execution within a user thread is
able to elevate privileges to that of the kernel.

See NCC-ZEP-001

This has been fixed in releases v1.14.2, and v2.2.0, and in a branch
from v2.1.0.

- `Zephyr project bug tracker ZEPSEC-35
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-35>`_

- `PR23500 fix for v1.14.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/23500>`_

- `PR23499 fix for branch from v2.1.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23499>`_

- `PR23328 fix for v2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23328>`_

:cve:`2020-10028`
-----------------

Multiple Syscalls In GPIO Subsystem Performs No Argument Validation

Multiple syscalls with insufficient argument validation

See NCC-ZEP-006

This has been fixed in releases v1.14.2, and v2.2.0, and in a branch
from v2.1.0.

- `Zephyr project bug tracker ZEPSEC-32
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-32>`_

- `PR23733 fix for v1.14.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/23733>`_

- `PR23737 fix for branch from v2.1.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23737>`_

- `PR23308 fix for v2.2.0 (gpio patch)
  <https://github.com/zephyrproject-rtos/zephyr/pull/23308>`_

:cve:`2020-10058`
-----------------

Multiple Syscalls In kscan Subsystem Performs No Argument Validation

Multiple syscalls in the Kscan subsystem perform insufficient argument
validation, allowing code executing in userspace to potentially gain
elevated privileges.

See NCC-ZEP-006

This has been fixed in a branch from v2.1.0, and release v2.2.0.

- `Zephyr project bug tracker ZEPSEC-34
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-34>`_

- `PR23748 fix for branch from v2.1.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23748>`_

- `PR23308 fix for v2.2.0 (kscan patch)
  <https://github.com/zephyrproject-rtos/zephyr/pull/23308>`_

:cve:`2020-10059`
-----------------

UpdateHub Module Explicitly Disables TLS Verification

The UpdateHub module disables DTLS peer checking, which allows for a
man in the middle attack. This is mitigated by firmware images
requiring valid signatures. However, there is no benefit to using DTLS
without the peer checking.

See NCC-ZEP-018

This has been fixed in a PR against Zephyr main.

- `Zephyr project bug tracker ZEPSEC-36
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-36>`_

- `PR24954 fix on main (to be fixed in v2.3.0)
  <https://github.com/zephyrproject-rtos/zephyr/pull/24954>`_

- `PR24954 fix v2.1.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/24999>`_

- `PR24954 fix v2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/24997>`_

:cve:`2020-10060`
-----------------

UpdateHub Might Dereference An Uninitialized Pointer

In updatehub_probe, right after JSON parsing is complete, objects\[1]
is accessed from the output structure in two different places. If the
JSON contained less than two elements, this access would reference
uninitialized stack memory. This could result in a crash, denial of
service, or possibly an information leak.

Recommend disabling updatehub until such a time as a fix can be made
available.

See NCC-ZEP-030

This has been fixed in a PR against Zephyr main.

- `Zephyr project bug tracker ZEPSEC-37
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-37>`_

- `PR27865 fix on main (to be fixed in v2.4.0)
  <https://github.com/zephyrproject-rtos/zephyr/pull/27865>`_

- `PR27865 fix for v2.3.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/27889>`_

- `PR27865 fix for v2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/27891>`_

- `PR27865 fix for v2.1.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/27893>`_

:cve:`2020-10061`
-----------------

Error handling invalid packet sequence

Improper handling of the full-buffer case in the Zephyr Bluetooth
implementation can result in memory corruption.

This has been fixed in branches for v1.14.0, v2.2.0, and will be
included in v2.3.0.

- `Zephyr project bug tracker ZEPSEC-75
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-75>`_

- `PR23516 fix for v2.3 (split driver)
  <https://github.com/zephyrproject-rtos/zephyr/pull/23516>`_

- `PR23517 fix for v2.3 (legacy driver)
  <https://github.com/zephyrproject-rtos/zephyr/pull/23517>`_

- `PR23091 fix for branch from v1.14.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23091>`_

- `PR23547 fix for branch from v2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23547>`_

:cve:`2020-10062`
-----------------

Packet length decoding error in MQTT

CVE: An off-by-one error in the Zephyr project MQTT packet length
decoder can result in memory corruption and possible remote code
execution. NCC-ZEP-031

The MQTT packet header length can be 1 to 4 bytes. An off-by-one error
in the code can result in this being interpreted as 5 bytes, which can
cause an integer overflow, resulting in memory corruption.

This has been fixed in main for v2.3.

- `Zephyr project bug tracker ZEPSEC-84
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-84>`_

- `commit 11b7a37d for v2.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/23821/commits/11b7a37d9a0b438270421b224221d91929843de4>`_

- `NCC-ZEP report`_ (NCC-ZEP-031)

.. _NCC-ZEP report: https://research.nccgroup.com/2020/05/26/research-report-zephyr-and-mcuboot-security-assessment

:cve:`2020-10063`
-----------------

Remote Denial of Service in CoAP Option Parsing Due To Integer
Overflow

A remote adversary with the ability to send arbitrary CoAP packets to
be parsed by Zephyr is able to cause a denial of service.

This has been fixed in main for v2.3.

- `Zephyr project bug tracker ZEPSEC-55
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-55>`_

- `PR24435 fix in main for v2.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/24435>`_

- `PR24531 fix for branch from v2.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/24531>`_

- `PR24535 fix for branch from v2.1
  <https://github.com/zephyrproject-rtos/zephyr/pull/24535>`_

- `PR24530 fix for branch from v1.14
  <https://github.com/zephyrproject-rtos/zephyr/pull/24530>`_

- `NCC-ZEP report`_ (NCC-ZEP-032)

:cve:`2020-10064`
-----------------

Improper Input Frame Validation in ieee802154 Processing

- `Zephyr project bug tracker ZEPSEC-65
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-65>`_

- `PR24971 fix for v2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/24971>`_

- `PR33451 fix for v1.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/33451>`_

:cve:`2020-10065`
-----------------

OOB Write after not validating user-supplied length (<= 0xffff) and
copying to fixed-size buffer (default: 77 bytes) for HCI_ACL packets in
bluetooth HCI over SPI driver.

- `Zephyr project bug tracker ZEPSEC-66
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-66>`_

- This issue has not been fixed.

:cve:`2020-10066`
-----------------

Incorrect Error Handling in Bluetooth HCI core

In hci_cmd_done, the buf argument being passed as null causes
nullpointer dereference.

- `Zephyr project bug tracker ZEPSEC-67
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-67>`_

- `PR24902 fix for v2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/24902>`_

- `PR25089 fix for v1.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/25089>`_

:cve:`2020-10067`
-----------------

Integer Overflow In is_in_region Allows User Thread To Access Kernel Memory

A malicious userspace application can cause a integer overflow and
bypass security checks performed by system call handlers. The impact
would depend on the underlying system call and can range from denial
of service to information leak to memory corruption resulting in code
execution within the kernel.

See NCC-ZEP-005

This has been fixed in releases v1.14.2, and v2.2.0.

- `Zephyr project bug tracker ZEPSEC-27
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-27>`_

- `PR23653 fix for v1.14.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/23653>`_

- `PR23654 fix for the v2.1 branch
  <https://github.com/zephyrproject-rtos/zephyr/pull/23654>`_

- `PR23239 fix for v2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23239>`_

:cve:`2020-10068`
-----------------

Zephyr Bluetooth DLE duplicate requests vulnerability

In the Zephyr project Bluetooth subsystem, certain duplicate and
back-to-back packets can cause incorrect behavior, resulting in a
denial of service.

This has been fixed in branches for v1.14.0, v2.2.0, and will be
included in v2.3.0.

- `Zephyr project bug tracker ZEPSEC-78
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-78>`_

- `PR23707 fix for v2.3 (split driver)
  <https://github.com/zephyrproject-rtos/zephyr/pull/23707>`_

- `PR23708 fix for v2.3 (legacy driver)
  <https://github.com/zephyrproject-rtos/zephyr/pull/23708>`_

- `PR23091 fix for branch from v1.14.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23091>`_

- `PR23964 fix for v2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23964>`_

:cve:`2020-10069`
-----------------

Zephyr Bluetooth unchecked packet data results in denial of service

An unchecked parameter in bluetooth data can result in an assertion
failure, or division by zero, resulting in a denial of service attack.

This has been fixed in branches for v1.14.0, v2.2.0, and will be
included in v2.3.0.

- `Zephyr project bug tracker ZEPSEC-81
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-81>`_

- `PR23705 fix for v2.3 (split driver)
  <https://github.com/zephyrproject-rtos/zephyr/pull/23705>`_

- `PR23706 fix for v2.3 (legacy driver)
  <https://github.com/zephyrproject-rtos/zephyr/pull/23706>`_

- `PR23091 fix for branch from v1.14.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23091>`_

- `PR23963 fix for branch from v2.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/23963>`_

:cve:`2020-10070`
-----------------

MQTT buffer overflow on receive buffer

In the Zephyr Project MQTT code, improper bounds checking can result
in memory corruption and possibly remote code execution.  NCC-ZEP-031

When calculating the packet length, arithmetic overflow can result in
accepting a receive buffer larger than the available buffer space,
resulting in user data being written beyond this buffer.

This has been fixed in main for v2.3.

- `Zephyr project bug tracker ZEPSEC-85
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-85>`_

- `commit 0b39cbf3 for v2.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/23821/commits/0b39cbf3c01d7feec9d0dd7cc7e0e374b6113542>`_

- `NCC-ZEP report`_ (NCC-ZEP-031)

:cve:`2020-10071`
-----------------

Insufficient publish message length validation in MQTT

The Zephyr MQTT parsing code performs insufficient checking of the
length field on publish messages, allowing a buffer overflow and
potentially remote code execution. NCC-ZEP-031

This has been fixed in main for v2.3.

- `Zephyr project bug tracker ZEPSEC-86
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-86>`_

- `commit 989c4713 fix for v2.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/23821/commits/989c4713ba429aa5105fe476b4d629718f3e6082>`_

- `NCC-ZEP report`_ (NCC-ZEP-031)

:cve:`2020-10072`
-----------------

All threads can access all socket file descriptors

There is no management of permissions to network socket API file
descriptors. Any thread running on the system may read/write a socket
file descriptor knowing only the numerical value of the file
descriptor.

- `Zephyr project bug tracker ZEPSEC-87
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-87>`_

- `PR25804 fix for v2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/25804>`_

- `PR27176 fix for v1.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/27176>`_

:cve:`2020-10136`
-----------------

IP-in-IP protocol routes arbitrary traffic by default zephyrproject

- `Zephyr project bug tracker ZEPSEC-64
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-64>`_

:cve:`2020-13598`
-----------------

FS: Buffer Overflow when enabling Long File Names in FAT_FS and calling fs_stat

Performing fs_stat on a file with a filename longer than 12
characters long will cause a buffer overflow.

- `Zephyr project bug tracker ZEPSEC-88
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-88>`_

- `PR25852 fix for v2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/25852>`_

- `PR28782 fix for v2.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/28782>`_

- `PR33577 fix for v1.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/33577>`_

:cve:`2020-13599`
-----------------

Security problem with settings and littlefs

When settings is used in combination with littlefs all security
related information can be extracted from the device using MCUmgr and
this could be used e.g in bt-mesh to get the device key, network key,
app keys from the device.

- `Zephyr project bug tracker ZEPSEC-57
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-57>`_

- `PR26083 fix for v2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/26083>`_

:cve:`2020-13600`
-----------------

Malformed SPI in response for eswifi can corrupt kernel memory


- `Zephyr project bug tracker ZEPSEC-91
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-91>`_

- `PR26712 fix for v2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/26712>`_

:cve:`2020-13601`
-----------------

Possible read out of bounds in dns read

- `Zephyr project bug tracker ZEPSEC-92
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-92>`_

- `PR27774 fix for v2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/27774>`_

- `PR30503 fix for v1.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/30503>`_

:cve:`2020-13602`
-----------------

Remote Denial of Service in LwM2M do_write_op_tlv

In the Zephyr LwM2M implementation, malformed input can result in an
infinite loop, resulting in a denial of service attack.

- `Zephyr project bug tracker ZEPSEC-56
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-56>`_

- `PR26571 fix for v2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/26571>`_

- `PR33578 fix for v1.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/33578>`_

:cve:`2020-13603`
-----------------

Possible overflow in mempool

 * Zephyr offers pre-built 'malloc' wrapper function instead.
 * The 'malloc' function is wrapper for the 'sys_mem_pool_alloc' function
 * sys_mem_pool_alloc allocates 'size + WB_UP(sizeof(struct sys_mem_pool_block))' in an unsafe manner.
 * Asking for very large size values leads to internal integer wrap-around.
 * Integer wrap-around leads to successful allocation of very small memory.
 * For example: calling malloc(0xffffffff) leads to successful allocation of 7 bytes.
 * That leads to heap overflow.

- `Zephyr project bug tracker ZEPSEC-111
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-111>`_

- `PR31796 fix for v2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/31796>`_

- `PR32808 fix for v1.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/26571>`_

CVE-2021
========

:cve:`2021-3319`
----------------

DOS: Incorrect 802154 Frame Validation for Omitted Source / Dest Addresses

Improper processing of omitted source and destination addresses in
ieee802154 frame validation (ieee802154_validate_frame)

This has been fixed in main for v2.5.0

- `Zephyr project bug tracker GHSA-94jg-2p6q-5364
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-94jg-2p6q-5364>`_

- `PR31908 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/31908>`_

:cve:`2021-3320`
----------------
Mismatch between validation and handling of 802154 ACK frames, where
ACK frames are considered during validation, but not during actual
processing, leading to a type confusion.

- `PR31908 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/31908>`_

:cve:`2021-3321`
----------------

Incomplete check of minimum IEEE 802154 fragment size leading to an
integer underflow.

- `Zephyr project bug tracker ZEPSEC-114
  <https://zephyrprojectsec.atlassian.net/browse/ZEPSEC-114>`_

- `PR33453 fix for v2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/33453>`_

:cve:`2021-3323`
----------------

Integer Underflow in 6LoWPAN IPHC Header Uncompression

This has been fixed in main for v2.5.0

- `Zephyr project bug tracker GHSA-89j6-qpxf-pfpc
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-89j6-qpxf-pfpc>`_

- `PR 31971 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/31971>`_

:cve:`2021-3430`
----------------

Assertion reachable with repeated LL_CONNECTION_PARAM_REQ.

This has been fixed in main for v2.6.0

- `Zephyr project bug tracker GHSA-46h3-hjcq-2jjr
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-46h3-hjcq-2jjr>`_

- `PR 33272 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/33272>`_

- `PR 33369 fix for 2.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/33369>`_

- `PR 33759 fix for 1.14.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/33759>`_

:cve:`2021-3431`
----------------

BT: Assertion failure on repeated LL_FEATURE_REQ

This has been fixed in main for v2.6.0

- `Zephyr project bug tracker GHSA-7548-5m6f-mqv9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-7548-5m6f-mqv9>`_

- `PR 33340 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/33340>`_

- `PR 33369 fix for 2.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/33369>`_

:cve:`2021-3432`
----------------

Invalid interval in CONNECT_IND leads to Division by Zero

This has been fixed in main for v2.6.0

- `Zephyr project bug tracker GHSA-7364-p4wc-8mj4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-7364-p4wc-8mj4>`_

- `PR 33278 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/33278>`_

- `PR 33369 fix for 2.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/33369>`_

:cve:`2021-3433`
----------------

BT: Invalid channel map in CONNECT_IND results to Deadlock

This has been fixed in main for v2.6.0

- `Zephyr project bug tracker GHSA-3c2f-w4v6-qxrp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3c2f-w4v6-qxrp>`_

- `PR 33278 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/33278>`_

- `PR 33369 fix for 2.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/33369>`_

:cve:`2021-3434`
----------------

L2CAP: Stack based buffer overflow in le_ecred_conn_req()

This has been fixed in main for v2.6.0

- `Zephyr project bug tracker GHSA-8w87-6rfp-cfrm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8w87-6rfp-cfrm>`_

- `PR 33305 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/33305>`_

- `PR 33419 fix for 2.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/33419>`_

- `PR 33418 fix for 1.14.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/33418>`_

:cve:`2021-3435`
----------------

L2CAP: Information leakage in le_ecred_conn_req()

This has been fixed in main for v2.6.0

- `Zephyr project bug tracker GHSA-xhg3-gvj6-4rqh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-xhg3-gvj6-4rqh>`_

- `PR 33305 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/33305>`_

- `PR 33419 fix for 2.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/33419>`_

- `PR 33418 fix for 1.14.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/33418>`_

:cve:`2021-3436`
----------------

Bluetooth: Possible to overwrite an existing bond during keys
distribution phase when the identity address of the bond is known

During the distribution of the identity address information we don’t
check for an existing bond with the same identity address.This means
that a duplicate entry will be created in RAM while the newest entry
will overwrite the existing one in persistent storage.

This has been fixed in main for v2.6.0

- `Zephyr project bug tracker GHSA-j76f-35mc-4h63
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-j76f-35mc-4h63>`_

- `PR 33266 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/33266>`_

- `PR 33432 fix for 2.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/33432>`_

- `PR 33433 fix for 2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/33433>`_

- `PR 33718 fix for 1.14.2
  <https://github.com/zephyrproject-rtos/zephyr/pull/33718>`_

:cve:`2021-3454`
----------------

Truncated L2CAP K-frame causes assertion failure

For example, sending L2CAP K-frame where SDU length field is truncated
to only one byte, causes assertion failure in previous releases of
Zephyr. This has been fixed in master by commit 0ba9437 but has not
yet been backported to older release branches.

This has been fixed in main for v2.6.0

- `Zephyr project bug tracker GHSA-fx88-6c29-vrp3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fx88-6c29-vrp3>`_

- `PR 32588 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/32588>`_

- `PR 33513 fix for 2.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/33513>`_

- `PR 33514 fix for 2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/33514>`_

:cve:`2021-3455`
----------------

Disconnecting L2CAP channel right after invalid ATT request leads freeze

When Central device connects to peripheral and creates L2CAP
connection for Enhanced ATT, sending some invalid ATT request and
disconnecting immediately causes freeze.

This has been fixed in main for v2.6.0

- `Zephyr project bug tracker GHSA-7g38-3x9v-v7vp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-7g38-3x9v-v7vp>`_

- `PR 35597 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/35597>`_

- `PR 36104 fix for 2.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/36104>`_

- `PR 36105 fix for 2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/36105>`_

:cve:`2021-3510`
----------------

Zephyr JSON decoder incorrectly decodes array of array

When using JSON_OBJ_DESCR_ARRAY_ARRAY, the subarray is has the token
type JSON_TOK_LIST_START, but then assigns to the object part of the
union. arr_parse then takes the offset of the array-object (which has
nothing todo with the list) treats it as relative to the parent
object, and stores the length of the subarray in there.

This has been fixed in main for v2.7.0

- `Zephyr project bug tracker GHSA-289f-7mw3-2qf4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-289f-7mw3-2qf4>`_

- `PR 36340 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/36340>`_

- `PR 37816 fix for 2.6
  <https://github.com/zephyrproject-rtos/zephyr/pull/37816>`_

:cve:`2021-3581`
----------------

HCI data not properly checked leads to memory overflow in the Bluetooth stack

In the process of setting SCAN_RSP through the HCI command, the Zephyr
Bluetooth protocol stack did not effectively check the length of the
incoming HCI data. Causes memory overflow, and then the data in the
memory is overwritten, and may even cause arbitrary code execution.

This has been fixed in main for v2.6.0

- `Zephyr project bug tracker GHSA-8q65-5gqf-fmw5
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8q65-5gqf-fmw5>`_

- `PR 35935 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/35935>`_

- `PR 35984 fix for 2.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/35984>`_

- `PR 35985 fix for 2.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/35985>`_

- `PR 35985 fix for 1.14
  <https://github.com/zephyrproject-rtos/zephyr/pull/35985>`_

:cve:`2021-3625`
----------------

Buffer overflow in Zephyr USB DFU DNLOAD

This has been fixed in main for v2.6.0

- `Zephyr project bug tracker GHSA-c3gr-hgvr-f363
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-c3gr-hgvr-f363>`_

- `PR 36694 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/36694>`_

:cve:`2021-3835`
----------------

Buffer overflow in Zephyr USB device class

This has been fixed in main for v3.0.0

- `Zephyr project bug tracker GHSA-fm6v-8625-99jf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fm6v-8625-99jf>`_

- `PR 42093 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/42093>`_

- `PR 42167 fix for 2.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/42167>`_

:cve:`2021-3861`
----------------

Buffer overflow in the RNDIS USB device class

This has been fixed in main for v3.0.0

- `Zephyr project bug tracker GHSA-hvfp-w4h8-gxvj
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hvfp-w4h8-gxvj>`_

- `PR 39725 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/39725>`_

:cve:`2021-3966`
----------------

Usb bluetooth device ACL read cb buffer overflow

This has been fixed in main for v3.0.0

- `Zephyr project bug tracker GHSA-hfxq-3w6x-fv2m
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hfxq-3w6x-fv2m>`_

- `PR 42093 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/42093>`_

- `PR 42167 fix for v2.7.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/42167>`_

CVE-2022
========

:cve:`2022-0553`
----------------

Possible to retrieve unencrypted firmware image

This has been fixed in main for v3.0.0

- `Zephyr project bug tracker GHSA-wrj2-9vj9-rrcp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-wrj2-9vj9-rrcp>`_

- `PR 42424 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/42424>`_

:cve:`2022-1041`
----------------

Out-of-bound write vulnerability in the Bluetooth Mesh core stack can be triggered during provisioning

This has been fixed in main for v3.1.0

- `Zephyr project bug tracker GHSA-p449-9hv9-pj38
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p449-9hv9-pj38>`_

- `PR 45136 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/45136>`_

- `PR 45188 fix for v3.0.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/45188>`_

- `PR 45187 fix for v2.7.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/45187>`_

:cve:`2022-1042`
----------------

Out-of-bound write vulnerability in the Bluetooth Mesh core stack can be triggered during provisioning

This has been fixed in main for v3.1.0

- `Zephyr project bug tracker GHSA-j7v7-w73r-mm5x
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-j7v7-w73r-mm5x>`_

- `PR 45066 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/45066>`_

- `PR 45135 fix for v3.0.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/45135>`_

- `PR 45134 fix for v2.7.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/45134>`_

:cve:`2022-1841`
----------------

Out-of-Bound Write in tcp_flags

This has been fixed in main for v3.1.0

- `Zephyr project bug tracker GHSA-5c3j-p8cr-2pgh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-5c3j-p8cr-2pgh>`_

- `PR 45796 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/45796>`_

:cve:`2022-2741`
----------------

can: denial-of-service can be triggered by a crafted CAN frame

This has been fixed in main for v3.2.0

- `Zephyr project bug tracker GHSA-hx5v-j59q-c3j8
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hx5v-j59q-c3j8>`_

- `PR 47903 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/47903>`_

- `PR 47957 fix for v3.1.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/47957>`_

- `PR 47958 fix for v3.0.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/47958>`_

- `PR 47959 fix for v2.7.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/47959>`_

:cve:`2022-2993`
----------------

bt: host: Wrong key validation check

This has been fixed in main for v3.2.0

- `Zephyr project bug tracker GHSA-3286-jgjx-8cvr
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-3286-jgjx-8cvr>`_

- `PR 48733 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/48733>`_

:cve:`2022-3806`
----------------

DoS: Invalid Initialization in le_read_buffer_size_complete()

- `Zephyr project bug tracker GHSA-w525-fm68-ppq3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-w525-fm68-ppq3>`_

CVE-2023
========

:cve:`2023-0396`
----------------

Buffer Overreads in Bluetooth HCI

- `Zephyr project bug tracker GHSA-8rpp-6vxq-pqg3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8rpp-6vxq-pqg3>`_

:cve:`2023-0397`
----------------

DoS: Invalid Initialization in le_read_buffer_size_complete()

- `Zephyr project bug tracker GHSA-wc2h-h868-q7hj
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-wc2h-h868-q7hj>`_

This has been fixed in main for v3.3.0

- `PR 54905 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/54905>`_

- `PR 47957 fix for v3.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/55024>`_

- `PR 47958 fix for v3.1.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/55023>`_

- `PR 47959 fix for v2.7.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/55022>`_

:cve:`2023-0779`
----------------

net: shell: Improper input validation

- `Zephyr project bug tracker GHSA-9xj8-6989-r549
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-9xj8-6989-r549>`_

This has been fixed in main for v3.3.0

- `PR 54371 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/54371>`_

- `PR 54380 fix for v3.2.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/54380>`_

- `PR 54381 fix for v2.7.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/54381>`_

:cve:`2023-1901`
----------------

HCI send_sync Dangling Semaphore Reference Re-use

- `Zephyr project bug tracker GHSA-xvvm-8mcm-9cq3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-xvvm-8mcm-9cq3>`_

This has been fixed in main for v3.4.0

- `PR 56709 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/56709>`_

:cve:`2023-1902`
----------------

HCI Connection Creation Dangling State Reference Re-use

- `Zephyr project bug tracker GHSA-fx9g-8fr2-q899
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fx9g-8fr2-q899>`_

This has been fixed in main for v3.4.0

- `PR 56709 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/56709>`_

:cve:`2023-3725`
----------------

Potential buffer overflow vulnerability in the Zephyr CANbus subsystem.

- `Zephyr project bug tracker GHSA-2g3m-p6c7-8rr3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-2g3m-p6c7-8rr3>`_

This has been fixed in main for v3.5.0

- `PR 61502 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/61502>`_

- `PR 61518 fix for 3.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/61518>`_

- `PR 61517 fix for 3.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/61517>`_

- `PR 61516 fix for 2.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/61516>`_

:cve:`2023-4257`
----------------

Unchecked user input length in the Zephyr WiFi shell module can cause
buffer overflows.

- `Zephyr project bug tracker GHSA-853q-q69w-gf5j
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-853q-q69w-gf5j>`_

This has been fixed in main for v3.5.0

- `PR 605377 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/605377>`_

- `PR 61383 fix for 3.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/61383>`_

:cve:`2023-4258`
----------------

bt: mesh: vulnerability in provisioning protocol implementation on provisionee side

- `Zephyr project bug tracker GHSA-m34c-cp63-rwh7
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m34c-cp63-rwh7>`_

This has been fixed in main for v3.5.0

- `PR 59467 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/59467>`_

- `PR 60078 fix for 3.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/60078>`_

- `PR 60079 fix for 3.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/60079>`_

:cve:`2023-4259`
----------------

Buffer overflow vulnerabilities in the Zephyr eS-WiFi driver

- `Zephyr project bug tracker GHSA-gghm-c696-f4j4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gghm-c696-f4j4>`_

This has been fixed in main for v3.5.0

- `PR 63074 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/63074>`_

- `PR 63750 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/63750>`_

:cve:`2023-4260`
----------------

Off-by-one buffer overflow vulnerability in the Zephyr FS subsystem

- `Zephyr project bug tracker GHSA-gj27-862r-55wh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gj27-862r-55wh>`_

This has been fixed in main for v3.5.0

- `PR 63079 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/63079>`_

:cve:`2023-4262`
----------------

- This issue has been determined to be a false positive after further analysis.

:cve:`2023-4263`
----------------

Potential buffer overflow vulnerability in the Zephyr IEEE 802.15.4 nRF 15.4 driver.

- `Zephyr project bug tracker GHSA-rf6q-rhhp-pqhf
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rf6q-rhhp-pqhf>`_

This has been fixed in main for v3.5.0

- `PR 60528 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/60528>`_

- `PR 61384 fix for 3.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/61384>`_

- `PR 61216 fix for 2.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/61216>`_

:cve:`2023-4264`
----------------

Potential buffer overflow vulnerabilities in the Zephyr Bluetooth subsystem

- `Zephyr project bug tracker GHSA-rgx6-3w4j-gf5j
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rgx6-3w4j-gf5j>`_

This has been fixed in main for v3.5.0

- `PR 58834 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/58834>`_

- `PR 60465 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/60465>`_

- `PR 61845 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/61845>`_

- `PR 61385 fix for 3.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/61385>`_

:cve:`2023-4265`
----------------

Two potential buffer overflow vulnerabilities in Zephyr USB code

- `Zephyr project bug tracker GHSA-4vgv-5r6q-r6xh
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-4vgv-5r6q-r6xh>`_

This has been fixed in main for v3.4.0

- `PR 59157 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/59157>`_
- `PR 59018 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/59018>`_

:cve:`2023-4424`
----------------

bt: hci: DoS and possible RCE

- `Zephyr project bug tracker GHSA-j4qm-xgpf-qjw3
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-j4qm-xgpf-qjw3>`_

This has been fixed in main for v3.5.0

- `PR 61651 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/61651>`_

- `PR 61696 fix for 3.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/61696>`_

- `PR 61695 fix for 3.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/61695>`_

- `PR 61694 fix for 2.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/61694>`_


:cve:`2023-5055`
----------------

L2CAP: Possible Stack based buffer overflow in le_ecred_reconf_req()

- `Zephyr project bug tracker GHSA-wr8r-7f8x-24jj
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-wr8r-7f8x-24jj>`_

This has been fixed in main for v3.5.0

- `PR 62381 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/62381>`_


:cve:`2023-5139`
----------------

Potential buffer overflow vulnerability in the Zephyr STM32 Crypto driver.

- `Zephyr project bug tracker GHSA-rhrc-pcxp-4453
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-rhrc-pcxp-4453>`_

This has been fixed in main for v3.5.0

- `PR 61839 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/61839>`_

:cve:`2023-5184`
----------------

Potential signed to unsigned conversion errors and buffer overflow
vulnerabilities in the Zephyr IPM driver

- `Zephyr project bug tracker GHSA-8x3p-q3r5-xh9g
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-8x3p-q3r5-xh9g>`_

This has been fixed in main for v3.5.0

- `PR 63069 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/63069>`_

:cve:`2023-5563`
----------------

The SJA1000 CAN controller driver backend automatically attempts to recover
from a bus-off event when built with CONFIG_CAN_AUTO_BUS_OFF_RECOVERY=y. This
results in calling k_sleep() in IRQ context, causing a fatal exception.

- `Zephyr project bug tracker GHSA-98mc-rj7w-7rpv
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-98mc-rj7w-7rpv>`_

This has been fixed in main for v3.5.0

- `PR 63713 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/63713>`_

- `PR 63718 fix for 3.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/63718>`_

- `PR 63717 fix for 3.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/63717>`_

:cve:`2023-5753`
----------------

Potential buffer overflow vulnerabilities in the Zephyr Bluetooth
subsystem source code when asserts are disabled.

- `Zephyr project bug tracker GHSA-hmpr-px56-rvww
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hmpr-px56-rvww>`_

This has been fixed in main for v3.5.0

- `PR 63605 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/63605>`_


:cve:`2023-5779`
----------------

Out of bounds issue in remove_rx_filter in multiple can drivers.

- `Zephyr project bug tracker GHSA-7cmj-963q-jj47
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-7cmj-963q-jj47>`_

This has been fixed in main for v3.6.0

- `PR 64399 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/64399>`_

- `PR 64416 fix for 3.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/64416>`_

- `PR 64415 fix for 3.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/64415>`_

- `PR 64427 fix for 3.3
  <https://github.com/zephyrproject-rtos/zephyr/pull/64427>`_

- `PR 64431 fix for 2.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/64431>`_

:cve:`2023-6249`
----------------

Signed to unsigned conversion problem in esp32_ipm_send may lead to buffer overflow

- `Zephyr project bug tracker GHSA-32f5-3p9h-2rqc
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-32f5-3p9h-2rqc>`_

This has been fixed in main for v3.6.0

- `PR 65546 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/65546>`_

:cve:`2023-6749`
----------------

Potential buffer overflow due unchecked data coming from user input in settings shell.

- `Zephyr project bug tracker GHSA-757h-rw37-66hw
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-757h-rw37-66hw>`_

This has been fixed in main for v3.6.0

- `PR 66451 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/66451>`_

- `PR 66584 fix for 3.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/66584>`_

:cve:`2023-6881`
----------------

Potential buffer overflow vulnerability in Zephyr fuse file system.

- `Zephyr project bug tracker GHSA-mh67-4h3q-p437
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-mh67-4h3q-p437>`_

This has been fixed in main for v3.6.0

- `PR 66592 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/66592>`_

:cve:`2023-7060`
----------------

Missing Security Control in Zephyr OS IP Packet Handling

- `Zephyr project bug tracker GHSA-fjc8-223c-qgqr
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-fjc8-223c-qgqr>`_

This has been fixed in main for v3.6.0

- `PR 66645 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/66645>`_

- `PR 66739 fix for 3.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/66739>`_

- `PR 66738 fix for 3.4
  <https://github.com/zephyrproject-rtos/zephyr/pull/66738>`_

- `PR 66887 fix for 2.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/66887>`_

CVE-2024
========

:cve:`2024-1638`
----------------

Bluetooth characteristic LESC security requirement not enforced without additional flags

- `Zephyr project bug tracker GHSA-p6f3-f63q-5mc2
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p6f3-f63q-5mc2>`_

This has been fixed in main for v3.6.0

- `PR 69170 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/69170>`_

:cve:`2024-3077`
----------------

Bluetooth: Integer underflow in gatt_find_info_rsp. A malicious Bluetooth LE
device can crash Bluetooth LE victim device by sending malformed gatt packet.

- `Zephyr project bug tracker GHSA-gmfv-4vfh-2mh8
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gmfv-4vfh-2mh8>`_

This has been fixed in main for v3.7.0

- `PR 69396 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/69396>`_

:cve:`2024-3332`
----------------

Bluetooth: DoS caused by null pointer dereference.

A malicious Bluetooth LE device can send a specific order of packet
sequence to cause a DoS attack on the victim Bluetooth LE device.

- `Zephyr project bug tracker GHSA-jmr9-xw2v-5vf4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jmr9-xw2v-5vf4>`_

This has been fixed in main for v3.7.0

- `PR 71030 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/71030>`_


:cve:`2024-4785`
----------------

Bluetooth: Missing Check in LL_CONNECTION_UPDATE_IND Packet Leads to Division by Zero

- `Zephyr project bug tracker GHSA-xcr5-5g98-mchp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-xcr5-5g98-mchp>`_

This has been fixed in main for v3.7.0

- `PR 72608 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/72608>`_

:cve:`2024-5754`
----------------

BT: Encryption procedure host vulnerability

- `Zephyr project bug tracker GHSA-gvv5-66hw-5qrc
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gvv5-66hw-5qrc>`_

This has been fixed in main for v3.7.0

- `PR 7395 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/7395>`_

- `PR 74124 fix for 3.6
  <https://github.com/zephyrproject-rtos/zephyr/pull/74124>`_

- `PR 74123 fix for 3.5
  <https://github.com/zephyrproject-rtos/zephyr/pull/74123>`_

- `PR 74122 fix for 2.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/74122>`_

:cve:`2024-5931`
----------------

BT: Unchecked user input in bap_broadcast_assistant

- `Zephyr project bug tracker GHSA-r8h3-64gp-wv7f
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-r8h3-64gp-wv7f>`_

This has been fixed in main for v3.7.0

- `PR 74062 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/74062>`_

- `PR 77966 fix for 3.6
  <https://github.com/zephyrproject-rtos/zephyr/pull/77966>`_


:cve:`2024-6135`
----------------

BT:Classic: Multiple missing buf length checks

- `Zephyr project bug tracker GHSA-2mp4-4g6f-cqcx
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-2mp4-4g6f-cqcx>`_

This has been fixed in main for v3.7.0

- `PR 74283 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/74283>`_

- `PR 77964 fix for 3.6
  <https://github.com/zephyrproject-rtos/zephyr/pull/77964>`_

:cve:`2024-6137`
----------------

BT: Classic: SDP OOB access in get_att_search_list

- `Zephyr project bug tracker GHSA-pm38-7g85-cf4f
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-pm38-7g85-cf4f>`_

This has been fixed in main for v3.7.0

- `PR 75575 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/75575>`_

:cve:`2024-6258`
----------------

BT: Missing length checks of net_buf in rfcomm_handle_data

- `Zephyr project bug tracker GHSA-7833-fcpm-3ggm
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-7833-fcpm-3ggm>`_

This has been fixed in main for v3.7.0

- `PR 74640 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/74640>`_

:cve:`2024-6259`
----------------

BT: HCI: adv_ext_report Improper discarding in adv_ext_report

- `Zephyr project bug tracker GHSA-p5j7-v26w-wmcp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-p5j7-v26w-wmcp>`_

This has been fixed in main for v3.7.0

- `PR 74639 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/74639>`_

- `PR 77960 fix for 3.6
  <https://github.com/zephyrproject-rtos/zephyr/pull/77960>`_

:cve:`2024-6442`
----------------

Bluetooth: ASCS Unchecked tailroom of the response buffer

- `Zephyr project bug tracker GHSA-m22j-ccg7-4v4h
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-m22j-ccg7-4v4h>`_

This has been fixed in main for v3.7.0

- `PR 74976 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/74976>`_

- `PR 77958 fix for 3.6
  <https://github.com/zephyrproject-rtos/zephyr/pull/77958>`_

:cve:`2024-6443`
----------------

zephyr: out-of-bound read in utf8_trunc

- `Zephyr project bug tracker GHSA-gg46-3rh2-v765
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-gg46-3rh2-v765>`_

This has been fixed in main for v3.7.0

- `PR 74949 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/74949>`_

- `PR 78286 fix for 3.6
  <https://github.com/zephyrproject-rtos/zephyr/pull/78286>`_

:cve:`2024-6444`
----------------

Bluetooth: ots: missing buffer length check

- `Zephyr project bug tracker GHSA-qj4r-chj6-h7qp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-qj4r-chj6-h7qp>`_

This has been fixed in main for v3.7.0

- `PR 74944 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/74944>`_

- `PR 77954 fix for 3.6
  <https://github.com/zephyrproject-rtos/zephyr/pull/77954>`_

:cve:`2024-8798`
----------------

Bluetooth: classic: avdtp: missing buffer length check

- `Zephyr project bug tracker GHSA-r7pm-f93f-f7fp
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-r7pm-f93f-f7fp>`_

This has been fixed in main for v4.0.0

- `PR 77969 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/77969>`_

- `PR 78409 fix for 3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/78409>`_

:cve:`2024-10395`
-----------------

net: lib: http_server: Buffer Under-read

No proper validation of the length of user input in
http_server_get_content_type_from_extension could cause a
segmentation fault or crash by causing memory to be read
outside of the bounds of the buffer.

- `Zephyr project bug tracker GHSA-hfww-j92m-x8fv
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-hfww-j92m-x8fv>`_

This has been fixed in main for v4.0.0

- `PR 80396 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/80396>`_

:cve:`2024-11263`
-----------------

arch: riscv: userspace: potential security risk when CONFIG_RISCV_GP=y

A rogue thread can corrupt the gp reg and cause the entire system to hard fault at best, at worst,
it can potentially trick the system to access another set of random global symbols.

- `Zephyr project bug tracker GHSA-jjf3-7x72-pqm9
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jjf3-7x72-pqm9>`_

This has been fixed in main for v4.0.0

- `PR 81155 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/81155>`_

- `PR 81370 fix for 3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/81370>`_

:cve:`2025-1673`
----------------

Out of bounds read when calling crc16_ansi and strlen in dns_validate_msg

A malicious or malformed DNS packet without a payload can cause an out-of-bounds
read, resulting in a crash (denial of service) or an incorrect computation.

- `Zephyr project bug tracker GHSA-jjhx-rrh4-j8mx
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-jjhx-rrh4-j8mx>`_

This has been fixed in main for v4.1.0

- `PR 82072 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/82072>`_

- `PR 82289 fix for 4.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/82289>`_

- `PR 82288 fix for 3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/82288>`_

:cve:`2025-1674`
----------------

Out of bounds read when unpacking DNS answers

A lack of input validation allows for out of bounds reads caused by malicious or
malformed packets.

- `Zephyr project bug tracker GHSA-x975-8pgf-qh66
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-x975-8pgf-qh66>`_

This has been fixed in main for v4.1.0

- `PR 82072 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/82072>`_

- `PR 82289 fix for 4.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/82289>`_

- `PR 82288 fix for 3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/82288>`_

:cve:`2025-1675`
----------------

Out of bounds read in dns_copy_qname

The function dns_copy_qname in dns_pack.c performs performs a memcpy operation
with an untrusted field and does not check if the source buffer is large enough
to contain the copied data.

- `Zephyr project bug tracker GHSA-2m84-5hfw-m8v4
  <https://github.com/zephyrproject-rtos/zephyr/security/advisories/GHSA-2m84-5hfw-m8v4>`_

This has been fixed in main for v4.1.0

- `PR 82072 fix for main
  <https://github.com/zephyrproject-rtos/zephyr/pull/82072>`_

- `PR 82289 fix for 4.0
  <https://github.com/zephyrproject-rtos/zephyr/pull/82289>`_

- `PR 82288 fix for 3.7
  <https://github.com/zephyrproject-rtos/zephyr/pull/82288>`_
