.. zephyr:code-sample:: cfb-shell-sample
   :name: Compact Framebuffer shell module
   :relevant-api: compact_framebuffer

   Use the CFB shell module to interact with a display.

Overview
********
This is a simple shell module that exercises displays using the Compact
Framebuffer(CFB) subsystem.

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

         cfb - Compact Framebuffer shell commands
         Options:
                 -h, --help  :Show command help.
         Subcommands:
           init         : [[<devname>] <pixfmt> [<xferbuf_size> [<cmdbuf_size>]]]
           get_device   : [none]
           get_param    : <all, height, width, ppt, rows, cols>
           get_fonts    : [none]
           set_font     : <idx>
           set_kerning  : <kerning>
           invert       : [<x> <y> <width> <height>]
           print        : <col: pos> <row: pos> "<text>"
           scroll       : scroll a text in vertical or horizontal direction
           draw         : drawing text
           clear        : [none]
           foreground   : <red> <green> <blue> <alpha>
           background   : <red> <green> <blue> <alpha>

**init**: should be called first to initialize the display.
You can specify the display pixel format, buffer size, and select device
with arguments.

Command example:

.. code-block:: console

         uart:~$ cfb init
         Framebuffer initialized: SSD16XX
         Display Cleared

**get_device**: prints the display device name.

Command example:

.. code-block:: console

         uart:~$ cfb get_device
         Framebuffer Device: SSD16XX

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

**foreground**: set foreground color.

Command example:

.. code-block:: console

         uart:~$ cfb foreground 0xFF 0 0 0

**background**: set background color.

Command example:

.. code-block:: console

         uart:~$ cfb background 0xFF 0 0 0
