:orphan:

..
  What goes here: new capabilities, major improvements and changes that
  materially affect users, with what changed and why it matters. See
  https://docs.zephyrproject.org/latest/releases/index.html#release-documentation
..
  Not here: lists of new, deprecated or removed APIs, Kconfig options,
  bindings, boards, drivers or samples. They are generated into the change
  inventory at release time. Not here either: bug fixes and minor changes,
  they are in the git log; and anything that requires the user to change their
  application, that goes in the migration guide.
..
  If you are describing a feature or functionality, consider adding it to the
  actual project documentation rather than the release notes, so that the
  information does not get lost in time, and link to it from here.

.. _zephyr_4.6:

Zephyr 4.6.0 (Working Draft)
############################

We are pleased to announce the release of Zephyr version 4.6.0.

Major enhancements with this release include:

An overview of the changes required or recommended when migrating your application from Zephyr
v4.5.0 to Zephyr v4.6.0 can be found in the separate :ref:`migration guide<migration_4.6>`.

The following sections describe the notable changes by component.

Security Vulnerability Related
******************************

..
  API changes with a user impact: describe the capability here and the
  required adaptation in the migration guide. The lists of new, deprecated and
  removed APIs and options, boards, shields, drivers and samples are generated
  into the change inventory at release time.

Libraries / Subsystems
**********************

* Architectures

  * arm64: the x19-x28 callee-saved registers are now captured in the exception
    stack frame on the paths that may dump or unwind it (synchronous faults,
    SError, kernel oops and syscall entry) and are included in the coredump
    architecture block, version 3, when
    :kconfig:option:`CONFIG_EXTRA_EXCEPTION_INFO` is enabled. The gdbstub
    coredump decoder was updated accordingly.

* Bluetooth

  * Added a HID Service server (:kconfig:option:`CONFIG_BT_HIDS`), the service
    the HID Device role of the HID over GATT Profile is built on.
  * :c:func:`bt_addr_matches_irk` is now public, so a scanner can check whether a
    resolvable private address was generated from an IRK it holds.
  * PACS exposes the audio locations through :c:func:`bt_pacs_get_location` and
    reports changes to a registered :c:struct:`bt_pacs_cb`.
  * :c:func:`bt_l2cap_server_unregister` removes a registered L2CAP server.
  * :c:member:`bt_gatt_cb.execute_write_ended` tells the application how an Execute
    Write Request ended: whether the client's prepared values were written, cancelled,
    or discarded after a write failed.
  * The ``bt_mcp_media_control_server_`` API controls the local media player and
    replaces the removed ``media_proxy_`` API, see the :ref:`migration guide
    <migration_4.6>`.

* Display

  * Display timings can be read and set at runtime with
    :c:func:`display_get_timing` and :c:func:`display_set_timing`.

* FIDO2

  * Added a PSA Protected Storage backend, enabled with
    :kconfig:option:`CONFIG_FIDO2_STORAGE_PSA_PROTECTED_STORAGE`.

* I3C

  * The requested tCAS (clock after START) is configurable through
    :c:member:`i3c_config_controller.tcas_ps` and the ``tcas-ps`` controller
    Devicetree property.

* Libc

  * ``zephyr_write_stdout()`` no longer converts LF to CRLF. Backends that
    convert on their own, such as the UART and EFI consoles, now receive a
    single LF and emit one CRLF. With Newlib, ARCMWDT and the Picolibc POSIX
    ``write()`` path the duplicated carriage return (CR CR LF) is gone;
    Picolibc's ``printf()`` output is unchanged.

  * ``printf()`` output to backends that pass bytes through, such as RTT, RAM
    console, semihosting, the IPM console sender, WCH SDI, whisper and the
    Bluetooth monitor, is now LF-only, matching ``printk()``.

* LoRaWAN

  * The native backend now supports the KR920, AS923 and US915 regions
    (:kconfig:option:`CONFIG_LORAWAN_REGION_KR920`,
    :kconfig:option:`CONFIG_LORAWAN_REGION_AS923`,
    :kconfig:option:`CONFIG_LORAWAN_REGION_US915`).
  * The LoRaWAN services restore the device class that was in use before a
    multicast class C session started, instead of always reverting to class A.
    The current class can be read with :c:func:`lorawan_get_class`.

* Mbed TLS

  * :kconfig:option:`CONFIG_MBEDTLS_SSL_CLI_C`
  * :kconfig:option:`CONFIG_MBEDTLS_SSL_SRV_C`

* Networking

  * :kconfig:option:`CONFIG_NET_DHCPV6_START_ON_RA` starts the DHCPv6 client
    automatically from the M and O flags of received Router Advertisements, in
    stateful mode with the M flag and in stateless mode (Information-request
    only) with the O flag.
  * HTTP server: accepted connections now have ``TCP_NODELAY`` set, so a response
    sent in several pieces (every TLS response, every websocket frame) is no
    longer held back by the client's delayed acknowledgements. Applications that
    prefer Nagle's algorithm can disable
    :kconfig:option:`CONFIG_HTTP_SERVER_TCP_NODELAY`.
  * HTTP server: HTTPS services no longer run the TLS handshake inside ``accept()``. A client that
    opens a connection and then stalls its handshake used to stop the whole server, HTTP services
    included, for up to :kconfig:option:`CONFIG_NET_SOCKETS_TLS_CONNECT_TIMEOUT` per connection.
  * The native network stack runs its protocol timers and deferred work on a dedicated work queue
    instead of the system work queue, so that a slow system work queue item no longer delays DHCP,
    Neighbor Discovery, address conflict detection or DNS timeouts, and the stack no longer holds
    up other system work queue users. See :kconfig:option:`CONFIG_NET_DEDICATED_WORKQUEUE` and the
    :ref:`migration guide <migration_4.6>`.
  * The connection manager tracks name resolution readiness, see
    :ref:`conn_mgr_monitoring_dns`: :c:macro:`NET_EVENT_L4_DNS_READY`,
    :c:macro:`NET_EVENT_L4_DNS_LOST` and :c:func:`conn_mgr_dns_is_ready`.
  * Threads can block until the network is ready with :c:func:`conn_mgr_wait_ready`,
    instead of tracking connection manager events, see :ref:`conn_mgr_monitoring_waiting`.
    With a ``last_seen`` value, it also paces retries after a failed connection attempt.
  * :c:func:`conn_mgr_get_ready_addr_family` gives the ``ai_family`` to pass to
    :c:func:`zsock_getaddrinfo` so that hostnames resolve only to addresses of a ready
    address family, see :ref:`conn_mgr_monitoring_addr_family`.
  * :c:func:`dns_resolve_get_server_families` reports the address families of the
    available unicast DNS servers.
  * :c:func:`zsock_getaddrinfo` orders ``AF_UNSPEC`` results as per a subset of
    :rfc:`6724`: reachable destinations first, then IPv6 before IPv4. Dual-stack
    applications that use the first result now connect over IPv6 when it is
    usable. :kconfig:option:`CONFIG_NET_SOCKETS_DNS_PREFER_IPV4` restores the
    preference of IPv4, see the :ref:`migration guide <migration_4.6>`.
  * Added the :ref:`STUN <stun_interface>` library, a codec for the messages of
    :rfc:`8489` with the attributes of ICE (:rfc:`8445`): parsing, building,
    MESSAGE-INTEGRITY, FINGERPRINT and the address attributes. A device finds out
    its public address with the Binding transaction, the blocking socket client
    or the ``net stun`` shell commands.

  * OCPP: the charge point can authenticate to the central system with HTTP Basic
    authentication (:kconfig:option:`CONFIG_OCPP_BASIC_AUTH`), with the credentials set in the
    new :c:struct:`ocpp_basic_auth` member of :c:struct:`ocpp_cs_info`. This is the charge
    point authentication of the security profiles 1 and 2 of the OCPP 1.6 security
    whitepaper. Their ``AuthorizationKey`` configuration key is not implemented, see
    :ref:`ocpp_basic_auth`.

  * Ethernet bridge

    * The bridge learns the port of each source MAC address into its FDB
      (:kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_FDB_LEARNING`), so that a frame to a known
      destination is sent out of one port instead of being flooded.
      Learned entries age out after
      :kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_FDB_AGEING_TIME` seconds without a frame from
      their address, and :c:struct:`eth_bridge_fdb_entry` records when that frame was last seen
      (FDB API version 0.2.0).
    * Experimental support for the Rapid Spanning Tree Protocol of IEEE Std 802.1Q-2014
      (:kconfig:option:`CONFIG_NET_ETHERNET_BRIDGE_STP`). Once started on a bridge with
      :c:func:`eth_bridge_stp_enable`, it blocks the bridge ports that would make a loop and
      falls back to STP BPDUs on a port where an STP bridge is attached. The
      ``NET_EVENT_ETHERNET_BRIDGE_PORT_STATE``, ``NET_EVENT_ETHERNET_BRIDGE_NEW_ROOT`` and
      ``NET_EVENT_ETHERNET_BRIDGE_TOPOLOGY_CHANGE`` net_mgmt events report port role and state
      changes, a new root bridge and the start of a topology change. The protocol does not
      run on ports that forward frames in hardware, DSA switch ports and Ethernet devices that
      report the new :c:enumerator:`ETHERNET_HW_PORT_FORWARDING` capability, as it cannot
      block them, unless the device also reports the new
      :c:enumerator:`ETHERNET_HW_STP_PORT_STATE` capability and takes the port state and FDB
      flushes from the protocol.

  * The ``local_in`` packet filter rules can match UDP and TCP packets by source or destination
    port with :c:macro:`NPF_UDP_SRC_PORT_MATCH`, :c:macro:`NPF_UDP_DST_PORT_MATCH`,
    :c:macro:`NPF_TCP_SRC_PORT_MATCH` and :c:macro:`NPF_TCP_DST_PORT_MATCH`, and TCP segments by
    the flags of their header with :c:macro:`NPF_TCP_FLAGS_MATCH`, see
    :ref:`net_pkt_filter_interface`. A rule list can thus, for example, drop the TCP connection
    requests to all but some ports.

  * TCP can scale its window (RFC 7323) when :kconfig:option:`CONFIG_NET_TCP_WINDOW_SCALE` is
    enabled, so a connection can use send and receive windows above 64 KB. The window size
    options, ``SO_RCVBUF`` and ``SO_SNDBUF`` then accept values up to 65535 << 14.
  * TCP derives the retransmission timeout (RTO) of each connection from its measured round
    trip time (:rfc:`6298`). :kconfig:option:`CONFIG_NET_TCP_INIT_RETRANSMISSION_TIMEOUT` is
    only used until the first measurement, so a path slower than that value, for example with
    large windows or Wi-Fi power save, no longer causes spurious retransmissions of the whole
    window. The RTO is kept between :kconfig:option:`CONFIG_NET_TCP_MIN_RTO` and
    :kconfig:option:`CONFIG_NET_TCP_MAX_RTO`. The retransmissions, and so the time until an
    unanswered connection fails with ``-ETIMEDOUT``, grow with the RTO of the connection.

* PTP Clock

  * :c:func:`ptp_clock_adjust_rate` adjusts the clock rate by an offset in scaled parts per
    million, as used by gPTP and IEEE 1588, instead of a floating-point ratio, so rate
    corrections no longer need floating-point arithmetic.

  * :ref:`MQTT 5.0 <mqtt_socket_interface>` support in the MQTT client
    (:kconfig:option:`CONFIG_MQTT_VERSION_5_0`) is no longer experimental.

* USB Type-C

  * Dual-role ports can prefer the Sink role through the Try.SNK and TryWait.SRC states
    (:kconfig:option:`CONFIG_USBC_CSM_DRP_TRY_SNK`).

* Utilities

  * :c:func:`utf8_is_valid` checks a given number of bytes for well-formed UTF-8
    as defined in :rfc:`3629`; overlong sequences, UTF-16 surrogates and code
    points above U+10FFFF are refused.

* Shell

  * With :kconfig:option:`CONFIG_SHELL_WILDCARD` enabled, an argument containing ``*`` or ``?``
    that matches no subcommand is now passed to the command handler unchanged. Previously the
    command was run without that argument and its remaining arguments, which usually failed
    with a wrong argument count.

Devicetree
**********

* New ``var-uint64`` binding property type for variable-length unsigned integers of up to 64
  bits, encoded as one cell (32-bit) or two cells (64-bit, high word first) and read with a scalar
  ``DT_PROP`` macro.

* :dtcompatible:`gnss-nmea-generic` supports optional enable and reset GPIOs, with configurable
  reset pulse and wake-up delays.

* :dtcompatible:`zephyr,charger-gpio` supports charging and charge-complete status inputs,
  including chargers without a software-controlled enable pin.

* Networking

  * Added on-device generation of TLS private keys and Certificate Signing Requests (CSRs),
    so that the private key never leaves the device. See
    :c:func:`tls_credential_keygen_generate` and :c:func:`tls_credential_keygen_delete`.
    Enable it with :kconfig:option:`CONFIG_TLS_CREDENTIALS_KEYGEN`.
    The :ref:`TLS credentials shell <tls_credentials_shell>` has the new ``cred keygen``,
    ``cred keydel``, ``cred csr`` and ``cred pubkey`` commands, and the
    :zephyr:code-sample:`tls-keygen` sample shows the full flow.

* ESP32 SDHC slots support GPIO-based card detection, including GPIOs on external expanders.
  Configure the optional ``cd-gpios`` property in :dtcompatible:`espressif,esp32-sdhc-slot`.
  An optional sleep pinctrl state controls the signal pins while card power is off.

Other notable changes
*********************

* Architectures

  * ARM Cortex-A/R: with :kconfig:option:`CONFIG_USE_SWITCH`, ISRs run in SYS mode and nested
    interrupts use the interrupt stack, which fixes return address corruption by a nested IRQ
    (:github:`122601`).

* Bluetooth

  * :kconfig:option:`CONFIG_BT_PRIVACY` can now be enabled in Broadcaster- and Observer-only builds,
    without :kconfig:option:`CONFIG_BT_SMP`.

* Hardware information

  * NXP RW61x hwinfo reports PM4 wakeup resets as :c:macro:`RESET_LOW_POWER_WAKE`.

* Interrupt controllers

  * GIC v3 LPI and ITS tables use bounded static storage, making their memory requirements
    predictable at build time and allowing initialization without a system heap. See the
    :ref:`migration guide <migration_4.6>` for capacity sizing and heap configuration changes.

* Kernel

  * Threads can be created pinned to CPU 0 by default instead of being allowed on all CPUs, with
    :kconfig:option:`CONFIG_SCHED_CPU_MASK_DEFAULT_SINGLE_CPU`. A thread's mask can be widened with
    :c:func:`k_thread_cpu_mask_enable` while it is not runnable (before it is started, or while
    it is blocked or suspended).

..
  Any more descriptive subsystem or driver changes. Do you really want to write
  a paragraph or is it enough to link to the documentation of the change?
