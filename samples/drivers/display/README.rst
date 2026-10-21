.. zephyr:code-sample:: display
   :name: Display
   :relevant-api: display_interface

   Draw basic rectangles on a display device.

Overview
********

This sample uses the :ref:`display API <display_api>` to draw some basic shapes onto the display.

For most display controllers, the shapes will be rectangles whose colors and positions are chosen so
that the orientation of the LCD and correct RGB bit order can be checked. The rectangles are drawn
in clockwise order, from top left corner: red, green, blue, grey. The shade of grey changes from
black to white.

.. tip::

   If the grey looks too green or red at any point or the order of the corners is not as described
   above then the LCD may be endian swapped!

.. raw:: html

   <style>
     .zephyr-display-sample {
       position: relative;
       width: 320px;
       height: 240px;
       margin: auto;
       background: white;
       border: 1px solid #ccc;
       box-sizing: border-box;
     }

     .zephyr-display-sample .corner {
       position: absolute;
       width: 80px;
       height: 40px;
     }

     .zephyr-display-sample .tl { top: 0; left: 0; }
     .zephyr-display-sample .tr { top: 0; right: 0; }
     .zephyr-display-sample .br { bottom: 0; right: 0; }
     .zephyr-display-sample .bl {
       bottom: 0;
       left: 0;
       animation: zephyr-display-bw-toggle 3s linear infinite;
     }

     /* RGB variant */
     .zephyr-display-sample--rgb .tl { background: red; }
     .zephyr-display-sample--rgb .tr { background: lime; }
     .zephyr-display-sample--rgb .br { background: blue; }

     /* X-align variant */
     .zephyr-display-sample--xalign .bar {
       position: absolute;
       left: 0;
       right: 0;
       height: 40px;
     }
     .zephyr-display-sample--xalign .bar-top    { top: 0; background: lime; }
     .zephyr-display-sample--xalign .bar-bottom {
       bottom: 0;
       animation: zephyr-display-bw-toggle 3s linear infinite;
     }

     /* Mono variant */
     .zephyr-display-sample--mono .tl { background: black; }
     .zephyr-display-sample--mono .tr { background: #e0e0e0; }
     .zephyr-display-sample--mono .br { background: #888; }

     /* 1bpp variant */
     .zephyr-display-sample--1bpp .tl,
     .zephyr-display-sample--1bpp .tr,
     .zephyr-display-sample--1bpp .br { background: black; }
     .zephyr-display-sample--1bpp .bl {
       animation: zephyr-display-bw-toggle 0.2s steps(2, jump-none) infinite;
     }

     @keyframes zephyr-display-bw-toggle {
       from { background-color: black; }
       to   { background-color: white; }
     }
   </style>

.. tabs::

   .. tab:: RGB

      .. raw:: html

         <figure class="zephyr-display-sample-wrap">
           <div class="zephyr-display-sample zephyr-display-sample--rgb" aria-hidden="true">
             <div class="corner tl"></div>
             <div class="corner tr"></div>
             <div class="corner br"></div>
             <div class="corner bl"></div>
           </div>
           <figcaption>
             <p>
               <span class="caption-text">
                Typical RGB output at 320x240: red, green, and blue corners (clockwise from
                top left); bottom left animates from black to white. Corner sizes scale with
                resolution on real hardware.
               </span>
             </p>
           </figcaption>
         </figure>

   .. tab:: Alignment width

      On displays where it is only possible to draw full lines at a time (see
      :c:enumerator:`SCREEN_INFO_X_ALIGNMENT_WIDTH`), such as those using the
      :dtcompatible:`sharp,ls0xx` driver, the rectangles described in the RGB tab will be replaced
      with bars that take up the entire width of the display.

      Only the green and grey bars will be visible (each corner draw uses the full width, so the
      top-left and top-right draws overlap at the top, and the bottom-right and bottom-left draws
      overlap at the bottom).

      .. raw:: html

         <figure class="zephyr-display-sample-wrap">
           <div class="zephyr-display-sample zephyr-display-sample--xalign" aria-hidden="true">
             <div class="bar bar-top"></div>
             <div class="bar bar-bottom"></div>
           </div>
           <figcaption>
             <p>
               <span class="caption-text">
                Full-width bands: green at the top, animated grey at the bottom.
               </span>
             </p>
           </figcaption>
         </figure>

   .. tab:: Grayscale

      On displays using an 8-bit luminance format (for example :c:enumerator:`PIXEL_FORMAT_L_8`),
      each corner is a different grey level; the bottom-left patch still animates from black to
      white. Other multi-bit monochrome formats behave similarly (different greys per corner).

      .. raw:: html

         <figure class="zephyr-display-sample-wrap">
           <div class="zephyr-display-sample zephyr-display-sample--mono" aria-hidden="true">
             <div class="corner tl"></div>
             <div class="corner tr"></div>
             <div class="corner br"></div>
             <div class="corner bl"></div>
           </div>
           <figcaption>
             <p>
               <span class="caption-text">
                Grayscale-style corners; bottom left animates through grey levels toward white.
               </span>
             </p>
           </figcaption>
         </figure>

   .. tab:: 1 bpp

      On displays with 1 bit per pixel, the greyscale animation of the bottom rectangle (or bar)
      will appear as flickering between black and white. Other corners stay at a fixed color that
      matches the display background for the active pixel format.

      .. raw:: html

         <figure class="zephyr-display-sample-wrap">
           <div class="zephyr-display-sample zephyr-display-sample--1bpp" aria-hidden="true">
             <div class="corner tl"></div>
             <div class="corner tr"></div>
             <div class="corner br"></div>
             <div class="corner bl"></div>
           </div>
          <figcaption>
             <p>
               <span class="caption-text">
               1 bpp: three corners stay at the background color (black for <code>MONO01</code>,
               white for <code>MONO10</code> on a black screen); bottom-left toggles.
               </span>
             </p>
          </figcaption>
         </figure>

   .. tab:: E-paper

      By querying the display controller's capabilities, it is possible to determine if the display
      is an e-paper display by checking if the :c:enumerator:`SCREEN_INFO_EPD` bit is set in the
      :c:member:`display_capabilities.screen_info`.

      For such displays, the color of the bottom-left corner will change at a much slower rate than
      on a typical LCD, to align with the typical refresh rate of e-ink technologies.

Building and Running
********************

As this is a generic sample it should work with any display supported by Zephyr.

Below is an example on how to build for a :zephyr:board:`nrf52840dk` board with a
:ref:`adafruit_2_8_tft_touch_v2`.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: nrf52840dk/nrf52840
   :goals: build
   :shield: adafruit_2_8_tft_touch_v2
   :compact:

For testing purpose without the need of any hardware, the :zephyr:board:`native_sim <native_sim>`
board is also supported and can be built as follows;

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/display
   :board: native_sim
   :goals: build
   :compact:

List of Arduino-based display shields
*************************************

- :ref:`adafruit_2_8_tft_touch_v2`
- :ref:`ssd1306_128_shield`
- :ref:`st7789v_generic`
- :ref:`waveshare_epaper`
