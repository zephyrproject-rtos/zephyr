#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
// #include <zephyr/drivers/display/display_gc9x01x.h>
#include <zephyr/display/cfb.h>

void main(){
    static const struct device *dev2 = DEVICE_DT_GET(DT_NODELABEL(gc9x01_display));
    static const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

    int ret_old = 0;
    // static const struct display_driver_api api;
    struct spi_config config;
    config.operation = SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB | SPI_WORD_SET(8);
    uint16_t x_pos = 120;
	uint16_t y_pos = 120;
    struct display_buffer_descriptor desc;
    char hello[] = "hello";
    desc.buf_size = sizeof(hello);
    desc.width = sizeof(hello) / sizeof(hello[0]);
    desc.pitch = desc.width;
    desc.height = 1;
    display_blanking_off(dev2);
    // display_set_contrast(dev2,255);

    // display_set_pixel_format(dev2,
	// 			    PIXEL_FORMAT_RGB_565);

    int ret = display_write(dev2, x_pos, y_pos, &desc, hello);
    printf("GC9A01 DISPLAY\n");
}


// #include <display.h>
// #include <devicetree.h>
// #include <drivers/spi.h>
// #include <font.h>
// #include <gpio.h>
// #include <string.h>

// #define DISPLAY_NODE DT_NODELABEL(display)  // Assuming your display node is named "display"

// // Placeholder for character buffer (replace with actual font data)
// static const uint8_t font_data[5][FONT_HEIGHT] = {
//     {0x00, 0x00, 0x5F, 0x00, 0x00}, // 'h'
//     {0x00, 0x07, 0x00, 0x07, 0x00}, // 'e'
//     {0x14, 0x7F, 0x14, 0x7F, 0x14}, // 'l'
//     {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // 'l'
//     {0x00, 0x62, 0x63, 0x00, 0x00}, // 'o'
// };

// // Function to write a single character to the display
// static void write_char(const struct device *dev, char ch, int x, int y) {
//     uint8_t data[FONT_WIDTH * FONT_HEIGHT];

//     // Copy character data from font into buffer (replace with font rendering logic)
//     memcpy(data, font_data[ch - 'a'], FONT_WIDTH * FONT_HEIGHT);

//     // Write the character data to the display using the display API
//     display_write(dev, x, y, NULL, data);
// }

// // Function to write a string to the display
// static void write_string(const struct device *dev, const char *str, int x, int y) {
//     int i;

//     for (i = 0; str[i] != '\0'; i++) {
//         write_char(dev, str[i], x + (i * FONT_WIDTH), y);
//     }
// }

// int main(void) {
//     const struct device *dev = device_get_binding(DISPLAY_NODE);
//     if (!dev) {
//         return -1;
//     }

//     // Initialize the display
//     if (display_init(dev) != 0) {
//         return -1;
//     }

//     // Write "hello" to the display at position (10, 10)
//     write_string(dev, "hello", 10, 10);

//     return 0;
// }
