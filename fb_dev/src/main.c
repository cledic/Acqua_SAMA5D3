#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "drv_glcd.h"

/* Dimensioni schermo */
#define LCD_WIDTH            (800)
#define LCD_HEIGHT           (480)
/* Macro di conversione RGB */
#define RGB(r,g,b)           (((r&0xF8)<<8)|((g&0xFC)<<3)|((b&0xF8)>>3)) //5 red | 6 green | 5 blue

/* Macro */
#define wait_ms(x)                      usleep(x * 1000)
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Definizione funzioni. */
unsigned int lcd_drawimage( char *filename);
unsigned int lcd_drawRGBimage( const char *filename);
//
static void parse_opts(int argc, char *argv[]);
static void print_usage(const char *prog);

/* Variabili alterabili da cmdline dall'utente */
static const char *fb = "/dev/fb0";             // A default viene usatfb0
static int noreset=0;                                           // A default viene eseguito il reset+init
static int pin_io=82;                                           // A default viene usato il pin 82
static const char *filename;
static const char *MoveFilename;
static const char *FullScreenFilename;
static const char *ScrollingImageFilename;
static int clearcmd=0;                                          // Comando di clear dello schermo.

/* Geometria dell'immagine impostata dall'utente. */
struct RECT {
                int x;
                int y;
                int w;
                int h;
        unsigned char r;
        unsigned char g;
        unsigned char b;
};
/* Funzioni che tengono conto della geometria ipmostata dall'utente. */
unsigned int lcd_drawRGBimageRect( const char *filename, struct RECT rect);
unsigned int lcd_drawmovieRGBRect( char *filename, struct RECT rect);
void lcd_fillareaRect( struct RECT rect);
unsigned int lcd_drawRandomColor( struct RECT rect);
unsigned int lcd_scrollVerticalRGBRect( char *filename, struct RECT rect);

int fd;
struct RECT rect;
int isImage=0;
int isMovie=0;
int isFullScreen=0;
int FrameRate=60;               // frame rate used for movie playing in ms
int LoopFor=-1;
int isRandomFullScreen=0;
int isScrollingImage=0;

/**
 */
static void print_usage(const char *prog)
{
        fprintf( stdout, "Usage: %s [-Dnpf]\n", prog);
        fprintf( stdout, "  -D --device <val>           device to use (default %s)\n", fb);
        fprintf( stdout, "  -f --file <name>            file RGB to draw\n");
        fprintf( stdout, "  -s --screen <name>          full screen file RGB to draw\n");
        fprintf( stdout, "  -m --movie <name>           movie file RGB to play\n");
        fprintf( stdout, "  -n --noreset                no reset is performed\n");
        fprintf( stdout, "  -p --pinreset <val>         pin to use as reset\n");
        fprintf( stdout, "  -c --clear                  redraw the screeen with color if supplied\n");
        fprintf( stdout, "                              or WHITE. Execute first if with -f opt\n");
        fprintf( stdout, "  -l --loop <val>             loop n times, used with -m option\n");
        fprintf( stdout, "  -t --rate <val>             frame rate in ms for -m option\n");
        fprintf( stdout, "  -x --xpos <val>             x position to start to draw\n");
        fprintf( stdout, "  -y --ypos <val>             y position to start to draw\n");
        fprintf( stdout, "  -w --width <val>            width to draw\n");
        fprintf( stdout, "  -h --height <val>           height to draw\n");
        fprintf( stdout, "  -r --red <val>              red color\n");
        fprintf( stdout, "  -g --green <val>            green color\n");
        fprintf( stdout, "  -b --blue <val>             blue color\n");
        fprintf( stdout, "  -R --rand                   Generate a full screen random color image\n");
        fprintf( stdout, "                              use -l option to loop\n");
        fprintf( stdout, "  -S --scroll <name>          Esegue lo scroll verticale dell'immagine\n");
        //
        exit(1);
}

/**
 */
int main(int argc, char *argv[])
{
        // Imposto i valori iniziali della dimensione dell'LCD...
        rect.x=0;
        rect.y=0;
        rect.w=LCD_WIDTH;
        rect.h=LCD_HEIGHT;
        rect.r=rect.g=rect.b=0;

        parse_opts(argc, argv);

        if ( FBLCD_Init( &fb[0])) {
                //printf("FBLCD Init Error!\n");
                return(1);
        } else {
                //printf("FBLCD Init Success!\n");
        }

        //
        if ( clearcmd) lcd_fillareaRect( rect);
        //
        if ( (filename != (const char*)NULL) && isImage) lcd_drawRGBimageRect( (const char*)filename, rect);
        if ( (FullScreenFilename != (const char*)NULL) && isFullScreen) lcd_drawRGBimage( (const char*)FullScreenFilename);
        if ( (MoveFilename != (const char*)NULL) && isMovie) {
                if ( LoopFor == -1) {
                        if (lcd_drawmovieRGBRect( (const char*)MoveFilename, rect)) {
                                return 1;
                        }
                } else {
                        while( LoopFor--) {
                                if ( lcd_drawmovieRGBRect( (const char*)MoveFilename, rect)) {
                                        return 1;
                                }
                        }
                }
        }

        if ( isRandomFullScreen ) {
                if ( LoopFor == -1) {
                        lcd_drawRandomColor( rect);
                } else {
                        while( LoopFor--) {
                                lcd_drawRandomColor( rect);
                        }
                }
        }

        if ( (ScrollingImageFilename != (const char*)NULL) && isScrollingImage) {
                if ( lcd_scrollVerticalRGBRect( (const char*)ScrollingImageFilename, rect))
                        return 1;
        }

        close( fd);
        return 0;
}

/**
 *
 */
unsigned int lcd_scrollVerticalRGBRect( char *filename, struct RECT rect)
{

        int i, fidx;
        FILE *f;

        unsigned char *buff=(unsigned char*)malloc( (rect.w*rect.h*3));

        if ( buff==(unsigned char *)NULL)
                return 1;

        f=fopen( filename, "rb");
        if (f==(FILE*)NULL) {
      		free(buff);
      		return 1;
      	}

        fidx=0;
        while( !feof( f)) {
                // metto a zero l'intero array
                memset( buff, 0x00, (rect.w*rect.h*3));

                i=fread( &buff[0], 1, (rect.w*rect.h*3), f);
                if ( i!= (rect.w*rect.h*3))
                        break;

                FBLCD_DrawImg( rect.x, rect.y, rect.w, rect.h, &buff[0]);

                fseek ( f, rect.w*3*fidx, SEEK_SET);
                fidx++;

                usleep( FrameRate*1000);
        }
        free(buff);
        fclose( f);

        return 0;
}


/** Legge da /dev/urandom i byte richiesti e li visualizza.
 *
 * @param       RECT rect
 * @return      0 -> OK, 1 -> errore.
 */
unsigned int lcd_drawRandomColor( struct RECT rect)
{
        FILE *f;

        //
        unsigned char *buff=(unsigned char*)malloc( (rect.w*rect.h*3));

        if ( buff==NULL)
                return 1;

        f=fopen( "/dev/urandom", "rb");
        if (f==NULL) {
      		free(buff);
      		return 1;
      	}

        fread( &buff[0], 1, (rect.w*rect.h*3), f);

        fclose( f);

        FBLCD_DrawImg( rect.x, rect.y, rect.w, rect.h, &buff[0]);

        free( buff);

        usleep( FrameRate*1000);

        return 0;
}


void lcd_fillareaRect( struct RECT rect)
{
        int i;

        unsigned char *buff=(unsigned char*)malloc( (rect.w*rect.h*3));
        if ( buff==NULL)
                return 1;

        i=0;
        while( i<(rect.w*rect.h*3)) {
                buff[i++] = rect.r;
                buff[i++] = rect.g;
                buff[i++] = rect.b;
        }

        FBLCD_DrawImg( rect.x, rect.y, rect.w, rect.h, &buff[0]);
}

/** Legge il file specificato e lo visualizza sullo schermo.
 * L'immagine deve essere in formato RGB. Tiene conto della dimensione
 * usando la struttura RECT.
 *
 * @param       char *filename
 *                      il nome del file da visualizzare.
 * @param       RECT rect
 * @return      0 -> OK, 1 -> errore.
 */
unsigned int lcd_drawRGBimageRect( const char *filename, struct RECT rect)
{
        FILE *f;

        //
        unsigned char *buff=(unsigned char*)malloc( (rect.w*rect.h*3));

        if ( buff==NULL)
                return 1;

        // metto a zero l'intero array
        memset( buff, 0x00, (rect.w*rect.h*3)+1);

        f=fopen( filename, "rb");
        if (f==NULL) {
                free(buff);
                return 1;
        }

        fread( &buff[0], 1, (rect.w*rect.h*3), f);

        fclose( f);

        FBLCD_DrawImg( rect.x, rect.y, rect.w, rect.h, &buff[0]);

        free( buff);

        return 0;
}

unsigned int lcd_drawRGBimage( const char *filename)
{
        FILE *f;

        //
        unsigned char *buff=(unsigned char*)malloc( (800*480*3));

        if ( buff==NULL)
                return 1;

        // metto a zero l'intero array
        memset( buff, 0x00, (800*480*3)+1);

        f=fopen( filename, "rb");
        if (f==NULL){
                free(buff);
                return 1;
        }

        fread( &buff[0], 1, (800*480*3), f);

        fclose( f);

        FBLCD_DrawImg( 0, 0, 800, 480, &buff[0]);

        free( buff);

        return 0;
}


/** Visualizza il contenuto di un file precedentemente realizzato come sequenza di immagini in movimento
 *  in formato RGB 565, di dimensione 320x240
 */
unsigned int lcd_drawmovieRGBRect( char *filename, struct RECT rect)
{

        int i;
        FILE *f;

        unsigned char *buff=(unsigned char*)malloc( (rect.w*rect.h*3));

        if ( buff==(unsigned char *)NULL)
                return 1;

        f=fopen( filename, "rb");
        if (f==(FILE*)NULL){
                free(buff);
                return 1;
        }

        while( !feof( f)) {
                // metto a zero l'intero array
                memset( buff, 0x00, (rect.w*rect.h*3));

                i=fread( &buff[0], 1, (rect.w*rect.h*3), f);
                if ( i!= (rect.w*rect.h*3))
                        break;

                FBLCD_DrawImg( rect.x, rect.y, rect.w, rect.h, &buff[0]);

                usleep( FrameRate*1000);
        }
        //
        fclose( f);
        free(buff);

        return 0;
}


static void parse_opts(int argc, char *argv[])
{

        static const struct option lopts[] = {
                { "device",     0, 0, 'D' },
                { "noreset",    0, 0, 'n' },
                { "rand",       0, 0, 'R' },
                { "scroll",     1, 0, 'S' },
                { "pinreset",   1, 0, 'p' },
                { "file",       1, 0, 'f' },
                { "screen",     1, 0, 's' },
                { "movie",      1, 0, 'm' },
                { "loop",       1, 0, 'l' },
                { "rate",       1, 0, 't' },
                { "xpos",       1, 0, 'x' },
                { "ypos",       1, 0, 'y' },
                { "width",      1, 0, 'w' },
                { "height",     1, 0, 'h' },
                { "clear",      0, 0, 'c' },
                { "red",        1, 0, 'r' },
                { "blue",       1, 0, 'b' },
                { "green",      1, 0, 'g' },
                { NULL, 0, 0, 0 },
        };
        int c;

//      fprintf( stdout, "parse_opts\n");

        while (1) {
                c = getopt_long(argc, argv, "f:m:s:p:DncRx:l:t:y:w:h:r:g:b:S:", lopts, NULL);

                if (c == -1)
                        break;

//              fprintf( stdout, "opts: %c\n", c);

                switch (c) {
                case 'D':
                        fb = optarg;
                        break;
                case 'n':
                        noreset=1;
                        break;
                case 'R':
                        isRandomFullScreen=1;
                        break;
                case 'p':
                        pin_io = atoi(optarg);
                        break;
                case 's':
                        isFullScreen=1;
                        FullScreenFilename = optarg;
                        break;
                case 'f':
                        isImage=1;
                        filename = optarg;
                        break;
                case 'S':
                        isScrollingImage=1;
                        ScrollingImageFilename = optarg;
                        break;
                case 'm':
                        isMovie=1;
                        MoveFilename = optarg;
                        break;
                case 'l':
                        LoopFor = atoi(optarg);
                        break;
                case 't':
                        FrameRate = atoi(optarg);
                        break;
                case 'x':
                        rect.x = atoi(optarg);
                        if ( rect.x > LCD_WIDTH)
                                rect.x=LCD_WIDTH;
                        break;
                case 'y':
                        rect.y = atoi(optarg);
                        if ( rect.y > LCD_HEIGHT)
                                rect.y=LCD_HEIGHT;
                        break;
                case 'w':
                        rect.w = atoi(optarg);
                        if ( rect.w > LCD_WIDTH)
                                rect.w=LCD_WIDTH;
                        break;
                case 'h':
                        rect.h = atoi(optarg);
                        if ( rect.y > LCD_HEIGHT)
                                rect.y=LCD_HEIGHT;
                        break;
                case 'r':
                        rect.r = atoi(optarg);
                        break;
                case 'g':
                        rect.g = atoi(optarg);
                        break;
                case 'b':
                        rect.b = atoi(optarg);
                        break;
                case 'c':
                        clearcmd=1;
                        break;
                default:
                        print_usage(argv[0]);
                        break;
                }
        }
}


