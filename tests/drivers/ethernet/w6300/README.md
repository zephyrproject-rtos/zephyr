# W6300 Ethernet Driver Test

This test application verifies the functionality of the W6300 Ethernet driver on the
`w6300_evb_pico2_rp2350a_hazard3` board by running a minimal HTTP server.

## Building and Running

To build and run the test, use the `west` command:

```bash
west build -b w6300_evb_pico2_rp2350a_hazard3 tests/drivers/ethernet/w6300
west flash
```

## Expected Output

The test initializes the network interface and starts an HTTP server on port 8080.
When a client connects, it logs the request and replies with a simple HTML page.
Typical log output looks like this:

```
Starting W6300 HTTP server
HTTP server listening on port 8080
Client connected
Request:
GET / HTTP/1.1
Host: 192.168.11.2
```

## Phone Ethernet Setup Tips

If you cannot reach the device from a phone (for example, `ERR_ADDRESS_UNREACHABLE`),
make sure the phone is on the same IPv4 subnet as the W6300 board. For a direct
Ethernet adapter connection you can set a manual IPv4 address on the phone, for
example:

* **Phone IP address:** `192.168.11.10`
* **Subnet mask:** `255.255.255.0`
* **Gateway:** `192.168.11.1` (which matches the board IP in this static setup)

Then open `http://192.168.11.2:8080` in the phone browser (matching the board IP in
`prj.conf`). Since DHCP is disabled in this test, you must set a static IP on the phone
(e.g. `192.168.11.10`) to connect.
