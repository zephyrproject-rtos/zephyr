.. _moving-to-west:

Moving to West
##############

To convert a "pre-west" Zephyr setup on your computer to west, follow these
steps. If you are starting from scratch, use the :ref:`getting_started`
instead. See :ref:`west-troubleshooting` for advice on common issues.

#. Install west.

   On Linux::

     pip3 install --user -U west

   On Windows and macOS::

     pip3 install -U west

   For details, see :ref:`west-install`.

#. Move your zephyr repository to a new :file:`zephyrproject` parent directory,
   and change directory there.

   On Linux and macOS::

     mkdir zephyrproject
     mv zephyr zephyrproject
     cd zephyrproject

   On Windows ``cmd.exe``::

     mkdir zephyrproject
     move zephyr zephyrproject
     chdir zephyrproject

   The name :file:`zephyrproject` is recommended, but you can choose any name
   with no spaces anywhere in the path.

#. Create a :ref:`west installation <west-installation>` using the zephyr
   repository as a local manifest repository::

     west init -l zephyr

   This creates :file:`zephyrproject/.west`, marking the root of your
   installation, and does some other setup. It will not change the contents of
   the zephyr repository in any way.

#. Clone the rest of the repositories used by zephyr::

     west update

   **Make sure to run this command whenever you pull zephyr.** Otherwise, your
   local repositories will get out of sync. (Run ``west list`` for current
   information on these repositories.)

You are done: :file:`zephyrproject` is now set up to use west.
