.. _bluetooth_audio_stream:

Stream Control API
##################

The Bluetooth Audio Stream Control API provides functionality for managing the
audio streams.

Unicast
*******

Unicast topology :kconfig:`CONFIG_BT_AUDIO_UNICAST` uses Isochronous Channels
:kconfig:`CONFIG_BT_ISO` to establish a point-to-point connection.

Unicast depends in two services, Published Audio Capabilities Service (PACS)
:kconfig:`CONFIG_BT_PACS` which exposes the audio codec capabilities and Audio
Stream Control Service (ASCS) :kconfig:`CONFIG_BT_ASCS` that exposes Audio
Stream Endpoints (ASE) which can be used to configure audio streams.

Shell
*****

Basic Audio Profile (BAP) :kconfig:`CONFIG_BT_BAP` shell module implements
commands corresponding to the procedures available in the BAP specification.
TODO: Add link to BAP specification.

.. toctree::
   :maxdepth: 1

   ../shell/bap.rst

API reference
**************

.. doxygengroup:: bt_audio
   :project: Zephyr
   :members:
