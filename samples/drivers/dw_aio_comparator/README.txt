This test is used to test the DesignWare AIO/Comparator. The voltage input
pin is analog in A0 on circuit board, which maps to AIN[10] on chip.

To test:
1. Connect the A0 pin to ground via a resistor (to limit current).
   Any larger then 1k Ohm would be fine.
2. Turn on the device.
3. Wait for device to boot, until "app started" line appeared.
4. Remove resistor between A0 pin and ground.
   The line "*** A0, AIN[10] triggered rising." should appear.
5. Reconnect the resistor.
   The line "*** A0, AIN[10] triggered falling." should appear.
6. Keep removing/inserting the resistor to your heart's content.
