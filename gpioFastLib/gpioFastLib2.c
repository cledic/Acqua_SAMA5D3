/*
 * This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

/*
 * This program is written By Hardelettrosoft Team. Thanks to Giuseppe Manzoni.
 *
 * It is based on the work of Roberto Asquini for its FastIO_C_example.
 *  FastIO_C_example is based on the work of Douglas Gilbert for its mem2io.c
 *  for accessing input output register of the CPU from userspace
 *
 * Changelog - Hardlettrosoft
 * ---------------------------------
 * 2013 April 01 - First release
 * 2013 April 05 - Update comments and text formatting
 * 2013 April 08 - Update function name readGpio(..) in fastReadGpio(..)
 *
 */


#define verbose 0




#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#define DEV_MEM "/dev/mem"
#define MAP_SIZE 4096   /* needs to be a power of 2! */
#define MAP_MASK (MAP_SIZE - 1)

// addresses of interesting physical Port B registers
#define PIOB_OER  0xfffff410   // (Wr) PIO Output Enable Register -> 1 to the bit that has to be put in output
#define PIOB_ODR  0xfffff414   // (Wr) PIO Output Disable Register -> 1 to the bit that has to be put in input
#define PIOB_SODR 0xfffff430   // (Wr) PIO Set Output Data Register -> 1 to the output bit that has to be set
#define PIOB_CODR 0xfffff434   // (Wr) PIO Clear Output Data Register -> 1 to the output bit that has to be cleared
#define PIOB_ODSR 0xfffff438   // (Rd) PIO Output Data Status Register : to read the output status of the PortB pins
#define PIOB_PDSR 0xfffff43C   // (Rd) PIO Pin Data Status Register _ to read the status of the PortB input pins

int mem_fd;
void * mmap_ptr;
off_t mask_addr;

// variables to store the mapped address of the interesting registers
void * mapped_PIOB_OER_addr;
void * mapped_PIOB_ODR_addr;
void * mapped_PIOB_SODR_addr;
void * mapped_PIOB_CODR_addr;
void * mapped_PIOB_ODSR_addr;
void * mapped_PIOB_PDSR_addr;


unsigned int init_memoryToIO(void);
unsigned int close_memoryToIO(void);
void setPortBinInput(void);
void setPortBinOutput(void);
unsigned int readGeneralRegister(unsigned int reg);
unsigned int readPortBoutbits(void);
unsigned int readPortBinbits(void);
void writePortB(unsigned int uintData);
void setGpioinInput(unsigned int uintGpioB);
void setGpioinOutput(unsigned int uintGpioB);
void fastSetGpio(unsigned int uintGpioB);
void fastClearGpio(unsigned int uintGpioB);
void fastSetPB29(void);
void fastClearPB29(void);
unsigned int fastReadGpio(unsigned int uintGpioB);


// to map in a local page the peripheral address registers used
unsigned int init_memoryToIO(void) {
    mem_fd = -1;

    if ((mem_fd = open(DEV_MEM, O_RDWR | O_SYNC)) < 0) {
        printf("open of " DEV_MEM " failed");
        return 1;
    } else if (verbose) printf("open(" DEV_MEM "O_RDWR | O_SYNC) okay\n");
    mask_addr = (PIOB_OER & ~MAP_MASK);  // preparation of mask_addr (base of the memory accessed)

    if (verbose) printf ("Mask address = %08x\n",mask_addr);
    mmap_ptr = (void *)-1;
    mmap_ptr = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE,
                       MAP_SHARED, mem_fd, mask_addr);
    if (verbose) printf ("Mmap_ptr = %08x\n",mmap_ptr);

    if ((void *)-1 == mmap_ptr) {
        printf("addr=0x%x, mask_addr=0x%lx :\n", PIOB_OER, mask_addr);
        printf("    mmap");
        return 1;
    }
    if (verbose) printf("mmap() ok, mask_addr=0x%lx, mmap_ptr=%p\n", mask_addr, mmap_ptr);
    mapped_PIOB_OER_addr = mmap_ptr + (PIOB_OER & MAP_MASK);
    mapped_PIOB_ODR_addr = mmap_ptr + (PIOB_ODR & MAP_MASK);
    mapped_PIOB_SODR_addr = mmap_ptr + (PIOB_SODR & MAP_MASK);
    mapped_PIOB_CODR_addr = mmap_ptr + (PIOB_CODR & MAP_MASK);
    mapped_PIOB_ODSR_addr = mmap_ptr + (PIOB_ODSR & MAP_MASK);
    mapped_PIOB_PDSR_addr = mmap_ptr + (PIOB_PDSR & MAP_MASK);
    return 0;
}


// closing memory mapping
unsigned int close_memoryToIO(void) {
    if (-1 == munmap(mmap_ptr, MAP_SIZE)) {
        printf("mmap_ptr=%p:\n", mmap_ptr);
        printf("    munmap");
        return 1;
    } else if (verbose) printf("call of munmap() ok, mmap_ptr=%p\n", mmap_ptr);
    if (mem_fd >= 0) close(mem_fd);
    return 0;
}


// put PortB in input mode
void setPortBinInput(void) {
    *((unsigned long *)mapped_PIOB_ODR_addr) = 0xffffffff;
}


// put PortB in output mode
void setPortBinOutput(void) {
    *((unsigned long *)mapped_PIOB_OER_addr) = 0xffffffff;
}


unsigned int readGeneralRegister(unsigned int reg) {
    void * ap;
    unsigned long ul; // returns the content of the CPU register reg

    ap = mmap_ptr + (reg & MAP_MASK);
    ul = *((unsigned long *)ap);    // read the register
    if (verbose) printf("read: addr=0x%x, val=0x%x\n", reg, (unsigned int)ul);
    return (unsigned int)ul;
}


unsigned int readPortBoutbits(void) {
    unsigned long ul;    // returns the content of the register reg

    ul = *((unsigned long *)mapped_PIOB_ODSR_addr);    // read the register
    if (verbose) printf("read: addr=0x%x, val=0x%x\n", PIOB_ODSR, (unsigned int)ul);
    return (unsigned int)ul;
}


unsigned int readPortBinbits(void) {
    unsigned long ul; // returns the content of the register reg

    ul = *((unsigned long *)mapped_PIOB_PDSR_addr);     // read the register
    if (verbose) printf("read: addr=0x%x, val=0x%x\n", PIOB_PDSR, (unsigned int)ul);
    return (unsigned int)ul;
}


// write the output registers of Port B with the value "data"
void writePortB(unsigned int uintData) {
    *((unsigned long *)mapped_PIOB_SODR_addr) = uintData;
    *((unsigned long *)mapped_PIOB_CODR_addr) = ~uintData;
}


/*
Function:       void setGpioinInput(unsigned int uintGpioB)
Aim:            Put given gpio in input mode
Parameters:
                uintGpioB -> Port B bit
Return:         -
Revision:       1 - First Release
Author:         Hardelettrosoft Team, Giuseppe Manzoni
Note:           -
*/
void setGpioinInput(unsigned int uintGpioB) {
    unsigned long ulnGpio = 1 << uintGpioB;
    *((unsigned long *)mapped_PIOB_ODR_addr) = ulnGpio;
}


/*
Function:       void setGpioinOutput(unsigned int uintGpioB)
Aim:            Put given gpio in output mode
Parameters:
                uintGpioB -> Port B bit
Return:         -
Revision:       1 - First Release
Author:         Hardelettrosoft Team, Giuseppe Manzoni
Note:           -
*/
void setGpioinOutput(unsigned int uintGpioB) {
    unsigned long ulnGpio = 1 << uintGpioB;
    *((unsigned long *)mapped_PIOB_OER_addr) = ulnGpio;
}


/*
Function:       void fastSetGpio(unsigned int uintGpioB)
Aim:            Set given gpio
Parameters:
                uintGpioB -> Port B bit
Return:         -
Revision:       1 - First Release
Author:         Hardelettrosoft Team, Giuseppe Manzoni
Note:           -
*/
inline void fastSetGpio(unsigned int uintGpioB) {
    unsigned long ulnGpio = 1 << uintGpioB;
    *((unsigned long *)mapped_PIOB_SODR_addr) = ulnGpio;
}


/*
Function:       void fastClearGpio(unsigned int uintGpioB)
Aim:            Clear given gpio
Parameters:
                uintGpioB -> Port B bit
Return:         -
Revision:       1 - First Release
Author:         Hardelettrosoft Team, Giuseppe Manzoni
Note:           -
*/
inline void fastClearGpio(unsigned int uintGpioB) {
    unsigned long ulnGpio = 1 << uintGpioB;
    *((unsigned long *)mapped_PIOB_CODR_addr) = ulnGpio;
}


/*
Function:       void fastSetPB29(void)
Aim:            Set PB29 = Kernel ID 61
Parameters:     -
Return:         -
Revision:       1 - First Release
Author:         Hardelettrosoft Team, Giuseppe Manzoni
Note:           It is not really more faster than fastSetGpio(..)
*/
void fastSetPB29(void){
    *((unsigned long *)mapped_PIOB_SODR_addr) = (1<<29);
}


/*
Function:       void fastClearPB29(void)
Aim:            Clear PB29 = Kernel ID 61
Parameters:     -
Return:         -
Revision:       1 - First Release
Author:         Hardelettrosoft Team, Giuseppe Manzoni
Note:           It is not really more faster than fastClearGpio(..)
*/
void fastClearPB29(void){
    *((unsigned long *)mapped_PIOB_CODR_addr) = (1<<29);
}


/*
Function:       unsigned int fastReadGpio(unsigned int uintGpioB)
Aim:            read logic level of given gpio
Parameters:
                uintGpioB -> Port B bit
Return:         logic level
Revision:       2 - 2013/04/08 Fix function name, was readGpio
                1 - 2013/04/01 First Release
Author:         Hardelettrosoft
Note:           -
*/
unsigned int fastReadGpio(unsigned int uintGpioB){
    unsigned long ul; // returns the content of the register reg
    unsigned int uintValue;
    ul = *((unsigned long *)mapped_PIOB_PDSR_addr);     // read the register
    uintValue = ( ul & (1 << uintGpioB)) >> uintGpioB;  // mask and get single bit value
    return uintValue;
}
