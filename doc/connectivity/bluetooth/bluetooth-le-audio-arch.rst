.. _bluetooth_le_audio_arch:

LE Audio Stack
##############

.. figure:: img/ble_audio_arch.svg
   :align: center
   :alt: Bluetooth Audio Architecture

   Bluetooth Audio Architecture

Generic Audio Framework (GAF)
*****************************
The Generic Audio Framework (GAF) is considered the middleware of the Bluetooth
LE Audio architecture. The GAF contains the profiles and services that allows
higher layer applications and profiles to set up streams, change volume, control
media and telephony and more. The GAF builds on GATT, GAP and isochronous
channels (ISO).

GAF uses GAP to connect, advertise and synchronize to other devices.
GAF uses GATT to configure streams, associate streams with content
(e.g. media or telephony), control volume and more.
GAF uses ISO for the audio streams themselves, both as unicast (connected)
audio streams or broadcast (unconnected) audio streams.

GAF mandates the use of the LC3 codec, but also supports other codecs.

.. figure:: img/gaf.svg
   :align: center
   :alt: Generic Audio Framework

   Generic Audio Framework

The top-level profiles TMAP and HAP are not part of the GAF, but rather provide
top-level requirements for how to use the GAF.

GAF has been implemented in Zephyr with the following structure.

.. figure:: img/zephyr_gaf.svg
   :align: center
   :alt: Generic Audio Framework

   Zephyr Generic Audio Framework

Bluetooth Audio Stack Status
============================

The following table shows the current status and support of the profiles in the
Bluetooth Audio Stack.

.. table:: Bluetooth Audio Profile status
   :widths: auto

   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   | Module | Role                          | Version | Added in Release | Status                | Remaining                                        |
   +========+===============================+=========+==================+=======================+==================================================+
   | VCP    | Volume Renderer               | 1.0     | 2.6              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Volume Controller             | 1.0     | 2.6              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   | MICP   | Microphone Device             | 1.0     | 2.7              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Microphone Controller         | 1.0     | 2.7              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   | CSIP   | Set Member                    | 1.0.1   | 3.0              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Set Coordinator               | 1.0.1   | 3.0              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   | CCP    | Call Control Server           | 1.0     | 3.0              | - Feature complete    | - API refactor                                   |
   |        |                               |         |                  | - Shell Module        | - Sample Application                             |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Call Control Client           | 1.0     | 3.0              | - Feature complete    | - API refactor                                   |
   |        |                               |         |                  | - Shell Module        | - Sample Application                             |
   |        |                               |         |                  | - BSIM test           |                                                  |
   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   | MCP    | Media Control Server          | 1.0     | 3.0              | - Feature complete    | - API refactor                                   |
   |        |                               |         |                  | - Shell Module        | - Support for multiple instances and connections |
   |        |                               |         |                  | - BSIM test           | - Sample Application                             |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Media Control Client          | 1.0     | 3.0              | - Feature complete    | - API refactor                                   |
   |        |                               |         |                  | - Shell Module        | - Sample Application                             |
   |        |                               |         |                  | - BSIM test           |                                                  |
   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   | BAP    | Unicast Server                | 1.0.1   | 3.0              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Unicast Client                | 1.0.1   | 3.0              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Broadcast Source              | 1.0.1   | 3.0              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Broadcast Sink                | 1.0.1   | 3.0              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Scan Delegator                | 1.0.1   | 3.3              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Broadcast Assistant           | 1.0.1   | 3.3              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   | CAP    | Acceptor                      | 1.0     | 3.2              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Initiator                     | 1.0     | 3.3              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Commander                     |         |                  | - WIP                 | - Feature complete                               |
   |        |                               |         |                  |                       | - Shell Module                                   |
   |        |                               |         |                  |                       | - BSIM test                                      |
   |        |                               |         |                  |                       | - Sample Application                             |
   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   | HAP    | Hearing Aid                   | 1.0     | 3.1              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Hearing Aid Unicast Client    | 1.0     | 3.1              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Hearing Aid Remote Controller |         |                  | - WIP                 | - Feature complete                               |
   |        |                               |         |                  |                       | - Shell Module                                   |
   |        |                               |         |                  |                       | - BSIM test                                      |
   |        |                               |         |                  |                       | - Sample Application                             |
   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   | TMAP   | Call Gateway                  | 1.0     | 3.4              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Call Terminal                 | 1.0     | 3.4              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Unicast Media Sender          | 1.0     | 3.4              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Unicast Media Receiver        | 1.0     | 3.4              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Broadcast Media Sender        | 1.0     | 3.4              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Broadcast Media Receiver      | 1.0     | 3.4              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   | PBP    | Public Broadcast Source       |         | 3.5              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Public Broadcast Sink         |         | 3.5              | - Feature complete    |                                                  |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  | - Sample Application  |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Public Broadcast Assistant    |         |                  |                       | - Feature complete                               |
   |        |                               |         |                  |                       | - Shell Module                                   |
   |        |                               |         |                  |                       | - BSIM test                                      |
   |        |                               |         |                  |                       | - Sample Application                             |
   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   | GMAP   | Unicast Game Gateway          |         | 3.5              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  |                       |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Unicast Game Terminal         |         | 3.5              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  |                       |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Broadcast Game Sender         |         | 3.5              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  |                       |                                                  |
   |        +-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+
   |        | Broadcast Game Receiver       |         | 3.5              | - Feature complete    | - Sample Application                             |
   |        |                               |         |                  | - Shell Module        |                                                  |
   |        |                               |         |                  | - BSIM test           |                                                  |
   |        |                               |         |                  |                       |                                                  |
   +--------+-------------------------------+---------+------------------+-----------------------+--------------------------------------------------+

Using the Bluetooth Audio Stack
===============================

To use any of the profiles in the Bluetooth Audio Stack, including the top-level
profiles outside of GAF, :kconfig:option:`CONFIG_BT_AUDIO` shall be enabled.
This Kconfig option allows the enabling of the individual profiles inside of the
Bluetooth Audio Stack. Each profile can generally be enabled on its own, but
enabling higher-layer profiles (such as CAP, TMAP and HAP) will typically
require enabling some of the lower layer profiles.

It is, however, possible to create a device that uses e.g. only Stream Control
(with just the BAP), without using any of the content control or
rendering/capture control profiles, or vice versa. Using the higher layer
profiles will however typically provide a better user experience and better
interoperability with other devices.

Common Audio Profile (CAP)
--------------------------

The Common Audio Profile introduces restrictions and requirements on the lower layer profiles.
The procedures in CAP works on one or more streams for one or more devices. Is it thus possible via
CAP to do a single function call to setup multiple streams across multiple devices.

The figure below shows a complete structure of the procedures in CAP and
how they correspond to procedures from the other profiles. The circles with I, A and C show whether
the procedure has active involvement or requirements from the CAP Initiator, CAP Accept and CAP
Commander roles respectively.

.. figure:: img/cap_proc.svg
   :align: center
   :alt: Common Audio Profile Procedures

   Common Audio Profile Procedures

The API reference for CAP can be found in :ref:`Common Audio Profile <bluetooth_cap>`.

Stream Control (BAP)
--------------------

Stream control is implemented by the Basic Audio Profile. This profile
defines multiple roles:

* Unicast Client
* Unicast Server
* Broadcast Source
* Broadcast Sink
* Scan Delegator
* Broadcast Assistant

Each role can be enabled individually, and it is possible to support more than
one role.

Notes about the stream control services
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are 3 services primarily used by stream control using the Basic Audio Profile.

Audio Stream Control Service (ASCS)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ASCS is a service used exclusively for setting up unicast streams,
and is located on the BAP Unicast Server device.
The service exposes one or more endpoints that can either be a sink or source endpoint,
from the perspective of the Unicast Server.
That means a sink endpoint is always audio from the Unicast Client to the Unicast Server,
and a source endpoint is always from the Unicast Server to the Unicast Client.

Unlike most other GATT services,
ASCS require that each characteristic in the service has unique data per client.
This means that if a Unicast Server is connected to multiple Unicast Clients,
the Unicast Clients are not able to see or control the endpoints configured by the other clients.
For example if a person's smartphone is streaming audio to a headset,
then the same person will not be able to see or control that stream from their smartwatch.

Broadcast Audio Scan Service (BASS)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

BASS is a service that is exclusively used by the Scan Delegator and Broadcast Assistant.
The main purpose of the service is to offload scanning from low power peripherals to e.g. phones
and PCs.
Unlike ASCS where the data is required to be unique per client,
the data in BASS (called receive states) are (usually) shared among all connected clients.
That means it is possible for a person to tell their headphones to synchronize to a
Broadcast Source using their phone,
and then later tell their headphones to stop synchronizing using their smartwatch.

A Broadcast Assistant can be any device,
and may only support this one role without any audio capabilities.
This allows legacy devices that do not support periodic advertisements or isochronous channels to
still provide an interface and scan offloading for peripherals.
The Bluetooth SIG have provided a guide on how to develop such legacy Broadcast Assistants that can
be found at
https://www.bluetooth.com/bluetooth-resources/developing-auracast-receivers-with-an-assistant-application-for-legacy-smartphones/.
An important note about this guide is that many operating systems (especially on phones),
do not allow generic usage of the BASS UUID,
effectively making it impossible to implement your own Broadcast Assistant,
because you cannot access the BASS.

Published Audio Capabilities Service (PACS)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

PACS is used to expose a device's audio capabilities in Published Audio Capabilities (PAC) records.
PACS is used by nearly all roles,
where the Unicast Client and Broadcast Assistant will act as PACS clients,
and Unicast Server and Broadcast Sink will act as PACS servers.
These records contain information about the codec, and which values are supported by each codec.
The values for the LC3 codec are defined by the Bluetooth Assigned numbers
(https://www.bluetooth.com/specifications/assigned-numbers/), and the values for other codecs such
as SBC are left undefined/implementation specific for BAP.

PACS also usually share the same data between each connected client,
but by using functions such as :c:func:`bt_pacs_conn_set_available_contexts_for_conn`,
it is possible to set specific values for specific clients.

The API reference for stream control can be found in
:ref:`Bluetooth Audio <bluetooth_audio>`.


Rendering and Capture Control
-----------------------------

Rendering and capture control is implemented by the Volume Control Profile
(VCP) and Microphone Control Profile (MICP).

The VCP implementation supports the following roles

* Volume Control Service (VCS) Server
* Volume Control Service (VCS) Client

The MICP implementation supports the following roles

* Microphone Control Profile (MICP) Microphone Device (server)
* Microphone Control Profile (MICP) Microphone Controller (client)

The API reference for volume control can be found in
:ref:`Bluetooth Volume Control <bluetooth_volume>`.

The API reference for Microphone Control can be found in
:ref:`Bluetooth Microphone Control <bluetooth_microphone>`.


Content Control
---------------

Content control is implemented by the Call Control Profile (CCP) and
Media Control Profile (MCP).

The CCP implementation is not yet implemented in Zephyr.

The MCP implementation supports the following roles

* Media Control Service (MCS) Server via the Media Proxy module
* Media Control Client (MCC)

The API reference for media control can be found in
:ref:`Bluetooth Media Control <bluetooth_media>`.


Coordinated Sets
----------------

Coordinated Sets is implemented by the Coordinated Sets Identification Profile
(CSIP).

The CSIP implementation supports the following roles

* Coordinated Set Identification Service (CSIP) Set Member
* Coordinated Set Identification Service (CSIP) Set Coordinator

The API reference for media control can be found in
:ref:`Bluetooth Coordinated Sets <bluetooth_coordinated_sets>`.
