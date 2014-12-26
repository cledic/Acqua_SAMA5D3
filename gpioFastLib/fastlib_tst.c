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
 * It improves the work of Roberto Asquini about its FastIO_C_example
 *  for set up and down one pin of Aria G25.
 * FastIO_C_example is based on the work of Douglas Gilbert for its mem2io.c
 *  for accessing input output register of the CPU from userspace
 *
 * Changelog - Hardlettrosoft
 * ---------------------------------
 * 2013 April 01 - First release
 * 2013 April 05 - Update comments and text formatting
 *
 *
 */

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "gpioFastLib2.h"


#define PIN 29 // PB29 = Kernel ID 61

struct timespec sleepTime;
struct timespec returnTime;

unsigned int time_to_wait=50;

int main(){

        sleepTime.tv_sec = 0;
        sleepTime.tv_nsec = time_to_wait*1000;

        printf("\nfastGpioLibGenericClkExample.c example program for AriaG25\n");
        printf("by Hardelettrosoft Team. Thanks to Giuseppe Manzoni\n\n");
        printf("\nSpeed test using HR Timer; time to wait set to: %d us\n", time_to_wait);
        if (init_memoryToIO()) {
                printf ("Error in init_memoryToIO() \n");
                return 1;
        }

        setGpioinOutput(PIN);

        for(;;){
                // 4xfastSetGpio == 500ns (without -O3!)
                fastSetGpio(PIN);

                // Settinng the time to wait value at 50us the resulting time on the scope was
                // 126ms. I achivied this wrong result using both function: nanosleep and usleep.
#if 1
                nanosleep(&sleepTime, &returnTime);
#else
                usleep( time_to_wait);
#endif

                fastClearGpio(PIN);

#if 1
                nanosleep(&sleepTime, &returnTime);
#else
                usleep( time_to_wait);
#endif
        }
}
