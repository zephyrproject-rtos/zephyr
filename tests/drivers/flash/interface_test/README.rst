Flash interface testing
#######################

These tests are designed to validate the robustness of the interface between the controller and the flash chip using pattern-based testing.
They are particularly effective for high-speed SPI interfaces (such as FlexSPI) and help identify incorrect interface configurations or pad settings.

A sequential-alternating-read-pattern test is available to help uncover issues that may occur when reading data through the AHB bus and caching is enabled.
