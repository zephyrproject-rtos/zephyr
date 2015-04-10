#!/usr/bin/gawk -f

#
# Copyright (c) 2012-2015 Wind River Systems, Inc.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1) Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.
#
# 2) Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# 3) Neither the name of Wind River Systems nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

# gawk script that computes true memory footprint of a VxMicro image
# from the output of "objdump -h <image.ELF>"

# IMPORTANT: uses gawk builtin function strtonum(), so won't run with plain awk

# print out headings for section info that will follow;
# initialize byte totals for text, data, and bss classes;
# disable flag that indicates image utilizes MMU;
# disable flag that causes 2nd line of section info to be processed

BEGIN {
   printf("SECTION NAME        ADDRESS     SIZE   HEX   PAD\n");

   first_section = 1;
   total_pad = 0;
   text = 0;
   data = 0;
   bss  = 0;
   kcpages = 0;
   rtpages = 0;
   kc_size = 0;
   rt_size = 0;
   nrt_size = 0;
   usaps_size= 0;
   total_size= 0;
   mmu = 0;

   searchline = 0;
   }

# process line marking the start of a "significant" section
# - look for lines with numeric first field and second field that doesn't
#   start with "." (i.e. don't want to worry about .debug_XXX and .comment)
# - for each section, capture useful data, record which page(s) it covers,
#   and print out basic section information
# - remember how much unused page space is left at the end of the section
#   (note: only need to keep this info for the final section) Bernie: Not True.
# - enable flag to cause additional section info on next line to be processed

/^\ *[0-9]+\ */ && (substr($2,1,1) != ".") \
   { 
   kc = 0;
   rt = 0;
   nrt = 0;
   usaps = 0;

   phyaddr = strtonum("0x" $5);
   addr = strtonum("0x" $4);
   size = strtonum("0x" $3);
   section = $2;

   if (match($2,/kc/)) {
      kc = 1;	
      kc_size += size;
    } else
   if (match($2,/rt/)) {
      rt = 1;	
      rt_size += size;
    } else
   if (match($2,/usaps/)) {
      usaps_size += size;
      usaps = 1;
    } else {
      nrt = 1;	
      nrt_size += size;
    } 

   total_size += size;
   if ((prev_section_pad = addr - endaddr) > 4096)
      prev_section_pad = (4096 - (endaddr % 4096)) % 4096;
   
   endaddr = addr + size;

   startpage = int(addr / 4096);
   endpage = int((endaddr - 1) / 4096);

   numpages = 0;
   for (pagenum = startpage; pagenum <= endpage; pagenum++) {
      pageset[pagenum] = 1;
      if (kc==1)
        kcpageset[pagenum] = 1;
      if (rt==1)
        rtpageset[pagenum] = 1;
      if (nrt==1)
        nrtpageset[pagenum] = 1;
      if (usaps==1)
        usapspageset[pagenum] = 1;
      numpages++;
   }

   if (first_section == 1) {
     first_section = 0;
     printf("%-16s 0x%08x %8d %5x ", section, phyaddr, size, size);
    }
   else {
     printf("%5d\n%-16s 0x%08x %8d %5x ", prev_section_pad, section, phyaddr, size, size);
     total_pad += prev_section_pad;
    }
    
# look for section that requires page alignment
# - set MMU flag to indicate MMU is being used by image
# - Include any unused memory from last section.

   if (match($7,/2**12$/)) {
   mmu = 1;
   }
   unused = (endpage + 1) * 4096 - endaddr;


   searchline = NR + 1;
   }

# process 2nd line of info for a significant section
# - add size info for section into the appropriate class total.
#   based on the attributes of the section

NR == searchline {
   if ($0 ~ "CODE")
      text += size;
   else if ($0 ~ "DATA")
      data += size;
   else
      bss  += size;
   }


# print out overall memory requirements for image
# - output is given in a similar format to that used by "size", but also
#   gives info on memory used for padding (which "size" doesn't account for)
# - note: doesn't report unused padding at the end of an MMU-less image,
#   since that memory doesn't really have to be present for the image to run

END {
   dec = text + data + bss;

   numpages = 0;
   numkcpages = 0;
   numrtpages = 0;
   numusapspages = 0;
   for (pagenum in pageset) {
      numpages++;
      if (kcpageset[pagenum] == 1)
	numkcpages++;
      else if (rtpageset[pagenum] == 1)
         numrtpages++;
      else if (nrtpageset[pagenum] == 1)
         numnrtpages++;
      else if (usapspageset[pagenum] == 1)
         numusapspages++;
      else printf("ERROR/n");
   }

   total = numpages * 4096;
   pad = total - dec;

   # Include unused memory from last section only if MMU is enabled
   if (mmu) 
      prev_section_pad = (4096 - (endaddr % 4096)) % 4096;
   else {
      prev_section_pad = 0;
      total -= unused;
      pad   -= unused;
   }

   total_pad += prev_section_pad;
   kernel_size = total_size-usaps_size;

   printf("%5d\n", prev_section_pad);

   printf("\n   text    data     bss     dec     hex     pad     dec     hex\n%7d %7d %7d %7d %7x %7d %7d %7x\n",
          text, data, bss, dec, dec, pad, total, total); 
   printf("\n\t      KC\tRT\tNRT\tUSAPS\tKERNEL\tTOTAL\tKERNEL+PAD");
   printf("\n  PAGES: %7d %9d %8d  %8d  %7d%7d",
	  numkcpages, numrtpages, numnrtpages, numusapspages, numpages-numusapspages, numpages);
   printf("\n  SIZES: %7d %9d %8d  %8d  %7d%7d %7d\n",
	  kc_size, rt_size, nrt_size, usaps_size, kernel_size, total_size, kernel_size+total_pad);
   }
