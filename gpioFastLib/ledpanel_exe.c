/*
 * ledpanel2
 *
 * Bit banging Linux driver for RGB 32x32 led panel
 * Optimized version with brightness control
 *
 * (c) 2014 Sergio Tanzilli - tanzilli@acmesystems.it
 * http://www.acmesystems.it/ledpanel
 *
 */

#include <stdio.h>
#include <string.h>
#include "gpioFastLib2.h"

#define LEDPANEL_R0             0
#define LEDPANEL_G0             1
#define LEDPANEL_B0             2
#define LEDPANEL_R1             3
#define LEDPANEL_G1             4
#define LEDPANEL_B1             5
#define LEDPANEL_A              6
#define LEDPANEL_B              7
#define LEDPANEL_C              8
#define LEDPANEL_D              9
#define LEDPANEL_CLK    10
#define LEDPANEL_STB    11
#define LEDPANEL_OE             12
#define LEDPANEL_TP             13

#define MAXBUFFER_PER_PANEL 32*32*3

#define TOP_BYTE_ARRAY          0
#define BOTTOM_BYTE_ARRAY       32*16*3

#define COLOR_MASK      7

#define A_PPOS  25

static int newframe=0;
static int ledpanel_row=0;
static int pbuffer_top=TOP_BYTE_ARRAY;
static int pbuffer_bottom=BOTTOM_BYTE_ARRAY;

static unsigned char rgb_buffer[MAXBUFFER_PER_PANEL];
static unsigned char rgb_buffer_copy[MAXBUFFER_PER_PANEL];

// GPIO lines used
static char ledpanel_gpio[] = {
        12, // 44, // R0
        13, // 45, // G0
        14, // 46, // B0

        15, // 47, // R1
        16, // 48, // G1
        17, // 49, // B1

        25, // 57, // A
        26, // 58, // B
        27, // 59, // C
        28, // 60, // D

         8, // 40, // CLK
         9, // 41, // STB
        10, // 42, // OE
        29, // 61, // TP
};

void configureIO( void);
unsigned int ledpanel_redraw( void);

int main( void)
{
        int i;
        unsigned char r, g, b;

        if (init_memoryToIO()) {
                printf ("Error in init_memoryToIO() \n");
                return 1;
        }
        //
        configureIO( );

        memset(rgb_buffer,MAXBUFFER_PER_PANEL,0);

        r=0;
        g=0;
        b=7;

        for ( i=0; i<MAXBUFFER_PER_PANEL;i+=3) {
                rgb_buffer[i+0]=r;              // R
                rgb_buffer[i+1]=g;              // G
                rgb_buffer[i+2]=b;              // B
        }

        memcpy(rgb_buffer_copy,rgb_buffer,MAXBUFFER_PER_PANEL);

        newframe=1;

        while( 1) {
                ledpanel_redraw();
                asm("");
        }

        return 0;
}

void configureIO( void)
{
        int i;

        for (i=0; i<sizeof(ledpanel_gpio);i++){
                setGpioinOutput( ledpanel_gpio[i]);
        }
}

// Send a new row the panel
unsigned int ledpanel_redraw( void){
        int col;
        int i;

        fastSetGpio( ledpanel_gpio[LEDPANEL_TP]);

        if (newframe==1) {
                newframe=0;
                ledpanel_row=0;
                pbuffer_top=TOP_BYTE_ARRAY;
                pbuffer_bottom=BOTTOM_BYTE_ARRAY;
        }

        for (col=0;col<32;col++) {

                //RED 0
                if (rgb_buffer_copy[pbuffer_top]==0) {
                        fastClearGpio( ledpanel_gpio[LEDPANEL_R0]);
                } else {
                        if (rgb_buffer_copy[pbuffer_top]<=rgb_buffer[pbuffer_top]) {
                                fastSetGpio( ledpanel_gpio[LEDPANEL_R0]);
                        }
                }
                if (rgb_buffer[pbuffer_top]>0) {
                        rgb_buffer_copy[pbuffer_top]--;
                        rgb_buffer_copy[pbuffer_top]&=COLOR_MASK;
                }
                pbuffer_top++;

                //GREEN 0
                if (rgb_buffer_copy[pbuffer_top]==0) {
                        fastClearGpio( ledpanel_gpio[LEDPANEL_G0]);
                } else {
                        if (rgb_buffer_copy[pbuffer_top]<=rgb_buffer[pbuffer_top]) {
                                fastSetGpio( ledpanel_gpio[LEDPANEL_G0]);
                        }
                }
                if (rgb_buffer[pbuffer_top]>0) {
                        rgb_buffer_copy[pbuffer_top]--;
                        rgb_buffer_copy[pbuffer_top]&=COLOR_MASK;
                }
                pbuffer_top++;

                //BLUE 0
                if (rgb_buffer_copy[pbuffer_top]==0) {
                        fastClearGpio( ledpanel_gpio[LEDPANEL_B0]);
                } else {
                        if (rgb_buffer_copy[pbuffer_top]<=rgb_buffer[pbuffer_top]) {
                                fastSetGpio( ledpanel_gpio[LEDPANEL_B0]);
                        }
                }
                if (rgb_buffer[pbuffer_top]>0) {
                        rgb_buffer_copy[pbuffer_top]--;
                        rgb_buffer_copy[pbuffer_top]&=COLOR_MASK;
                }
                pbuffer_top++;

                //RED 1
                if (rgb_buffer_copy[pbuffer_bottom]==0) {
                        fastClearGpio( ledpanel_gpio[LEDPANEL_R1]);
                } else {
                        if (rgb_buffer_copy[pbuffer_bottom]<=rgb_buffer[pbuffer_bottom]) {
                                fastSetGpio( ledpanel_gpio[LEDPANEL_R1]);
                        }
                }
                if (rgb_buffer[pbuffer_bottom]>0) {
                        rgb_buffer_copy[pbuffer_bottom]--;
                        rgb_buffer_copy[pbuffer_bottom]&=COLOR_MASK;
                }
                pbuffer_bottom++;

                //GREEN 1
                if (rgb_buffer_copy[pbuffer_bottom]==0) {
                        fastClearGpio( ledpanel_gpio[LEDPANEL_G1]);
                } else {
                        if (rgb_buffer_copy[pbuffer_bottom]<=rgb_buffer[pbuffer_bottom]) {
                                fastSetGpio( ledpanel_gpio[LEDPANEL_G1]);
                        }
                }
                if (rgb_buffer[pbuffer_bottom]>0) {
                        rgb_buffer_copy[pbuffer_bottom]--;
                        rgb_buffer_copy[pbuffer_bottom]&=COLOR_MASK;
                }
                pbuffer_bottom++;

                //BLUE 1
                if (rgb_buffer_copy[pbuffer_bottom]==0) {
                        fastClearGpio( ledpanel_gpio[LEDPANEL_B1]);
                } else {
                        if (rgb_buffer_copy[pbuffer_bottom]<=rgb_buffer[pbuffer_bottom]) {
                                fastSetGpio( ledpanel_gpio[LEDPANEL_B1]);
                        }
                }
                if (rgb_buffer[pbuffer_bottom]>0) {
                        rgb_buffer_copy[pbuffer_bottom]--;
                        rgb_buffer_copy[pbuffer_bottom]&=COLOR_MASK;
                }
                pbuffer_bottom++;

                fastSetGpio(ledpanel_gpio[LEDPANEL_CLK]);
                fastClearGpio(ledpanel_gpio[LEDPANEL_CLK]);
        }

        // Disable the OE
        fastSetGpio(ledpanel_gpio[LEDPANEL_OE]);

        // Change A,B,C,D
        //*((unsigned long *)PB_CODR) = (0xF << A_PPOS);
        //*((unsigned long *)PB_SODR) = (ledpanel_row << A_PPOS);
        writePortB( (ledpanel_row << A_PPOS));

        fastSetGpio(ledpanel_gpio[LEDPANEL_STB]);
        fastClearGpio(ledpanel_gpio[LEDPANEL_STB]);
        fastClearGpio(ledpanel_gpio[LEDPANEL_OE]);

        ledpanel_row++;
        if (ledpanel_row>=16) {
                ledpanel_row=0;
                pbuffer_top=TOP_BYTE_ARRAY;
                pbuffer_bottom=BOTTOM_BYTE_ARRAY;
        }

        fastClearGpio(ledpanel_gpio[LEDPANEL_TP]);

        return 0;
}

