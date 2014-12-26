/* */

#ifndef __FBLCD_DRV_H
#define __FBLCD_DRV_H

#ifndef TRUE
        #define TRUE 1
#endif

#ifndef FALSE
        #define FALSE 0
#endif

#define cFONT_SMALL     0
#define cFONT_BIG       1

typedef struct _FONTINFO {
    unsigned char *pText;
    unsigned int h_size;
    unsigned int v_size;
    unsigned int h_line;
} FONTINFO;

typedef unsigned int LdcPixel_t, *pLdcPixel_t;

#define C_FBLCD_H_SIZE           800
#define C_FBLCD_V_SIZE           480

#define         BLACK   0x000000
#define         WHITE   0xFFFFFF
#define         RED     0xFF0000
#define         LIME    0x00FF00
#define         BLUE    0x0000FF
#define         YELLOW  0xFFFF00
#define         CYAN    0x00FFFF
#define         MAGENTA 0xFF00FF
#define         SILVER  0xC0C0C0
#define         GRAY    0x808080
#define         MAROON  0x800000
#define         OLIVE   0x808000
#define         GREEN   0x008000
#define         PURPLE  0x800080
#define         TEAL    0x008080
#define         NAVY    0x000080

unsigned int FBLCD_Init ( char *fb);
unsigned int FBLCD_deInit( void);
void FBLCD_SetPixel(int x, int y, LdcPixel_t color);
void FBLCD_SetWindow(unsigned int X_Left, unsigned int Y_Up, unsigned int X_Right, unsigned int Y_Down);
/* ****************** Libreria grafica ****************************** */
void FBLCD_SetGrafBackgndColor( LdcPixel_t BackgndColor);
void FBLCD_DrawGrafBackgndColor( LdcPixel_t BackgndColor);
void FBLCD_SetPolyLine(unsigned int npoint, int poly[], LdcPixel_t color);
void FBLCD_SetLine(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, LdcPixel_t color);
void FBLCD_SetCircle(unsigned int x0, unsigned int y0, int radius, LdcPixel_t color);
void FBLCD_SetRect(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, unsigned int fill, LdcPixel_t color);
/* ****************************************************************** */
void FBLCD_SetFont(unsigned int Font, LdcPixel_t Color, LdcPixel_t BackgndColor);
void FBLCD_SetFontSmall(  void);
void FBLCD_SetFontBig(  void);
void FBLCD_DrawChar( unsigned char ch, unsigned int xpos, unsigned int ypos, unsigned int color);
void FBLCD_DrawStr(char *__putstr, unsigned int xpos, unsigned int ypos, unsigned int color);
void FBLCD_DrawImg(unsigned int x0, unsigned int y0, unsigned int xsz, unsigned int ysz, unsigned char *img);
void FBLCD_DrawIcone(unsigned int x0, unsigned int y0, unsigned int xsz, unsigned int ysz, const unsigned int *img);

#endif // __FBLCD_DRV_H
