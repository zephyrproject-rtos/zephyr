.. _json_parser:

JSON Parser
###########

Overview
********

A simple sample to consume json-c parsing/tokener API and convert to
json_object.

Building and Running
********************

This application can be built and executed on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/modules/json-c
   :host-os: unix
   :board: nucleo_f429zi
   :goals: run
   :compact:

To build for another board, change "nucleo_f429zi" above to that board's name.

Sample Output
=============

.. code-block:: console

str:
---
{ "msg-type": [ "0xdeadbeef", "irc log" ],              "msg-from": { "class": "soldier", "name": "Wixilav" },          "msg-to": { "class": "supreme-commande
r", "name": "[Redacted]" },             "msg-log": [                    "soldier: Boss there is a slight problem with the piece offering to humans",         "
supreme-commander: Explain yourself soldier!",                  "soldier: Well they don't seem to move anymore...",                     "supreme-commander: Oh
 snap, I came here to see them twerk!"                  ]               }
---

jobj from str:
---
{
  "msg-type": [
    "0xdeadbeef",
    "irc log"
  ],
  "msg-from": {
    "class": "soldier",
    "name": "Wixilav"
  },
  "msg-to": {
    "class": "supreme-commander",
    "name": "[Redacted]"
  },
  "msg-log": [
    "soldier: Boss there is a slight problem with the piece offering to humans",
    "supreme-commander: Explain yourself soldier!",
    "soldier: Well they don't seem to move anymore...",
    "supreme-commander: Oh snap, I came here to see them twerk!"
  ]
}
---
