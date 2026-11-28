.. zephyr:code-sample:: tailscale
   :name: Native Tailscale client
   :relevant-api: tailscale

   Join a tailnet and use ordinary TCP and UDP sockets over WireGuard.

Overview
********

The sample uses ``CONFIG_TAILSCALE`` and includes ``<zephyr/net/tailscale.h>``.
The client requires the native IPv4 and IPv6 unicast route tables
(``CONFIG_NET_IPV4_ROUTE`` and ``CONFIG_NET_IPV6_ROUTE``).
The sample enables ``CONFIG_UTF8`` to validate names from the control server.
``CONFIG_TAILSCALE`` selects the internal control stream and policy components.
Its TF-PSA-Crypto user configuration enables the Mbed TLS pthread backend
for the client and the dedicated WireGuard handshake work queue.
It initializes one node, joins a configured Tailscale or Headscale controller,
and waits for the controller to publish policy. Ordinary sockets use the
installed route and DNS policy.

Building and running
********************

Supply a DER trust anchor for the configured controller and any relay to be
used by this node. Provision Wi-Fi credentials through the board's Settings
backend. The sample connects using the stored credentials and acquires IPv4
with DHCP. Provide trusted UTC on the board before starting the client.
The build below enables SNTP bootstrap, which requires DNS and UDP access to
``time.cloudflare.com``. The sample stops startup if the SNTP request fails.
If the network cannot reach that server, leave
``CONFIG_SAMPLE_TAILSCALE_SNTP_BOOTSTRAP`` disabled and set trusted UTC separately.

To provision credentials on the Pico with the Wi-Fi shell, enable
``CONFIG_SHELL``, ``CONFIG_NET_L2_WIFI_SHELL`` and
``CONFIG_WIFI_CREDENTIALS_SHELL``.

.. zephyr-app-commands::
   :zephyr-app: samples/net/tailscale
   :board: weact_esp32s3_mini/esp32s3/procpu
   :gen-args: -DTAILSCALE_CA_FILE=/path/to/root.der -DCONFIG_SAMPLE_TAILSCALE_PRIVATE_CONSOLE=y -DCONFIG_SAMPLE_TAILSCALE_SNTP_BOOTSTRAP=y
   :goals: build

For a private controller, set ``CONFIG_TAILSCALE_CONTROL_HOST`` and
``CONFIG_TAILSCALE_CONTROL_PORT`` to its HTTPS endpoint. The certificate must
match the configured host. A build without a trust anchor can be used for
compile checks. Starting that build reports ``-ENOENT``.

On first boot, the private-console option displays the approval URL. Approve
the node and wait for the controller map. Keep the URL on the provisioning
channel. Applications using a preauthorization key can supply it at runtime
through the configuration passed to ``tailscale_start()``. The sample stores the
persistent identity in Zephyr Settings under ``tailscale/identity_v1`` and reuses
it on reboot.

Set ``CONFIG_SAMPLE_TAILSCALE_PEER`` to the MagicDNS name of a peer providing
TCP echo on port 9001 and UDP echo on port 9000. The sample resolves the name
and checks UDP and TCP replies over IPv4 and, when available, IPv6. It retries
UDP timeouts while WireGuard establishes the path and stops the client after
the exchange. Leave the peer unset to keep the interface available. The default
profile supports four live peers. The client rejects a map that exceeds this limit.
Configure
``CONFIG_TAILSCALE_PEER_STABLE_ID`` to select a single peer on a smaller image.

``CONFIG_TAILSCALE_DISCO`` enables direct UDP and authenticated endpoint
discovery. ``CONFIG_TAILSCALE_DERP`` enables relay connections. Both are enabled
by default. To use direct UDP only, keep DISCO enabled and set
``CONFIG_TAILSCALE_DERP=n``. The control server must be reachable directly,
and both peers must be able to reach each other over UDP.
Keep DERP enabled if your deployment uses ``CONFIG_TAILSCALE_REQUIRED_PROXY``.
The build rejects a configuration that requires the proxy and disables DERP.

With direct UDP only, enable ``CONFIG_TAILSCALE_DISCOVER_ENDPOINT`` to discover
and advertise the public IPv4 UDP mapping via STUN. The relay map must list a
node with a literal IPv4 address and an enabled STUN port. The client skips
nodes marked ``STUNOnly`` and regions marked ``NoMeasureNoHome`` or ``Avoid``.
Relay connections remain disabled.

When both DISCO and DERP are disabled, the client uses native WireGuard UDP with
the endpoints in the control map. The peer must support this transport without
Tailscale endpoint discovery.

The client reports readiness through the operational state of its virtual
interface. The sample uses ``zsock_getaddrinfo()`` and standard UDP and TCP
sockets after that interface comes up.

To expose a service, list its TCP or UDP port in the start configuration and
use the normal socket bind and listen operations. Tailscale also requires
the current control policy to permit the incoming traffic. Approved subnet
routes and an exit node can be requested in the same configuration.
Use ``tailscale_configure()`` to update routes and services while running.
``tailscale_stop()`` withdraws the interface and can retain the enrolled
identity for the next start.
