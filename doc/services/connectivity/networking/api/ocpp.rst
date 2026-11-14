.. _ocpp_interface:

Open Charge Point Protocol (OCPP)
#################################

.. contents::
    :local:
    :depth: 2

Overview
********

Open Charge Point Protocol (OCPP) is an application protocol for communication
between Charge Points (Electric vehicle (EV) charging stations) and a central
management system, also known as a charging station network. OCPP is a
`standard <https://openchargealliance.org/protocols/open-charge-point-protocol/>`_
defined by The Open Charge Alliance and goal is to offer a uniform solution for
the method of communication between charge point and central system. With this
protocol it is possible to connect any central system with any charge point,
regardless of the vendor.

Zephyr provides an OCPP Charge Point (CP) library built on top of websocket API
with payload in json format. The library can be enabled with
:kconfig:option:`CONFIG_OCPP` Kconfig option. Currently OCPP 1.6 with basic
core profile is supported.

OCPP charge point (CP) require a Central System (CS) server to connect, an open
source SteVe server shall be setup locally for development purpose, See
`SteVe server <https://github.com/steve-community/steve/blob/master/README.md>`_
for more information about the setup.

The Zephyr OCPP CP library implements the following items:

* engine to process socket connectivity and events
* OCPP core functions to frame/parse payload, user notification for OCPP events,
  heartbeat notification

Sample usage
************

Init ocpp library with overall CP and CS information. Prior to init an OCPP
library, a network interface should be ready using ethernet or wifi or modem.
A filled CP, CS structure and user callback needs to be passed in ocpp_init.

.. code-block:: c

   static int user_notify_cb(ocpp_notify_reason_t reason,
                             ocpp_io_value_t *io,
                             void *user_data)
   {

        switch (reason) {
        case OCPP_USR_GET_METER_VALUE:
                ...
                break;

        case OCPP_USR_START_CHARGING:
                ...
                break;

                ...
                ...
   }

   /* OCPP configuration */
   ocpp_cp_info_t cpi = { "basic", "zephyr", .num_of_con = 1};
   ocpp_cs_info_t csi =  {"192.168.1.3",   /* ip address */
                          "/steve/websocket/CentralSystemService/zephyr",
                          8180,
                          AF_INET};

   ret = ocpp_init(&cpi, &csi, user_notify_cb, NULL);

A central system may require the charge point to authenticate itself, see
:ref:`ocpp_basic_auth`.

A unique session must open for each physical connector before any ocpp
transaction API call.

.. code-block:: c

   ocpp_session_handle_t sh = NULL;
   ret = ocpp_session_open(&sh);

idtag is EV user's authentication token which should match with list on CS.
Authorize request must call to ensure validity of idtag (if charging request
originate from local CP) before start energy transfer to EV.

.. code-block:: c

    ocpp_auth_status_t status;
    ret = ocpp_authorize(sh, idtag, &status, 500);

On successful, authorization status is available in status.

Apart from local CP, charging request may originate from CS is notified to user
in callback with OCPP_USR_START_CHARGING, here authorize request call is
optional. When the CS is ready to provide power to EV, a start transaction
is notified to CS with meter reading and connector id using ocpp_start_transaction.

.. code-block:: c

   const int idcon = 1;
   const int mval = 25; //meter reading in wh
   ret = ocpp_start_transaction(sh, mval, idcon, 200);

Once the start transaction is success, user callback is invoked to get meter
readings from the library. callback should be not be hold for longer time.

.. _ocpp_basic_auth:

HTTP Basic authentication
*************************

A central system can require the charge point to authenticate with HTTP Basic
authentication (:rfc:`7617`) in the websocket handshake, as OCPP-J 1.6
(section 6.2.2) and the security profiles 1 and 2 of the
`OCPP 1.6 security whitepaper <https://openchargealliance.org/whitepapers/>`_
define. Enable :kconfig:option:`CONFIG_OCPP_BASIC_AUTH` and fill the
credentials before calling ocpp_init:

.. code-block:: c

   /* authorization key 0001020304050607FFFFFFFFFFFFFFFFFFFFFFFF */
   static const uint8_t key[] = {
           0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0xff, 0xff,
           0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   };

   csi.ws_url = "/steve/websocket/CentralSystemService/zephyr";
   csi.auth.username = "zephyr"; /* charge point identity */
   csi.auth.password = key;
   csi.auth.password_len = sizeof(key);

* The username has to be the charge point identity, the last element of the
  websocket URL (A00.FR.204). A central system rejects the handshake
  otherwise.
* The password is the authorization key (A00.FR.205), of 16 to
  :kconfig:option:`CONFIG_OCPP_BASIC_AUTH_PASSWORD_MAX_LEN` bytes. Its bytes
  are sent as they are, as in the example of OCPP-J 1.6, and not their
  hexadecimal digits. Pass the digits, or another text, only to a central
  system that compares the password as text.
* The ``AuthorizationKey`` configuration key is not implemented, so the
  central system cannot change the password.

.. warning::

   Without TLS (security profile 1) the credentials are readable by anyone on
   the path. The whitepaper allows this in trusted networks only (A00.FR.201)
   and recommends a connection secured by other means, such as a VPN
   (A00.FR.206).

With :kconfig:option:`CONFIG_NET_SOCKETS_SOCKOPT_TLS` and the TLS credentials
in ``csi.creds``, the same credentials are sent over TLS (security profile 2).

API Reference
*************

.. doxygengroup:: ocpp_api
