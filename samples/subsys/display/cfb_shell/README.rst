.. zephyr:code-sample:: cfb-shell-sample
   :name: Compact Framebuffer Subsystem shell module
   :relevant-api: compact_framebuffer_subsystem

   Use the CFB shell module to interact with a display.

Overview
********
This is a simple shell module that exercises displays using the Compact
Framebuffer (CFB) subsystem.

Building and Running
********************

Build the sample app by choosing the target board, for example:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/display/cfb_shell
   :board: reel_board
   :goals: build


Shell Module Command Help
=========================

.. code-block:: console

         cfb - Compact Framebuffer Subsystem shell commands
         Subcommands:
           init          : [none]
           display       : [<display_id>]
           pixel_format  : [RGB_888|MONO01|MONO10|ARGB_8888|RGB_565|BGR_565]
           get_param     : <all, height, width, ppt, rows, cols>
           get_fonts     : [none]
           set_font      : <idx>
           set_kerning   : <kerning>
           set_fgcolor   : [<red> [<green> [<blue> [<alpha>]]]
           set_bgcolor   : [<red> [<green> [<blue> [<alpha>]]]
           invert        : [<x> <y> <width> <height>]
           print         : <col: pos> <row: pos> "<text>"
           scroll        : scroll a text in vertical or horizontal direction
           draw          : drawing text
           clear         : [none]

**init**: should be called first to initialize the display.

Command example:

.. code-block:: console

         uart:~$ cfb init
         Initialized: ssd1306@3c [128x64@MONO01]

**display**: List and select which display to use.

Command example:

.. code-block:: console

         uart:~$ cfb display
         Displays:
         *  0: ssd1306@3c
            1: sh1106@3c

.. code-block:: console

         uart:~$ cfb display 1
         Display: select 1: sh1106@3c

**pixel_format**: Set display pixel format.

Command example:

.. code-block:: console

         uart:~$ cfb pixel_format MONO10
         Pixel format: MONO10

**get_param**: get the display parameters where height, width and ppt

(pixel per tile) are in pixels and the number of rows and columns. The row
position is incremented by a multiple of the ppt.

Command example:

.. code-block:: console

         uart:~$ cfb get_param all
         param: height=120
         param: width=250
         param: ppt=8
         param: rows=15
         param: cols=250

**get_fonts**: print the index, height and width in pixels of the static
defined fonts presented in the system.

Command example:

.. code-block:: console

         uart:~$ cfb get_fonts
         idx=0 height=32 width=20
         idx=1 height=24 width=15
         idx=2 height=16 width=10

**set_font**: choose the font to be used by passing the font index. Only one
font can be used at a time.

Command example:

.. code-block:: console

         uart:~$ cfb set_font 0
         Font idx=0 height=32 width=20 set

**set_kerning**: Specify the spacing between characters.

Command example:

.. code-block:: console

         uart:~$ cfb set_kerning 3

**set_fgcolor**: Set foreground color.

Command example:

.. code-block:: console

         uart:~$ cfb foreground 0xFF 0 0 0

**set_bgcolor**: Set background color.

Command example:

.. code-block:: console

         uart:~$ cfb background 0 0 0xFF 0

**invert**: invert the pixel color of the display.
It inverts the screen colors and swaps the foreground and background
olors if executed without arguments.
Reverses the image partially if you specify the start and end coordinates.
In this case, the foreground color and background color are not swapped.
Command example:

.. code-block:: console

         uart:~$ cfb invert
         Framebuffer Inverted

**print**: pass the initial column and row positions and the text in
double quotation marks when it contains spaces. If text hits the edge
of the display the remaining characters will be displayed on the next line. The
previous printed text will be overwritten.

Command example:

.. code-block:: console

         uart:~$ cfb print 60 5 ZEPHYR

**scroll**: pass the scroll direction, vertical or horizontal, the initial
column and row positions, and the text to be displayed in double quotation
marks when it contains spaces. If the text hits the edge of the display, the
remaining characters will be displayed in the next line. The text will scroll
until it hits the display boundary, last column for horizontal and last row
for vertical direction. The text passed with the scroll command will be moved
vertically or horizontally on the display.


Command example:

.. code-block:: console

         uart:~$ cfb scroll vertical 60 5 ZEPHYR

**draw**: draw text, point, line and rect.

.. code-block:: console

         draw - drawing text
         Subcommands:
           text   : <x> <x> "<text>"
           point  : <x> <y>
           line   : <x0> <y0> <x1> <y1>
           rect   : <x0> <y0> <x1> <y1>

**draw text**: Draw text.

.. code-block:: console

         uart:~$ cfb draw text 0 0 text

**draw point**: Draw point.

.. code-block:: console

         uart:~$ cfb draw point 0 0

**draw line**: Draw line.

.. code-block:: console

         uart:~$ cfb draw line 0 0 200 200

**draw rect**: Draw rectanble.

.. code-block:: console

         uart:~$ cfb draw rect 0 0 200 200

**clear**: clear the display screen.

Command example:

.. code-block:: console

         uart:~$ cfb clear
         Display Cleared
