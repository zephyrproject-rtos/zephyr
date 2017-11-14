/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2003 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
******************************************************************************/

#include "alt_types.h"

#include "altera_avalon_spi_regs.h"
#include "altera_avalon_spi.h"

/* This is a very simple routine which performs one SPI master transaction.
 * It would be possible to implement a more efficient version using interrupts
 * and sleeping threads but this is probably not worthwhile initially.
 */

int alt_avalon_spi_command(alt_u32 base, alt_u32 slave,
                           alt_u32 write_length, const alt_u8 * write_data,
                           alt_u32 read_length, alt_u8 * read_data,
                           alt_u32 flags)
{
  const alt_u8 * write_end = write_data + write_length;
  alt_u8 * read_end = read_data + read_length;

  alt_u32 write_zeros = read_length;
  alt_u32 read_ignore = write_length;
  alt_u32 status;

  /* We must not send more than two bytes to the target before it has
   * returned any as otherwise it will overflow. */
  /* Unfortunately the hardware does not seem to work with credits > 1,
   * leave it at 1 for now. */
  alt_32 credits = 1;

  /* Warning: this function is not currently safe if called in a multi-threaded
   * environment, something above must perform locking to make it safe if more
   * than one thread intends to use it.
   */

  IOWR_ALTERA_AVALON_SPI_SLAVE_SEL(base, 1 << slave);
  
  /* Set the SSO bit (force chipselect) only if the toggle flag is not set */
  if ((flags & ALT_AVALON_SPI_COMMAND_TOGGLE_SS_N) == 0) {
    IOWR_ALTERA_AVALON_SPI_CONTROL(base, ALTERA_AVALON_SPI_CONTROL_SSO_MSK);
  }

  /*
   * Discard any stale data present in the RXDATA register, in case
   * previous communication was interrupted and stale data was left
   * behind.
   */
  IORD_ALTERA_AVALON_SPI_RXDATA(base);
    
  /* Keep clocking until all the data has been processed. */
  for ( ; ; )
  {
    
    do
    {
      status = IORD_ALTERA_AVALON_SPI_STATUS(base);
    }
    while (((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) == 0 || credits == 0) &&
            (status & ALTERA_AVALON_SPI_STATUS_RRDY_MSK) == 0);

    if ((status & ALTERA_AVALON_SPI_STATUS_TRDY_MSK) != 0 && credits > 0)
    {
      credits--;

      if (write_data < write_end)
        IOWR_ALTERA_AVALON_SPI_TXDATA(base, *write_data++);
      else if (write_zeros > 0)
      {
        write_zeros--;
        IOWR_ALTERA_AVALON_SPI_TXDATA(base, 0);
      }
      else
        credits = -1024;
    };

    if ((status & ALTERA_AVALON_SPI_STATUS_RRDY_MSK) != 0)
    {
      alt_u32 rxdata = IORD_ALTERA_AVALON_SPI_RXDATA(base);

      if (read_ignore > 0)
        read_ignore--;
      else
        *read_data++ = (alt_u8)rxdata;
      credits++;

      if (read_ignore == 0 && read_data == read_end)
        break;
    }
    
  }

  /* Wait until the interface has finished transmitting */
  do
  {
    status = IORD_ALTERA_AVALON_SPI_STATUS(base);
  }
  while ((status & ALTERA_AVALON_SPI_STATUS_TMT_MSK) == 0);

  /* Clear SSO (release chipselect) unless the caller is going to
   * keep using this chip
   */
  if ((flags & ALT_AVALON_SPI_COMMAND_MERGE) == 0)
    IOWR_ALTERA_AVALON_SPI_CONTROL(base, 0);

  return read_length;
}

