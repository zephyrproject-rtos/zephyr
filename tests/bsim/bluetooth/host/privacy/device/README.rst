.. _bsim_test_bt_privacy:

Privacy feature test
********************

Tests goals
===========

The tests check the following aspects of the Bluetooth privacy feature:

* In device privacy mode, a scanner shall accept advertising packets from peers with any address
  type. That, even if they have previously exchanged IRK; (Core Specification version 5.4, vol. 1
  part A, 5.4.5.)
* After devices have exchanged IRK, they must correctly resolve RPA when receiving packets from the
  peer.

For references, see the Bluetooth Core specification version 5.4, vol. 1, part A, 5.4.5 and vol. 6,
part B, 6.
