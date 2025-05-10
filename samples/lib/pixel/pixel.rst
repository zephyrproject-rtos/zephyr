.. zephyr:code-sample-category:: lib_pixel
   :name: Pixel Library
   :show-listing:
   :live-search:

   These samples demonstrate how to use the Pixel processing library of Zephyr.

These samples can be used as starting point for test benches that print an input image,
perform some custom processing, and print the color image back along with the logs directly
on the terminal.

This helps debugging when other methods are not available.

The ``truecolor`` printing functions give accurate 24-bit RGB colors but slower than the
``256color`` variants.
