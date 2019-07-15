The DroidSansMono.ttf font used to generate the cfb_fonts bitmaps can be
obtained from:

https://android.googlesource.com/platform/frameworks/base/+/master/data/fonts/DroidSansMono.ttf

To reproduce the font bitmaps use these commands:

${ZEPHYR_BASE}/scripts/gen_cfb_font_header.py \
  -i DroidSansMono.ttf \
  -x 10 -y 16 -s 14 --center-x \
  -o cfbv_1016
${ZEPHYR_BASE}/scripts/gen_cfb_font_header.py \
  -i DroidSansMono.ttf \
  -x 15 -y 24 -s 22 --center-x --y-offset -2 \
  -o cfbv_1524
${ZEPHYR_BASE}/scripts/gen_cfb_font_header.py \
  -i DroidSansMono.ttf \
  -x 20 -y 32 -s 30 --center-x --y-offset -3 \
  -o cfbv_2032
