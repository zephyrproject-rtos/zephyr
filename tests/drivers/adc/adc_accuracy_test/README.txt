ADC accuracy test

This test checks that ADC readings match an expected value. It is
done using two approaches:

 - DAC source: a board DAC pin is set to a known value, which is then
 read on an ADC one. If they match, the test passes.

 - Reference voltage: an ADC channel is read and compared to an expected
 value.

For the DAC source, it is expected that DAC and ADC are connected. This
can be indicated for twister runs by setting the fixture "dac_adc_loop".
The test then sets DAC to half its resolution and reads the ADC to see
if they match. Note that DAC and ADC are expected to generate/read
voltage on the same range.

In the reference voltage case, the ADC is expected to be connected to a
known voltage reference, whose value is informed, in millivolts, at
property "reference_mv" from "zephyr,user" node. The test reads the ADC
to see if they match.
