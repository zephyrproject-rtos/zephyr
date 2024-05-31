In this test suite two instances of the SPI peripheral are connected together.
One SPI instance works as a controller, second one is configured as a peripheral.
In each test, both instances get identical configuration (CPOL, CPHA, bitrate, etc.).

Four GPIO loopbacks are required (see overlay for nrf54l15pdk for reference):
1. spi22-SPIM_SCK connected with spi21-SPIS_SCK,
2. spi22-SPIM_MISO connected with spi21-SPIS_MISO,
3. spi22-SPIM_MOSI connected with spi21-SPIS_MOSI,
4. spi22-cs-gpios connected with spi21-SPIS_CSN.
