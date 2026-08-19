.. _ftp_client_interface:

FTP client
##########

.. contents::
   :local:
   :depth: 2

Overview
********

The FTP client library can be used to download or upload files to FTP server.

The FTP client library reports FTP control message and download data with two separate callback
functions :c:member:`ftp_client.ctrl_callback` and :c:member:`ftp_client.data_callback`.
The library can be configured to automatically send KEEPALIVE message to the server through a timer
if :kconfig:option:`CONFIG_FTP_CLIENT_KEEPALIVE_TIME` is not zero.
The KEEPALIVE message is sent periodically at the completion of the time interval indicated by the
value of :kconfig:option:`CONFIG_FTP_CLIENT_KEEPALIVE_TIME`.

Protocols
*********

The library is implemented in accordance with the :rfc:`959` specification.

Limitations
***********

The library implements only a minimal set of commands as of now.
However, new command support can be easily added.

Due to the differences in the implementation of FTP servers, the library might need customization
to work with a specific server.

API Reference
*************

.. doxygengroup:: ftp_client
