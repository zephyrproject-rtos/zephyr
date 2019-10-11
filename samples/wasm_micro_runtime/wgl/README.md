Introduction
==============
This sample demonstrates that a graphic user interface application in WebAssembly programming with WAMR graphic library(WGL) extension. WGL defined a WASM application API set for programming the UI applications. 

WGL implemention is based on [LittlevGL](https://github.com/littlevgl/), an open-source embedded 2d graphic library. Comparing the building the LittlevGL into WASM bytecode in the [lvgl](../lvgl) sample, WGL compiled LittlevGL source code into the WAMR runtime and defined a wrapper API for exporting to Webassembly application. These extension API's are listed in: `<wamr_root>/core/iwasm/lib/app-libs/extension/gui/wgl.h`. Currently only a small set of API's are provided and that would be extended in future.


The runtime component supports building target for Zephyr/STM Nucleo board.

Below pictures show the WASM application is running on an STM board with an LCD touch panel. When users click the blue button, the WASM application increases the counter, and the latest counter value is displayed on the top banner of the touch panel. The number on top will plus one each second, and the number on the bottom will plus one when clicked.


![WAMR UI SAMPLE](../../../../modules/lib/wasm-micro-runtime/doc/pics/vgl_demo2.png "WAMR UI DEMO")

Install required SDK and libraries
==============
- Install EMSDK
```
    https://emscripten.org/docs/tools_reference/emsdk.html
```

Prepare STM32 board and patch Zephyr
==============
Since WAMR runtime incorporated LittlevGL source code, it is recommended that RAM SIZE not less than 320KB. In our test we use nucleo_f767zi, which is not supported by Zephyr. However, nucleo_f767zi is almost the same as nucleo_f746zg, except FLASH and SRAM size. So we changed the DTS setting of nucleo_f746zg boards for a workaround. Apply below patch to Zephyr:
```
diff --git a/dts/arm/st/f7/stm32f746.dtsi b/dts/arm/st/f7/stm32f746.dtsi
index 1bce3ad..401b4b1 100644
--- a/dts/arm/st/f7/stm32f746.dtsi
+++ b/dts/arm/st/f7/stm32f746.dtsi
@@ -11,7 +11,7 @@
 
        sram0: memory@20010000 {
                compatible = "mmio-sram";
-               reg = <0x20010000 DT_SIZE_K(256)>;
+               reg = <0x20010000 DT_SIZE_K(320)>;
        };
```

Build and Run
==============

Build
--------------------------------
- Build on Linux</br>
`./build.sh`</br>
    All binaries are in "out", which contains "host_tool", "ui_app.wasm", "ui_app_lvgl_compatible.wasm" and "zephyr-build". The ui_app.wasm is programmed by WGL API while ui_app_lvgl_compatible.wasm is programmed with LittlevGL compatible API.

Run
--------------------------------
- Test on STM32 NUCLEO_F767ZI with ILI9341 Display with XPT2046 touch</br>
Hardware Connections

```
+-------------------+-+------------------+
|NUCLEO-F767ZI       | ILI9341  Display  |
+-------------------+-+------------------+
| CN7.10             |         CLK       |
+-------------------+-+------------------+
| CN7.12             |         MISO      |
+-------------------+-+------------------+
| CN7.14             |         MOSI      |
+-------------------+-+------------------+
| CN11.1             | CS1 for ILI9341   |
+-------------------+-+------------------+
| CN11.2             |         D/C       |
+-------------------+-+------------------+
| CN11.3             |         RESET     |
+-------------------+-+------------------+
| CN9.25             |    PEN interrupt  |
+-------------------+-+------------------+
| CN9.27             |  CS2 for XPT2046  |
+-------------------+-+------------------+
| CN10.14            |    PC UART RX     |
+-------------------+-+------------------+
| CN11.16            |    PC UART RX     |
+-------------------+-+------------------+
```

- Enter the out/zephyr-build directory<br/>
```
$ cd ./out/zephyr-build/
```

- Startup the board and falsh zephyr image and you would see "App Manager started." on board's terminal.<br/>
```
$ ninja flash
```


- Install WASM application to Zephyr using host_tool</br>
First, connect PC and STM32 with UART. Then install to use host_tool. Assume the UART device is /dev/ttyUSB0.</br>
`sudo ./host_tool -D /dev/ttyUSB0 -i ui_app -f ui_app.wasm`
`sudo ./host_tool -D /dev/ttyUSB0 -u ui_app`
`sudo ./host_tool -D /dev/ttyUSB0 -i ui_app2 -f ui_app_lvgl_compatible.wasm`

