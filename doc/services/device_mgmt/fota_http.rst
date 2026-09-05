.. _fota_http:

Firmware Over-the-Air over HTTP
###############################

Overview
********

The FOTA HTTP library downloads a firmware image from an HTTP server and
writes it straight into the MCUboot slot that is not currently running.
It then lets the application request the swap, confirm the new image or
erase the slot. Unlike :ref:`hawkBit or UpdateHub <ota>`, no device
management protocol is involved: any web server able to serve a file can
act as the update source, which makes it a good fit for simple deployments
and for bench testing.

The image is streamed to flash through the :ref:`flash_img_api` as it
arrives, so its size is not limited by RAM. The slot is erased
progressively during the write, and the MCUboot trailer of the slot is
cleared before the first byte lands, so a leftover from a previous image
can never be mistaken for a pending swap.

After the download the library reads the MCUboot header back from flash
and fails with ``-ENOEXEC`` when the data is not an image. It does not
verify the signature: MCUboot does that before the image is ever booted,
and that check decides whether the image runs. An optional SHA-256
comparison against a digest supplied by the caller is available with
:kconfig:option:`CONFIG_FOTA_HTTP_SHA256_CHECK` to catch a corrupted
transfer before the reboot.

Update cycle
************

The typical sequence is:

#. :c:func:`fota_http_download` or :c:func:`fota_http_download_async`
   fetches the image into the secondary slot.
#. :c:func:`fota_http_apply` marks the image for a test boot, or as
   permanent.
#. The device reboots and MCUboot swaps the image in.
#. On a test boot the application validates itself and calls
   :c:func:`fota_http_confirm`. Without that call, the next reboot makes
   MCUboot revert to the previous image.

The revert step requires an MCUboot mode with revert support, such as
swap-using-move or swap-using-offset. In overwrite-only mode the previous
image is gone as soon as the swap happens and confirmation has no effect,
so check which mode the board's sysbuild configuration selects.

While a test image runs, the secondary slot holds the image MCUboot would
revert to, so :c:func:`fota_http_download` refuses with ``-EPERM`` until
:c:func:`fota_http_confirm` has been called.

Threads and stack
*****************

:c:func:`fota_http_download` blocks the calling thread for the whole
transfer and needs enough stack for the HTTP client and, with https, the
TLS handshake. :c:func:`fota_http_download_async` runs the same transfer
on a thread owned by the library, sized by
:kconfig:option:`CONFIG_FOTA_HTTP_THREAD_STACK_SIZE`, and reports the
result through a completion callback. The shell command uses the
asynchronous variant so the shell stack size does not matter.

Only one download runs at a time. :c:func:`fota_http_cancel` aborts the
transfer in progress when the next fragment arrives or the request times
out.

Redirects and resume
********************

Responses with status 301, 302, 303, 307 or 308 are followed up to
:kconfig:option:`CONFIG_FOTA_HTTP_MAX_REDIRECTS` times. Both absolute and
path-only ``Location`` headers are accepted.

With :kconfig:option:`CONFIG_FOTA_HTTP_RESUME` the download offset is saved
in settings while the transfer runs and when it fails. A later download
with the ``resume`` parameter set continues from that offset with an HTTP
``Range`` request. The offset is only reused when the URL and image index
match the interrupted download, and when the server sent an ``ETag`` or
``Last-Modified`` header the request carries ``If-Range`` with it, so a
file that changed on the server is fetched again from the start. A server
that ignores the ``Range`` header and answers with the full file restarts
the transfer from zero, and so does a ``416 Range Not Satisfiable`` answer,
which is what a server sends when the file grew shorter than the saved
offset. A validator too long to store also drops the saved progress, so the
next download starts from the beginning rather than resuming against a file
that may have changed. The offset is cleared once the image is complete.

Multiple images
***************

In a layout with more than one updateable image, the ``image_index``
parameter selects which secondary slot the download goes to.
:c:func:`fota_http_apply` and :c:func:`fota_http_confirm` both take the
same index, so a layout that carries firmware for a second device confirms
only what this device validated. When several images must run together,
download and apply every one of them before rebooting: MCUboot validates the
whole set and swaps it in one go, and confirm each one afterwards, because an
image left unconfirmed is reverted on its own.
:c:func:`fota_http_confirm_pending` only reports image 0, so an application
that updates several images tracks the rest itself. A downgrade check with
:kconfig:option:`CONFIG_FOTA_HTTP_REJECT_DOWNGRADE` only covers image 0.

With direct-XIP bootloaders the slot an image runs from is fixed at link
time, so the device must fetch the image variant built for the free slot.
:c:func:`fota_http_upload_slot` reports which slot that is. This only
covers single-image layouts: for every image after the first the library
always uploads to the odd-numbered slot of the pair, which is correct for
the swap and overwrite modes but not for direct-XIP.

Downgrade protection
********************

:kconfig:option:`CONFIG_FOTA_HTTP_REJECT_DOWNGRADE` compares the version in
the downloaded header with the running image and fails the download when
it is older. This is a convenience check for the application. An enforced
policy belongs in the bootloader, see the MCUboot downgrade prevention and
hardware rollback protection options.

TLS
***

``https`` URLs are accepted when :kconfig:option:`CONFIG_FOTA_HTTP_TLS` is
enabled. The CA certificate of the server is looked up under
:kconfig:option:`CONFIG_FOTA_HTTP_TLS_SEC_TAG`, or under the tags passed in
the download parameters, and can be registered with
:c:func:`fota_http_tls_add_ca` or directly with
:c:func:`tls_credential_add`. For mutual TLS, register the client
certificate and key with :c:func:`fota_http_tls_add_client_cert`.

Peer verification is required by default and
:kconfig:option:`CONFIG_FOTA_HTTP_TLS_PEER_VERIFY` should stay at its
default in production. The host name from the URL is verified against the
certificate. When the URL carries an IP address and the certificate has no
matching entry, pass the expected name in the ``tls_hostname`` parameter.

Certificate validity dates are only enforced when
:kconfig:option:`CONFIG_MBEDTLS_HAVE_TIME_DATE` is enabled, which requires
a correct wall clock on the device before the first download. TLS 1.3 can
be selected with :kconfig:option:`CONFIG_FOTA_HTTP_TLS_VERSION_1_3`.

Shell commands
**************

With :kconfig:option:`CONFIG_FOTA_HTTP_SHELL` enabled, a ``fota`` command
exposes the library from the console:

.. code-block:: console

   fota download <url> [resume] [image=<n>]   download an image
   fota cancel                                abort the download in progress
   fota apply [permanent] [image=<n>]         boot the downloaded image on the next reset
   fota confirm [image]                       confirm the running image
   fota status                                show images and confirmation state
   fota erase [image]                         erase the secondary slot

See the :zephyr:code-sample:`fota-http` sample for a complete walkthrough.

API Reference
*************

.. doxygengroup:: fota_http
