Bluetooth: A2DP Shell
#####################

The :code:`a2dp` command exposes parts of the A2DP API.

The following examples assume that you have two devices already connected.

.. _a2dp_conn_disconn:

a2dp connection
***************
Demostrate the flow of creating a2dp connection.
 * Both sides register a2dp callbacks. using :code:`a2dp register_cb`.
 * Any side establish a2dp connection. It will create the AVDTP Signaling channel. using :code:`a2dp connect`.
 * Any side can get the acl conn. using :code:`a2dp get_conn`.
 * Any side disconnect a2dp connection. using :code:`a2dp disconnect`.

.. tabs::

        .. group-tab:: Device A (initiator)

                .. code-block:: console

                        uart:~$ a2dp register_cb
                        success
                        uart:~$ a2dp connect
                        Bonded with XX:XX:XX:XX:XX:XX
                        Security changed: XX:XX:XX:XX:XX:XX level 2
                        a2dp connected
                        uart:~$ a2dp get_conn
                        a2dp conn is: 0xXXXXXXXX
                        uart:~$ a2dp disconnect
                        a2dp disconnected

        .. group-tab:: Device B (acceptor)

                .. code-block:: console

                        uart:~$ a2dp register_cb
                        success
                        <input `a2dp connect` in initiator side>
                        Connected: XX:XX:XX:XX:XX:XX
                        Bonded with XX:XX:XX:XX:XX:XX
                        Security changed: XX:XX:XX:XX:XX:XX level 2
                        a2dp connected
                        <input `a2dp disconnect` in initiator side>
                        a2dp disconnected

.. _a2dp_basic_operations:

basic a2dp operations
*********************
Demostrate the flow of the basic a2dp operations:
 * Source and Sink sides register stream endpoints. using :code:`a2dp register_ep source sbc` and :code:`a2dp register_ep sink sbc`.
 * Create a2dp connection based on :ref:`a2dp connection <a2dp_conn_disconn>`
 * Initiator discover remote device's stream endpoints. using :code:`a2dp discover_peer_eps 0x0104`
 * Initiator configure the stream to create the stream after discover remote's endpoints. using :code:`a2dp configure`.
 * Initiator establish the stream. using :code:`a2dp establish`.
 * Sink send delay report. using :code:`a2dp send_delay_report`.
 * Initiator start the media. using :code:`a2dp start`.
 * Source test the media sending. using :code:`a2dp send_media` to send one test packet data.
 * Initiator suspend the media. using :code:`a2dp suspend`.
 * Initiator release the media. using :code:`a2dp release`.

.. note::
   Initiator is a2dp source role and acceptor is a2dp sink role in the follow logs. The delay report
   can only be sent by the sink role. The media data can only be sent by the source role.

.. tabs::

        .. group-tab:: Device A (initiator)

                .. code-block:: console

                        uart:~$ a2dp register_ep source sbc
                        SBC source endpoint is registered
                        uart:~$ a2dp discover_peer_eps 0x0104
                        endpoint id: 1, (sink), (idle):
                          codec type: SBC
                          sample frequency:
                                  44100
                                  48000
                          channel mode:
                                  Mono
                                  Stereo
                                  Joint-Stereo
                          Block Length:
                                  16
                          Subbands:
                                  8
                          Allocation Method:
                                  Loudness
                          Bitpool Range: 18 - 35
                        uart:~$ a2dp configure
                        success to configure
                        stream configured
                        uart:~$ a2dp establish
                        success to establish
                        stream established
                        <input `a2dp send_delay_report` in sink side>
                        receive delay report and accept
                        received delay report: 1 1/10ms
                        uart:~$ a2dp start
                        success to start
                        stream started
                        uart:~$ a2dp send_media
                        frames num: 1, data length: 160
                        data: 1, 2, 3, 4, 5, 6 ......
                        uart:~$ a2dp suspend
                        success to suspend
                        stream suspended
                        uart:~$ a2dp release
                        success to release
                        stream released

        .. group-tab:: Device B (acceptor)

                .. code-block:: console

                        uart:~$ a2dp register_ep sink sbc
                        SBC sink endpoint is registered
                        <input `a2dp configure` in initiator side>
                        receive requesting config and accept
                        sample rate 44100Hz
                        stream configured
                        <input `a2dp establish` in initiator side>
                        receive requesting establishment and accept
                        stream established
                        uart:~$ a2dp send_delay_report
                        success to send report delay
                        <input `a2dp start` in initiator side>
                        receive requesting start and accept
                        stream started
                        <input `a2dp send_media` in source side>
                        received, num of frames: 1, data length: 160
                        data: 1, 2, 3, 4, 5, 6 ......
                        <input `a2dp suspend` in initiator side>
                        receive requesting suspend and accept
                        stream suspended
                        <input `a2dp release` in initiator side>
                        receive requesting release and accept
                        stream released

Abort operation
***************
Demostrate the abort operation
 * Establish a2dp stream based on :ref:`basic a2dp operations <a2dp_basic_operations>`
 * Initiator abort the stream. using :code:`a2dp abort`.

.. tabs::

        .. group-tab:: Device A (initiator)

                .. code-block:: console

                        uart:~$ a2dp abort
                        success to abort
                        stream released

        .. group-tab:: Device B (acceptor)

                .. code-block:: console

                        <input `a2dp abort` in initiator side>
                        receive requesting abort and accept
                        stream released

get configuration and reconfigure operation
*******************************************
Demostrate the get configuration
 * Establish a2dp stream based on :ref:`basic a2dp operations <a2dp_basic_operations>`
 * Initiator get configuration. using :code:`a2dp get_config`.
 * Initiator reconfigure the stream. using :code:`a2dp reconfigure`.

.. tabs::

        .. group-tab:: Device A (initiator)

                .. code-block:: console

                        uart:~$ a2dp get_config
                        get config result: 0
                        sample rate 44100Hz
                        uart:~$ a2dp reconfigure
                        success to configure
                        stream configured

        .. group-tab:: Device B (acceptor)

                .. code-block:: console

                        <input `a2dp get_config` in initiator side>
                        receive get config request and accept
                        <input `a2dp reconfigure` in initiator side>
                        receive requesting reconfig and accept
                        sample rate 44100Hz
                        stream configured
