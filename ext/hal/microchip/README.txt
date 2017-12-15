The CEC1702 HAL is downloaded from:
http://ww1.microchip.com/downloads/en/softwarelibrary/cec1702_sdk/cec1702_hw_blks_b0_build_0300.zip

and header files are from:
Source/hw_blks_B0/common/include

treated with:
sed -i -e 's/[ \t\r]*$//g' -e '/system_CEC1702_C0.h/d'
