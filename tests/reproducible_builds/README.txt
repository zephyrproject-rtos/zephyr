Title: Test framework to validate reproducible binary

Description:
This script verify that builds are reproducible, i.e. producing exactly
the same binary if built using the same source code repository.
Validating the binary generated in below scenario:
- build generated in the same directory
- build generated in different directories
- Enable “TIMER_RANDOM_GENERATOR" config and generate the build

Usage:
Execute sh ./check_reproducible_build.sh  which will clone the latest Zephyr
source code repository and generate the binary in above mentioned scenario.
And further it will compare all the binaries using diffoscope tool.

Sample Output:
checking reproducible build from the same directory

checking reproducible build from the different directory
Bytes in section header mismatch
Bytes in section header of elf file1 and file2 are 585232 and  548764 respectively
Number of section headers mismatch
Number of section headers of elf file1 and file2 are 28 and  27 respectively
Entries in symbol table mismatch
Entries in Symbol table of elf file1 and file2 are 981 and  940 respectively

checking reproducible build with CONFIG_TIMER_RANDOM_GENERATOR
Bytes in section header mismatch
Bytes in section header of elf file1 and file2 are 548760 and  548764 respectively
