The MEC1701 HAL was downloaded from:

https://github.com/MicrochipTech/Peripheral-mec1701

Massaged the file with :
sed -i -e 's/[ \t\r]*$//g' MCHP_MEC1701_bit_fields.h, then file was renamed
to MCHP_MEC1701.h

Field EOF was renamed to PEOF as it was collapsing with EOF defined in stdio.h
