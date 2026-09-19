.. zephyr:code-sample:: fota-http
   :name: Firmware Over-the-Air over HTTP
   :relevant-api: fota_http

   Download a signed firmware image over HTTP into the MCUboot secondary
   slot and swap to it.

Overview
********

This sample shows how to update a device from any plain HTTP server using
the :ref:`fota_http` library. The device fetches a signed image with an
HTTP GET request, streams it into the slot MCUboot is not running from, and
then requests the bootloader to boot it. No update server or device
management protocol is required: a directory served by a web server is
enough.

The sample enables the ``fota`` shell command so the whole cycle can be
driven from the console.

Requirements
************

* A board with a network interface and an MCUboot partition layout with
  two application slots.
* An HTTP server reachable from the board.

The sample is built with sysbuild so that MCUboot is built and flashed
along with the application. The bootloader is configured in swap-using-offset
mode so that the revert path can be exercised; overwrite-only mode has no
revert support.

.. warning::

   The images are signed with the MCUboot development key shipped in the
   MCUboot repository. Anyone can sign an image with it. Set
   :kconfig:option:`CONFIG_MCUBOOT_SIGNATURE_KEY_FILE` in the MCUboot
   configuration to a key of your own before using this on a real device.

Building and Running
********************

Build and flash the sample with sysbuild. Give the image a version so the
two builds can be told apart on the console:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mgmt/fota_http
   :board: esp32c5_devkitc/esp32c5/hpcore
   :goals: build flash
   :west-args: --sysbuild
   :gen-args: -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\"1.0.0\"
   :compact:

Build a second image with a different version. This is the one the board
will download, so keep the build directory around:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mgmt/fota_http
   :board: esp32c5_devkitc/esp32c5/hpcore
   :build-dir: build-v2
   :goals: build
   :west-args: --sysbuild
   :gen-args: -DCONFIG_MCUBOOT_IMGTOOL_SIGN_VERSION=\"2.0.0\"
   :compact:

Serve the signed image from the host. Any HTTP server works, for example:

.. code-block:: console

   cd build-v2/fota_http/zephyr && python3 -m http.server 8000 --bind 0.0.0.0

Only ``zephyr.signed.bin`` is accepted by MCUboot, not ``zephyr.bin``.

On boards with Wi-Fi, connect to the network first:

.. code-block:: console

   uart:~$ wifi connect -s <ssid> -p <passphrase> -k <key_mgmt>

Boards with Ethernet obtain an address with DHCP at boot.

Download the image, request the swap and reboot:

.. code-block:: console

   uart:~$ fota download http://<host-ip>:8000/zephyr.signed.bin
   uart:~$ fota apply
   uart:~$ kernel reboot cold

After the reboot the console shows image version 2.0.0 running as a test
image. Without confirmation, the next reboot makes MCUboot revert to
version 1.0.0. Confirm it to keep it:

.. code-block:: console

   uart:~$ fota status
   uart:~$ fota confirm

The ``fota apply permanent`` variant skips the test boot and confirms the
image right away.

Automatic download
******************

Set ``CONFIG_SAMPLE_FOTA_HTTP_AUTO_URL`` to have the sample start the
download itself instead of waiting for a shell command. It registers for
the connection manager L4 connected event, so the download starts as soon
as the interface has an address, then calls
:c:func:`fota_http_download_async` and prints the progress reported by the
library thread:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/mgmt/fota_http
   :board: frdm_k64f
   :goals: build
   :west-args: --sysbuild
   :gen-args: -DCONFIG_SAMPLE_FOTA_HTTP_AUTO_URL=\"http://<host-ip>:8000/zephyr.signed.bin\"
   :compact:

On boards with Wi-Fi the download starts once ``wifi connect`` has
succeeded. The image is only stored. ``fota apply`` is still needed to
boot it. The download is skipped while the running image is still a test
image, because the secondary slot then holds the image MCUboot reverts
to, and when the secondary slot already contains the running version.
When built with the :file:`resume.conf` fragment, a transfer cut short by a
reset continues from the last saved offset after the next boot.

Resuming a download
*******************

Build with the :file:`resume.conf` Kconfig fragment to keep the download offset in
settings. After a power cut or a network error, run the download again
with the ``resume`` keyword and the transfer continues with an HTTP Range
request:

.. code-block:: console

   uart:~$ fota download http://<host-ip>:8000/zephyr.signed.bin resume

Servers that do not support Range requests answer with the full file and
the download restarts from zero.

TLS
***

Build with the :file:`tls.conf` Kconfig fragment to accept ``https`` URLs. The sample
registers the PEM certificate in :file:`src/ca_certificate.h` as the
trusted CA. It is a self-signed certificate for ``localhost`` and
``127.0.0.1`` that only exists to exercise the code path. Replace it with
the certificate of your server, or with the CA that signed it, before
serving real images.

To try it on the bench, generate a self-signed certificate for the host
address and paste the contents of :file:`server.pem` into
:file:`src/ca_certificate.h`:

.. code-block:: console

   openssl req -x509 -newkey rsa:2048 -nodes -keyout server.key \
     -out server.pem -days 365 -subj "/CN=<host-ip>" \
     -addext "subjectAltName=IP:<host-ip>"

Then serve the build directory over https from the host:

.. code-block:: python

   import http.server, ssl
   httpd = http.server.HTTPServer(("0.0.0.0", 8443),
                                  http.server.SimpleHTTPRequestHandler)
   ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
   ctx.load_cert_chain("server.pem", "server.key")
   httpd.socket = ctx.wrap_socket(httpd.socket, server_side=True)
   httpd.serve_forever()

The certificate can also be provisioned at run time instead of being built
into the image. The overlay enables the TLS credentials shell for that:

.. code-block:: console

   uart:~$ cred add 1 CA DEFAULT strt <PEM certificate>

Certificate validity dates are only checked when
:kconfig:option:`CONFIG_MBEDTLS_HAVE_TIME_DATE` is enabled, which requires
a correct wall clock on the device before the first download.
