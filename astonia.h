/*
 * Part of Astonia Client (c) Daniel Brockhaus. Please read license.txt.
 */

#define DEVELOPER               // this one will compile the developer version - comment me out for the final release

//#define TICKPRINT
#define SDL_FAST_MALLOC     // will use the C library instead of the error-checking client version

#define MAXSPRITE 250000

#define XRES    800
#define YRES    (__yres)
#define YRES0   600
#define YRES1   540
#define YRES2   500

extern int __yres;

#define FDX             40      // width of a map tile
#define FDY             20      // height of a map tile

extern int quit;

#define PARANOIA(a) a

int  note(const char *format,...) __attribute__((format(printf, 1, 2)));
int  warn(const char *format,...) __attribute__((format(printf, 1, 2)));
int  fail(const char *format,...) __attribute__((format(printf, 1, 2)));
void paranoia(const char *format,...) __attribute__((format(printf, 1, 2)));

void* xmalloc(int size,int ID);
void* xcalloc(int size,int ID);
void* xrealloc(void *ptr,int size,int ID);
void* xrecalloc(void *ptr,int size,int ID);
void xfree(void *ptr);
char* xstrdup(const char *src,int ID);

#define bzero(ptr,size) memset(ptr,0,size)

#define MEM_NONE        0
#define MEM_GLOB        1
#define MEM_TEMP        2
#define MEM_ELSE        3
#define MEM_DL          4
#define MEM_IC          5
#define MEM_SC          6
#define MEM_VC          7
#define MEM_PC          8
#define MEM_GUI         9
#define MEM_GAME        10
#define MEM_TEMP11      11
#define MEM_VPC         12
#define MEM_VSC         13
#define MEM_VLC         14
#define MEM_SDL_BASE    15
#define MEM_SDL_PIXEL   16
#define MEM_SDL_PNG     17
#define MEM_SDL_PIXEL2  18
#define MEM_TEMP5       19
#define MEM_TEMP6       20
#define MEM_TEMP7       21
#define MEM_TEMP8       22
#define MEM_TEMP9       23
#define MEM_TEMP10      24
#define MAX_MEM         25

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef abs
#define abs(a)	((a)<0 ? (-(a)) : (a))
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof((a)[0]))
#endif

#define DOT_TL          0       // top left?
#define DOT_BR          1       // bottom right?
#define DOT_WEA         2       // worn equipment
#define DOT_INV         3       // inventory
#define DOT_CON         4       // container
#define DOT_SCL         5       // scroll bar left, uses only X
#define DOT_SCR         6       // scroll bar right, uses only X
#define DOT_SCU         7       // scroll bars up arrows at this Y
#define DOT_SCD         8       // scroll bars down arrors at thy Y
#define DOT_TXT         9       // chat window
#define DOT_MTL         10      // map top left
#define DOT_MBR         11      // map bottom right
#define DOT_SKL         12      // skill list
#define DOT_GLD         13      // gold
#define DOT_JNK         14      // trashcan
#define DOT_MOD         15      // speed mode
#define DOT_MCT         16      // map center
#define DOT_TOP         17      // top left corner of equipment bar
#define DOT_BOT         18      // top left corner of bottom window holding skills, chat, etc.
#define DOT_TX2         19      // chat window bottom right
#define DOT_SK2         20      // skill list window bottom right
#define DOT_IN1         21      // inventory top left
#define DOT_IN2         22      // inventory bottom right
#define DOT_HLP         23      // help top left
#define DOT_HL2         24      // help bottom right
#define DOT_TEL         25      // teleporter top left
#define DOT_COL         26      // color picker top left
#define MAX_DOT         27

void init_dots(void);
int dotx(int didx);
int doty(int didx);
int butx(int bidx);
int buty(int bidx);

