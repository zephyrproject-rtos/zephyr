.. zephyr:code-sample:: mesh_badge
   :name: Bluetooth Mesh badge
   :relevant-api: monochrome_character_framebuffer bt_mesh bt_gatt bluetooth

   Implement a smart badge using the reel board and Bluetooth Mesh.

Overview
********

This sample app for the reel board showcases Bluetooth Mesh

The app starts off as a regular Bluetooth GATT peripheral application.
Install the "nRF Connect" app on your phone (available both for
Android and iOS) to access the service that the app exposes. The service
can also be accessed with any Bluetooth LE GATT client from your PC,
however these instructions focus on the necessary steps for phones.

Steps to set up
***************

#. On your phone, use the nRF Connect app to Scan for devices and look
   for "reel board"
#. Connect to the device. You'll see a single service - select it
#. Request to write to the characteristic by pressing on the upward pointing
   arrow symbol
#. Select "Text" to enter text instead of hex
#. Enter your name (or any other arbitrary text). Multiple words
   separated by spaces are possible. The font used on the reel display
   allows three rows of up to 12 characters
   wide text. You can force line breaks with a comma.
#. Press "Send" - this will trigger pairing since this is a protected
   characteristic. The passkey for the pairing will be shown on the board's
   display. Enter the passkey in your phone.
#. Once pairing is complete the board will show the text you sent. If
   you're not happy with it you can try writing something else.
#. When you're happy with the text, disconnect from the board (exit the app or
   go back to the device scan page)
#. Once disconnected the board switches over to Bluetooth Mesh mode, and you
   can't connect to it anymore over GATT.

If you configure multiple boards like this they can communicate with
each other over mesh: by pressing the user button on the board the first
word (name) of the stored text will be sent to all other boards in
the network and cause the other boards to display "<name> says hi!".

To reset a board to its initial state (disable mesh, erase the stored
text, and make it connectable over GATT):

#. Keep the user button pressed when powering on (or press the reset button
   when powered)
#. Wait until "Resetting Device" is shown
