.. _hexiwear-4x4-buttons:

4x4 Buttons
###########

Overview
********

This demo reads buttons from the 4x4 button array (installed into slot 3 of
the docking station) and sets corresponding LEDs on the rotary LED click
board in slot 2.  

Building and Running
********************

This project outputs 'Hello World' to the console and then prints a label
for each time a button is pressed.   Multiple buttons can be pressed at
once, and each button pressed will be indicated.

.. code-block:: console

   $ cd samples/boards/hexiwear/4x4-buttons
   $ make 
   $ cp outdir/hexiwear_k64/zephyr.bin /daplink


Sample Output
=============

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.7.99 - BUILD: May 18 2017 21:16:37 *****          
   Hello World! arm                                                             
   SPI SPI_0 configured                                                         
   8                                                                            
   Wrote LED value 2                                                            
   C                                       
   Wrote LED value 8                       
   B                                       
   Wrote LED value 32768                   
   6                                       
   Wrote LED value 1                                                               
   7D                                                                              
   Wrote LED value 65                                                              
   D                                                                               
   Wrote LED value 64 		
