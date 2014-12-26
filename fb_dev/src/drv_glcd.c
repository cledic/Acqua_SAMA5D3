/* */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
/* */
#include "drv_glcd.h"
/* */
#include "Terminal24x12.h"
#include "Terminal12x6.h"

/* Variables to store screen info */
char *fbp = 0;
struct fb_var_screeninfo vinfo, orig_vinfo;;
struct fb_fix_screeninfo finfo;
int fbfd = 0;
long int screensize = 0;
static int FBLCD_InitDone=0;

/* */
static LdcPixel_t TextColour;
static LdcPixel_t TextBackgndColour;
static LdcPixel_t GrafBackgndColour;        // colore di fondo.

static unsigned int FontIdx;

FONTINFO FontInfo[2];

void FBLCD_SetPixel(int x, int y, LdcPixel_t color)
{
        int r, g, b;

        r=(color>>16) & 0xFF;
        g=(color>>8) & 0xFF;
        b=color & 0xFF;

    // calculate the pixel's byte offset inside the buffer
    // note: x * 2 as every pixel is 2 consecutive bytes
    unsigned int pix_offset = x * 2 + y * finfo.line_length;

    // now this is about the same as 'fbp[pix_offset] = value'
    // but a bit more complicated for RGB565
    //unsigned short c = ((r / 8) << 11) + ((g / 4) << 5) + (b / 8);
    unsigned short c = ((r / 8) * 2048) + ((g / 4) * 32) + (b / 8);
    // write 'two bytes at once'
    *((unsigned short*)(fbp + pix_offset)) = c;

}

/*************************************************************************
 * Function Name: FBLCD_Init
 *
 *************************************************************************/
unsigned int FBLCD_Init ( char *fb)
{
    // Open the file for reading and writing
    fbfd = open( fb, O_RDWR);
    if (!fbfd) {
      printf("Error: cannot open framebuffer device.\n");
      return(1);
    }
    //printf("The framebuffer device was opened successfully.\n");

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
      printf("Error reading variable information.\n");
          return(1);
    }
    //printf("Original %dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel );

    // Store for reset (copy vinfo to vinfo_orig)
    memcpy(&orig_vinfo, &vinfo, sizeof(struct fb_var_screeninfo));

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
      printf("Error reading fixed information.\n");
      return(1);
    }

    // map fb to user mem
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    fbp = (char*)mmap(0,
              screensize,
              PROT_READ | PROT_WRITE,
              MAP_SHARED,
              fbfd,
              0);

    if ((int)fbp == -1) {
        printf("Failed to mmap.\n");
                return(1);
    }
        /* All OK */
        FBLCD_InitDone=1;

        /* Da mettere in una funzione: Font_Init */
        FontInfo[0].pText=(unsigned char*)&Text12x6[0];
        FontInfo[0].h_size=6;
        FontInfo[0].v_size=12;
        FontInfo[0].h_line=1;

        FontInfo[1].pText=(unsigned char*)&Text24x12[0];
        FontInfo[1].h_size=12;
        FontInfo[1].v_size=24;
        FontInfo[1].h_line=2;

        FontIdx=0;

        return(0);
}

unsigned int FBLCD_deInit( void)
{
    munmap(fbp, screensize);
    if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo)) {
        printf("Error re-setting variable information.\n");
                return(1);
    }
    close(fbfd);
        return (0);
}

/*************************************************************************
 * Function Name: FBLCD_SetFont
 * Parameters: pFontType_t pFont, LdcPixel_t Color
 *              LdcPixel_t BackgndColor
 *
 * Return: none
 *
 * Description: Set current font, font color and font background color
 *
 *************************************************************************/
void FBLCD_SetFont(unsigned int Font, LdcPixel_t Color, LdcPixel_t BackgndColor)
{
  FontIdx = Font;
  TextColour = Color;
  TextBackgndColour = BackgndColor;
}

/*************************************************************************
 * Function Name: FBLCD_SetGrafBackgndColor
 * Parameters: LdcPixel_t BackgndColor
 *
 * Return: none
 *
 * Description: Set current background color
 *
 *************************************************************************/
void FBLCD_SetGrafBackgndColor( LdcPixel_t BackgndColor)
{
  GrafBackgndColour = BackgndColor;
}


/*************************************************************************
 * Function Name: FBLCD_DrawGrafBackgndColor
 * Parameters: LdcPixel_t BackgndColor
 *
 * Return: none
 *
 * Description: draw the screen with the backgndcolor
 *
 *************************************************************************/
void FBLCD_DrawGrafBackgndColor( LdcPixel_t BackgndColor)
{
        int x, y;

    for (y = 0; y < (vinfo.yres); y++) {
        for (x = 0; x < vinfo.xres; x++) {
                        FBLCD_SetPixel( x, y, BackgndColor);
                }
        }

  //
  FBLCD_SetGrafBackgndColor( BackgndColor);
}

void FBLCD_DrawStr(char *__putstr, unsigned int xpos, unsigned int ypos, unsigned int color)
{
    char __ps;
    unsigned int xp,yp;

    xp=xpos; yp=ypos;

    while((__ps = *__putstr))
    {
        __putstr++;
        if (__ps== 0) break;
        FBLCD_DrawChar((unsigned char)__ps,xp,yp, color);
        xp+=FontInfo[FontIdx].h_size;           // la dimensione del font per adesso ▒ impostata manualmente.
    }
}

void FBLCD_DrawChar( unsigned char ch, unsigned int xpos, unsigned int ypos, unsigned int color)
{
    unsigned int H_Line, H_Size, V_Size, SrcInc;
    unsigned char *pSrc;
    unsigned int i, j, txtcolor, x0, y0;

    // per adesso imposto i valori di queste grandezze manualmente
    H_Line=FontInfo[FontIdx].h_line;
    H_Size=FontInfo[FontIdx].h_size;
    V_Size=FontInfo[FontIdx].v_size;

    //
    pSrc=(unsigned char*)&FontInfo[FontIdx].pText[0]+(H_Line*V_Size*ch);

    //
        x0=xpos;
        y0=ypos;
    for ( i=0; i<V_Size; i++) {
        SrcInc=H_Line;
                x0=xpos;
        for ( j=0; j<H_Size; j++) {
            txtcolor=(*pSrc & (1UL<<(j&0x07)))?color:TextBackgndColour;
            FBLCD_SetPixel(x0++, y0, txtcolor);
            if ((j&0x07) == 7) {
                ++pSrc;
                --SrcInc;
            }
        }
        pSrc += SrcInc;
                y0++;
    }
}

void FBLCD_SetFontSmall(  void)
{
    FontIdx=0;
}

void FBLCD_SetFontBig(  void)
{
    FontIdx=1;
}

/*************************************************************************
 * Function Name: FBLCD_SetPolyLine
 * Parameters: unsigned int npoint, unsigned int poly[], LdcPixel_t color
 *
 * Return: none
 *
 * Description: Draw a polyline.
 *
 *************************************************************************/
void FBLCD_SetPolyLine(unsigned int npoint, int poly[], LdcPixel_t color) {
  unsigned int i, ii;

  i=0;
  ii=npoint*2;
  while (i < ii) {
    FBLCD_SetLine( poly[i], poly[i+1], poly[i+2], poly[i+3], color);
    i+=2;
  }
}

/*************************************************************************
 * Function Name: FBLCD_SetLine
 * Parameters: unsigned int x0(X_Left), unsigned int y0(Y_Up),
 *             unsigned int x0(X_Right), unsigned int y1(Y_Dwn), LdcPixel_t color
 *
 * Return: none
 *
 * Description: Draw a line
 *
 *************************************************************************/
void FBLCD_SetLine(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, LdcPixel_t color) {
  //
  FBLCD_SetPixel(x0, y0, color);
  int dy = y1 - y0;
  int dx = x1 - x0;
  int stepx, stepy;

  if (dy < 0) { dy = -dy; stepy = -1; } else { stepy = 1; }
  if (dx < 0) { dx = -dx; stepx = -1; } else { stepx = 1; }

  dy <<= 1; // dy is now 2*dy
  dx <<= 1; // dx is now 2*dx

  if (dx > dy) {
          int fraction = dy - (dx >> 1);          // same as 2*dy - dx
          while (x0 != x1) {
                  if (fraction >= 0) {
                          y0 += stepy;
                          fraction -= dx;           // same as fraction -= 2*dx
                  }
                  x0 += stepx;
                  fraction += dy;                   // same as fraction -= 2*dy
                  FBLCD_SetPixel(x0, y0, color);
          }
  } else {
          int fraction = dx - (dy >> 1);
          while (y0 != y1) {
                  if (fraction >= 0) {
                          x0 += stepx;
                          fraction -= dy;
                  }
                  y0 += stepy;
                  fraction += dx;
                                  FBLCD_SetPixel(x0, y0, color);
          }
  }
}

/*************************************************************************
 * Function Name: FBLCD_SetCircle
 * Parameters: unsigned int x0(X_Left), unsigned int y0(Y_Up), int radius, LdcPixel_t color
 *
 * Return: none
 *
 * Description: Draw a circle
 *
 *************************************************************************/
void FBLCD_SetCircle(unsigned int x0, unsigned int y0, int radius, LdcPixel_t color) {
  //

  int f = 1 - radius;
  int ddF_x = 0;
  int ddF_y = -2 * radius;
  int x = 0;
  int y = radius;
  //
  FBLCD_SetPixel(x0, y0 + radius, color);
  FBLCD_SetPixel(x0, y0 - radius, color);
  FBLCD_SetPixel(x0 + radius, y0, color);
  FBLCD_SetPixel(x0 - radius, y0, color);

  while(x < y) {
        if(f >= 0) {
                y--;
                ddF_y += 2;
                f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x + 1;
        FBLCD_SetPixel(x0 + x, y0 + y, color);
        FBLCD_SetPixel(x0 - x, y0 + y, color);
        FBLCD_SetPixel(x0 + x, y0 - y, color);
        FBLCD_SetPixel(x0 - x, y0 - y, color);
        /* */
        FBLCD_SetPixel(x0 + y, y0 + x, color);
        FBLCD_SetPixel(x0 - y, y0 + x, color);
        FBLCD_SetPixel(x0 + y, y0 - x, color);
        FBLCD_SetPixel(x0 - y, y0 - x, color);
  }
}

/*************************************************************************
 * Function Name: FBLCD_SetRect
 * Parameters: unsigned int x0(X_Left), unsigned int y0(Y_Up),
 *             unsigned int x1(X_Right), unsigned int y1(Y_Dwn), Int32 fill, LdcPixel_t color
 *
 * Return: none
 *
 * Description: Draw a rectangle
 *
 *************************************************************************/
void FBLCD_SetRect(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, unsigned int fill, LdcPixel_t color) {
  //

  // check if the rectangle is to be filled
  if ( fill) {
    while( y0 < y1) {
      FBLCD_SetLine(x0, y0, x1, y0, color);
      y0++;
    }
  } else {
    // best way to draw un unfilled rectangle is to draw four lines
    FBLCD_SetLine(x0, y0, x1, y0, color);
    FBLCD_SetLine(x0, y1, x1, y1, color);
    FBLCD_SetLine(x0, y0, x0, y1, color);
    FBLCD_SetLine(x1, y0, x1, y1, color);
  }
}

/*************************************************************************
 * Function Name: FBLCD_DrawImg
 * Parameters: unsigned int x0 (x left), unsigned int y0 (y up),
 *             unsigned int xsz, unsigned int ysz, unsigned char *img pointer
 *             to the data as RGB byte.
 *
 * Return: none
 *
 * Description: Draw a RGB image
 *
 *************************************************************************/
void FBLCD_DrawImg(unsigned int x0, unsigned int y0, unsigned int xsz, unsigned int ysz, unsigned char *img)
{
        unsigned int x, y;
        unsigned char r, g, b;

    for (y = 0; y < ysz; y++) {
        for (x = 0; x < xsz; x++) {
                        /* */
                        r=*img++;
                        g=*img++;
                        b=*img++;
                        /* */
                        // FBLCD_SetPixel( x0+x, y0+y, ((r<<16)|(g<<8)|b));
                        unsigned int pix_offset = (x0+x)*2 + (y0+y) * finfo.line_length;
                        *((unsigned short*)(fbp + pix_offset)) = ((r / 8) * 2048) + ((g / 4) * 32) + (b / 8);
                }
        }
}

/*************************************************************************
 * Function Name: FBLCD_DrawIcone
 * Parameters: unsigned int x0 (x left), unsigned int y0 (y up),
 *             unsigned int xsz, unsigned int ysz, unsigned int *img as pointer
 *             to the icone RGB 32bit data
 *
 * Return: none
 *
 * Description: Draw a RGB image
 *
 *************************************************************************/
void FBLCD_DrawIcone(unsigned int x0, unsigned int y0, unsigned int xsz, unsigned int ysz, const unsigned int *img)
{
        unsigned int x, y;

    for (y = 0; y < ysz; y++) {
        for (x = 0; x < xsz; x++) {
                        /* */
                        FBLCD_SetPixel( x0+x, y0+y, *img++);
                }
        }
}
