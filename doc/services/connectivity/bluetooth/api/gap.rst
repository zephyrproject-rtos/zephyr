.. _bt_gap:

Generic Access Profile (GAP)
############################

Resolvable Private Address rotation
***********************************

With :kconfig:option:`CONFIG_BT_PRIVACY` the Host gives advertising sets, the scanner and the
initiator Resolvable Private Addresses (RPAs) and rotates them all together when one shared timer
expires, every :kconfig:option:`CONFIG_BT_RPA_TIMEOUT` seconds by default. The rotations can stop
while no advertising set, active scanner or initiator is running.

At a rotation, the Host calls the :c:member:`bt_le_ext_adv_cb.rpa_expired` callback of an
advertising set before giving it a new RPA. With a Controller that supports extended advertising the
set is paused at that point, so data set with :c:func:`bt_le_ext_adv_set_data` from the callback
goes out with the new RPA. Returning ``false`` keeps the current RPA until the next rotation. The
callback is also called for a set that is not advertising, as long as it holds a valid RPA, which it
gets when it is created or started. A set whose callback lets the RPA go while it is not advertising
gets a new RPA when it is next started. A set with a duration or event limit keeps its RPA until it
stops and gets the callback then, possibly while it is still advertising.

An application whose advertising data changes on its own schedule can align the rotations with it.
With :kconfig:option:`CONFIG_BT_RPA_TIMEOUT_DYNAMIC`, calling :c:func:`bt_le_set_rpa_timeout` from
the callback of a rotation sets the time to the next one; computing it from the application's own
clock at every rotation keeps the two in step. :c:func:`bt_le_rotate_rpa` rotates the RPAs
immediately, for example before starting a set.

API Reference
*************

.. doxygengroup:: bt_gap

.. doxygengroup:: bt_addr

.. doxygengroup:: bt_gap_defines
