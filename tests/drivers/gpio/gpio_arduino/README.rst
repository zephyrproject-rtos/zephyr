Test GPIO From ARDUINO interface
################################

This test is to test gpio with unified interface pins
so any arduino compatible boards does not need code change

Below apis can be tested in real boards

* gpio config
* gpio set/get
* gpio interrupt and callback

Requirements:
#############

* Arduiono D0 and D1 are shorted by fly wire

* add arduino_gpio in your hardware map as fixture
