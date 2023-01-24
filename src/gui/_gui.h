/*
 * Part of Astonia Client (c) Daniel Brockhaus. Please read license.txt.
 */

#define INVDX                   4
#define INVDY                   4
#define CONDX                   4
#define CONDY                   4
#define SKLDY                   16
#define SKLWIDTH                145
#define LINEHEIGHT              10

#define FX_ITEMLIGHT            DDFX_NLIGHT
#define FX_ITEMBRIGHT           DDFX_BRIGHT
#define DOTF_TOPOFF     (1<<0)  // dot moves with top bar

#define BUTID_MAP       0
#define BUTID_WEA       1
#define BUTID_INV       2
#define BUTID_CON       3
#define BUTID_SCL       4
#define BUTID_SCR       5
#define BUTID_SKL       6
#define BUTID_GLD       7
#define BUTID_JNK       8
#define BUTID_MOD       9
#define BUTID_TELE	10

#define BUT_MAP         0
#define BUT_WEA_BEG     1
#define BUT_WEA_END     12
#define BUT_INV_BEG     13
#define BUT_INV_END     28
#define BUT_CON_BEG     29
#define BUT_CON_END     44
#define BUT_SCL_UP      45
#define BUT_SCL_TR      46
#define BUT_SCL_DW      47
#define BUT_SCR_UP      48
#define BUT_SCR_TR      49
#define BUT_SCR_DW      50
#define BUT_SKL_BEG     51
#define BUT_SKL_END     66
#define BUT_GLD         67
#define BUT_JNK         68
#define BUT_MOD_WALK0   69
#define BUT_MOD_WALK1   70
#define BUT_MOD_WALK2   71

#define BUT_TEL		    72
#define BUT_HELP_NEXT	73
#define BUT_HELP_PREV	74
#define BUT_HELP_MISC	75
#define BUT_HELP_CLOSE	76
#define BUT_EXIT	    77
#define BUT_HELP	    78
#define BUT_NOLOOK	    79
#define BUT_COLOR	    80
#define BUT_SKL_LOOK	81
#define BUT_QUEST	    82
#define BUT_HELP_DRAG   83
#define MAX_BUT         84

#define BUTF_NOHIT      (1<<1)  // button is ignored int hit processing
#define BUTF_CAPTURE    (1<<2)  // button captures mouse on lclick
#define BUTF_MOVEEXEC   (1<<3)  // button calls cmd_exec(lcmd) on mousemove
#define BUTF_RECT       (1<<4)  // editor - button is a rectangle
#define BUTF_TOPOFF     (1<<5)  // button moves with top bar

#define CMD_RETURN	256
#define CMD_DELETE	257
#define CMD_BACK	258
#define CMD_LEFT	259
#define CMD_RIGHT	260
#define CMD_HOME	261
#define CMD_END		262
#define CMD_UP		263
#define CMD_DOWN	264


#define CMD_NONE                0
#define CMD_MAP_MOVE            1
#define CMD_MAP_DROP            2

#define CMD_ITM_TAKE            3
#define CMD_ITM_USE             4
#define CMD_ITM_USE_WITH        5

#define CMD_CHR_ATTACK          6
#define CMD_CHR_GIVE            7

#define CMD_INV_USE             8
#define CMD_INV_USE_WITH        9
#define CMD_INV_TAKE            10
#define CMD_INV_SWAP            11
#define CMD_INV_DROP            12

#define CMD_WEA_USE             13
#define CMD_WEA_USE_WITH        14
#define CMD_WEA_TAKE            15
#define CMD_WEA_SWAP            16
#define CMD_WEA_DROP            17

#define CMD_CON_TAKE            18
#define CMD_CON_BUY             19
#define CMD_CON_SWAP            20
#define CMD_CON_DROP            21
#define CMD_CON_SELL            22

#define CMD_MAP_LOOK            23
#define CMD_ITM_LOOK            24
#define CMD_CHR_LOOK            25
#define CMD_INV_LOOK            26
#define CMD_WEA_LOOK            27
#define CMD_CON_LOOK            28

#define CMD_MAP_CAST_L          29
#define CMD_ITM_CAST_L          30
#define CMD_CHR_CAST_L          31
#define CMD_MAP_CAST_R          32
#define CMD_ITM_CAST_R          33
#define CMD_CHR_CAST_R          34
#define CMD_MAP_CAST_K        	35
#define CMD_CHR_CAST_K        	36
#define CMD_SLF_CAST_K        	37

#define CMD_SPL_SET_L           38
#define CMD_SPL_SET_R           39

#define CMD_SKL_RAISE           40

#define CMD_INV_OFF_UP          41
#define CMD_INV_OFF_DW          42
#define CMD_INV_OFF_TR          43

#define CMD_SKL_OFF_UP          44
#define CMD_SKL_OFF_DW          45
#define CMD_SKL_OFF_TR          46

#define CMD_CON_OFF_UP          47
#define CMD_CON_OFF_DW          48
#define CMD_CON_OFF_TR          49

#define CMD_USE_FKEYITEM        50

#define CMD_SAY_HITSEL          51

#define CMD_DROP_GOLD           52
#define CMD_TAKE_GOLD           53

#define CMD_JUNK_ITEM           54

#define CMD_SPEED0              55
#define CMD_SPEED1              56
#define CMD_SPEED2              57

#define CMD_CON_FASTTAKE	    61
#define CMD_CON_FASTBUY		    62
#define CMD_CON_FASTSELL	    63
#define CMD_TELEPORT		    64
#define CMD_CON_FASTDROP	    65

#define CMD_HELP_NEXT           66
#define CMD_HELP_PREV           67
#define CMD_HELP_MISC           68
#define CMD_HELP_CLOSE		    69
#define CMD_EXIT		        70
#define CMD_HELP		        71
#define CMD_NOLOOK		        72

#define CMD_COLOR		        73
#define CMD_SKL_LOOK		    74
#define CMD_QUEST		        75
#define CMD_HELP_DRAG           76

#define STV_EMPTYLINE           -1
#define STV_JUSTAVALUE          -2      // value is in curr

#define TGT_MAP 1
#define TGT_ITM 2
#define TGT_CHR 3
#define TGT_SLF 4

struct dot {
    int flags;

    int x;
    int y;
};
typedef struct dot DOT;

struct but {
    int flags;      // flags

    int id;         // something an application can give a button, but it need not ;-)
    int val;        // something an application can give a button, but it need not ;-)

    int x;          // center x coordinate - or left if button is a RECT
    int y;          // center y coordinate - or top if button is a RECT
    int dx;         // width of a rect button
    int dy;         // height of a rect button

    int sqhitrad;   // hit (square) radius of this button
};

typedef struct but BUT;

struct skltab {
    int v;                          // negative v-values indicate a special display (empty lines, negative exp, etc...)
    int button;         // show button
    char name[80];
    int base;
    int curr;
    int raisecost;
    int barsize;                    // positive is blue, negative is red
};

typedef struct skltab SKLTAB;

struct keytab {
    int keycode;
    int userdef;
    int vk_item,vk_char,vk_spell;
    char name[40];
    int tgt;
    int cl_spell;
    int skill;
    unsigned int usetime;
};

typedef struct keytab KEYTAB;

struct spell {
    int cl;                         // id of spell sent to server (0=look/spellmode change)
    char name[40];                  // name in text display
};

typedef struct spell SPELL;

extern int winxres,winyres;
extern int gui_topoff;
extern DOT *dot;
extern BUT *but;

extern int plrmn;                      // mn of player
extern int invsel;                     // index into item
extern int weasel;                     // index into weatab
extern int consel;                     // index into item
extern int splsel;
extern int sklsel;
extern int butsel;                     // is always set, if any of the others is set
extern int telsel;
extern int helpsel;
extern int questsel;
extern int colsel;
extern int skl_look_sel;

extern int weatab[12];
extern char weaname[12][32];

extern int cur_cursor;
extern int mousex,mousey,vk_shift,vk_control,vk_alt,vk_rbut,vk_lbut,shift_override,control_override;
extern int mousedx,mousedy;
extern int vk_item,vk_char,vk_spell;

extern int capbut;                     // the button capturing the mouse

extern int invoff,max_invoff;
extern int conoff,max_conoff;
extern int skloff,max_skloff;

extern int fkeyitem[4];

extern int lcmd;
extern int rcmd;

extern int takegold;                   // the amout of gold to take

extern SKLTAB *skltab;
extern int skltab_max;
extern int skltab_cnt;

extern KEYTAB keytab[];
extern int max_keytab;

extern int clan_offset;

extern int show_color,show_cur;
extern int show_color_c[];
extern int show_cx;
extern char hitsel[];

void dx_copysprite_emerald(int scrx,int scry,int emx,int emy);
void display_cmd(void);

void display_wear(void);
void display_look(void);
void display_citem(void);
void display_gold(void);
void display_container(void);
void display_inventory(void);
void display_keys(void);
void display_skill(void);
void display_scrollbars(void);
void display_tutor(void);
void display_screen(void);
void display_text(void);
void display_mode(void);
void display_mouseover(void);
void display_selfspells(void);
void display_exp(void);
void display_military(void);
void display_rage(void);
void display_game_special(void);
int do_display_questlog(int nr);

void display_teleport(void);
int get_teleport(int x,int y);

void display_color(void);
int get_color(int x,int y);
void cmd_color(int nr);

void cmd_proc(int key);
