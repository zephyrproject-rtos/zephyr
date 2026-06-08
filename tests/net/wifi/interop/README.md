# native_sim Wi-Fi interop test

This test exercises the `native_sim` Wi-Fi driver (`drivers/wifi/native_sim/`)
end to end. It builds a small Wi-Fi shell application, brings up a simulated
Linux `mac80211_hwsim` radio with two hostapd access points, and uses the
[pytest twister harness](https://docs.zephyrproject.org/latest/develop/test/pytest.html)
to drive the `wifi` shell (`scan` / `connect` / `status` / `disconnect`) and
assert on the results.

Simulated access points (from net-tools):

| SSID          | Security  | Channel | Passphrase |
|---------------|-----------|---------|------------|
| `zephyr-open` | open      | 1       | -          |
| `zephyr-wpa2` | WPA2-PSK  | 6       | `password` |

The test is marked `slow`, so it is **not** run in regular CI; trigger it
manually (see below).

## How it works

The native_sim driver opens an `AF_PACKET` socket and an `nl80211` netlink
socket on a host network interface, so two host-side prerequisites must be met
before the binary runs:

1. A `mac80211_hwsim` STA radio + hostapd APs must exist on the host.
2. The `zephyr.exe` binary must have `cap_net_raw` and `cap_net_admin`.

Because the radio is a host kernel object and the STA interface must live in the
same network namespace as `zephyr.exe`, the test does **not** isolate the radio
in a container by default — it uses the host directly. A privileged Docker
option is documented at the end.

### Interface name

The driver binds to the host interface named `CONFIG_WIFI_NATIVE_SIM_DRV_NAME`,
i.e. **`zwifi`**. The host STA interface must have this exact name, which is what
the net-tools wrapper `zwifi.sh` (`-i zwifi`) creates.

## Running (host setup - default)

Prerequisites on the host: `iw`, `hostapd`, `dnsmasq`, `libnl-3` / `libnl-genl-3`
(plus the 32-bit variants used by the build), and the
[net-tools](https://github.com/zephyrproject-rtos/net-tools) checkout on the
`zwifi` setup branch at `$ZEPHYR_BASE/../tools/net-tools`. Root is required for
`modprobe`, the network namespaces and `setcap` (passwordless `sudo`, or run the
whole command as root).

```sh
source ../.venv/bin/activate
```

Option A - let the test bring the radio up and tear it down:

```sh
./scripts/twister -p native_sim --enable-slow -i -c \
    -T tests/net/wifi/interop \
    --pytest-args=--wifi-setup=net-tools
```

Option B - bring the radio up yourself, then run the test against it:

```sh
# Start (creates the zwifi STA interface and the APs):
sudo ../tools/net-tools/net-setup.sh -c zwifi.conf -i zwifi start

./scripts/twister -p native_sim --enable-slow -i -c -T tests/net/wifi/interop

# Stop when done:
sudo ../tools/net-tools/net-setup.sh -c zwifi.conf -i zwifi stop
```

### pytest options

Passed via `--pytest-args=<opt>` (repeat the flag per option):

* `--wifi-setup={none,net-tools}` - `none` (default) assumes the radio is
  already up; `net-tools` runs the net-tools script for the session.
* `--net-tools-dir=<path>` - net-tools location (default
  `$ZEPHYR_BASE/../tools/net-tools`).
* `--wifi-iface=<name>` - host STA interface name (default `zwifi`).
* `--no-setcap` - skip `setcap` (use when the binary already has the caps, e.g.
  when running the whole test as root).

## Running (Docker - optional)

`mac80211_hwsim` is a host kernel module, so a container cannot truly isolate
it: the module loads into the shared host kernel and the radios are visible
host-wide. The only self-contained option is an **all-in-one privileged**
container that `modprobe`s the module, starts the APs and runs twister/pytest
itself, sharing the host network namespace:

```sh
./tests/net/wifi/interop/docker-test.sh
```

This needs `--privileged`, `--network=host`, and the host's `/lib/modules`
bind-mounted read-only (so the in-container `modprobe` can find and load
`mac80211_hwsim` into the shared host kernel), plus a host kernel that actually
ships the module. See `docker/Dockerfile` and `docker-test.sh`; treat them as a
starting point for a reproducible environment rather than true isolation.
