/*
 * Part of Astonia Client (c) Daniel Brockhaus. Please read license.txt.
 *
 * Client/Server Communication
 *
 * Parses server commands and sends back player input
 *
 */

#include <time.h>
#include <zlib.h>
#include <SDL_net.h>

#include "../../src/astonia.h"
#include "../../src/client.h"
#include "../../src/client/_client.h"
#include "../../src/sdl.h"
#include "../../src/gui.h"
#include "../../src/modder.h"

int display_gfx=0,display_time=0;
static int rec_bytes=0;
static int sent_bytes=0;
static TCPsocket sock=NULL;
int sockstate=0;
static unsigned int socktime=0;
int socktimeout=0;
static int change_area=0;
int kicked_out=0;
unsigned int unique=0;
#ifdef STORE_UNIQUE
unsigned int usum=0;
#endif
int target_port=5556;
DLL_EXPORT char *target_server=NULL;
DLL_EXPORT char password[16];
static int zsinit;
static struct z_stream_s zs;

DLL_EXPORT char username[40];
DLL_EXPORT int tick;
DLL_EXPORT int mirror=0;
DLL_EXPORT int realtime;
DLL_EXPORT int protocol_version=0;

int newmirror=0;
int lasttick;           // ticks in inbuf
static int lastticksize;       // size inbuf must reach to get the last tick complete in the queue

static struct queue queue[Q_SIZE];
int q_in,q_out,q_size;

static int server_cycles;

static int ticksize;
static int inused;
static int indone;
static int login_done;
static unsigned char inbuf[MAX_INBUF];

static int outused;
static unsigned char outbuf[MAX_OUTBUF];

DLL_EXPORT int act;
DLL_EXPORT int actx;
DLL_EXPORT int acty;

DLL_EXPORT unsigned int cflags;        // current item flags
DLL_EXPORT unsigned int csprite;       // and sprite

DLL_EXPORT int originx;
DLL_EXPORT int originy;
DLL_EXPORT struct map map[MAPDX*MAPDY];
DLL_EXPORT struct map map2[MAPDX*MAPDY];

DLL_EXPORT int value[2][V_MAX];
DLL_EXPORT int item[INVENTORYSIZE];
DLL_EXPORT int item_flags[INVENTORYSIZE];
DLL_EXPORT int hp;
DLL_EXPORT int mana;
DLL_EXPORT int rage;
DLL_EXPORT int endurance;
DLL_EXPORT int lifeshield;
DLL_EXPORT int experience;
DLL_EXPORT int experience_used;
DLL_EXPORT int mil_exp;
DLL_EXPORT int gold;

DLL_EXPORT struct player player[MAXCHARS];

DLL_EXPORT union ceffect ceffect[MAXEF];
DLL_EXPORT unsigned char ueffect[MAXEF];

DLL_EXPORT int con_type;
DLL_EXPORT char con_name[80];
DLL_EXPORT int con_cnt;
DLL_EXPORT int container[CONTAINERSIZE];
DLL_EXPORT int price[CONTAINERSIZE];
DLL_EXPORT int itemprice[CONTAINERSIZE];
DLL_EXPORT int cprice;

DLL_EXPORT int lookinv[12];
DLL_EXPORT int looksprite,lookc1,lookc2,lookc3;
DLL_EXPORT char look_name[80];
DLL_EXPORT char look_desc[1024];

DLL_EXPORT char pent_str[7][80];

DLL_EXPORT int pspeed=0;   // 0=normal   1=fast      2=stealth     - like the server

int may_teleport[64+32];

DLL_EXPORT int frames_per_second=TICKS;

struct vnquest vnq={0};

int sv_map01(unsigned char *buf,int *last,struct map *cmap) {
    int p,c;

    if ((buf[0]&(16+32))==SV_MAPTHIS) {
        p=1;
        c=*last;
    } else if ((buf[0]&(16+32))==SV_MAPNEXT) {
        p=1;
        c=*last+1;

    } else if ((buf[0]&(16+32))==SV_MAPOFF) {
        p=2;
        c=*last+*(unsigned char *)(buf+1);
    } else {
        p=3;
        c=*(unsigned short *)(buf+1);
    }

    if (c>MAPDX*MAPDY || c<0) { fail("sv_map01 illegal call with c=%d\n",c); exit(-1); }

    if (buf[0]&1) {
        cmap[c].ef[0]=*(unsigned int *)(buf+p); p+=4;
    }
    if (buf[0]&2) {
        cmap[c].ef[1]=*(unsigned int *)(buf+p); p+=4;
    }
    if (buf[0]&4) {
        cmap[c].ef[2]=*(unsigned int *)(buf+p); p+=4;
    }
    if (buf[0]&8) {
        cmap[c].ef[3]=*(unsigned int *)(buf+p); p+=4;
    }

    *last=c;

    return p;
}

int sv_map10(unsigned char *buf,int *last,struct map *cmap) {
    int p,c;

    if ((buf[0]&(16+32))==SV_MAPTHIS) {
        p=1;
        c=*last;
    } else if ((buf[0]&(16+32))==SV_MAPNEXT) {
        p=1;
        c=*last+1;
    } else if ((buf[0]&(16+32))==SV_MAPOFF) {
        p=2;
        c=*last+*(unsigned char *)(buf+1);
    } else {
        p=3;
        c=*(unsigned short *)(buf+1);
    }

    if (c>MAPDX*MAPDY || c<0) { fail("sv_map10 illegal call with c=%d\n",c); exit(-1); }

    if (buf[0]&1) {
        cmap[c].csprite=*(unsigned int *)(buf+p); p+=4;
        cmap[c].cn=*(unsigned short *)(buf+p); p+=2;
    }
    if (buf[0]&2) {
        cmap[c].action=*(unsigned char *)(buf+p); p++;
        cmap[c].duration=*(unsigned char *)(buf+p); p++;
        cmap[c].step=*(unsigned char *)(buf+p); p++;
    }
    if (buf[0]&4) {
        cmap[c].dir=*(unsigned char *)(buf+p); p++;
        cmap[c].health=*(unsigned char *)(buf+p); p++;
        cmap[c].mana=*(unsigned char *)(buf+p); p++;
        cmap[c].shield=*(unsigned char *)(buf+p); p++;
    }
    if (buf[0]&8) {
        cmap[c].csprite=0;
        cmap[c].cn=0;
        cmap[c].action=0;
        cmap[c].duration=0;
        cmap[c].step=0;
        cmap[c].dir=0;
        cmap[c].health=0;
    }

    *last=c;

    return p;
}

int sv_map11(unsigned char *buf,int *last,struct map *cmap) {
    int p,c;
    int tmp32;

    if ((buf[0]&(16+32))==SV_MAPTHIS) {
        p=1;
        c=*last;
    } else if ((buf[0]&(16+32))==SV_MAPNEXT) {
        p=1;
        c=*last+1;
    } else if ((buf[0]&(16+32))==SV_MAPOFF) {
        p=2;
        c=*last+*(unsigned char *)(buf+1);
    } else {
        p=3;
        c=*(unsigned short *)(buf+1);
    }

    if (c>MAPDX*MAPDY || c<0) { fail("sv_map11 illegal call with c=%d\n",c); exit(-1); }

    if (buf[0]&1) {
        tmp32=*(unsigned int *)(buf+p); p+=4;
        cmap[c].gsprite=(unsigned short int)(tmp32&0x0000FFFF);
        cmap[c].gsprite2=(unsigned short int)(tmp32>>16);
    }
    if (buf[0]&2) {
        tmp32=*(unsigned int *)(buf+p); p+=4;
        cmap[c].fsprite=(unsigned short int)(tmp32&0x0000FFFF);
        cmap[c].fsprite2=(unsigned short int)(tmp32>>16);
    }
    if (buf[0]&4) {
        cmap[c].isprite=*(unsigned int *)(buf+p); p+=4;
        if (cmap[c].isprite&0x80000000) {
            cmap[c].isprite&=~0x80000000;
            cmap[c].ic1=*(unsigned short *)(buf+p); p+=2;
            cmap[c].ic2=*(unsigned short *)(buf+p); p+=2;
            cmap[c].ic3=*(unsigned short *)(buf+p); p+=2;
        } else {
            cmap[c].ic1=0;
            cmap[c].ic2=0;
            cmap[c].ic3=0;
        }
    }
    if (buf[0]&8) {
        if (*(unsigned char *)(buf+p)) {
            cmap[c].flags=*(unsigned short *)(buf+p); p+=2;
        } else {
            cmap[c].flags=*(unsigned char *)(buf+p); p++;
        }
    }

    *last=c;

    return p;
}

int svl_ping(char *buf) {
    int t,diff;

    t=*(unsigned int *)(buf+1);
    diff=SDL_GetTicks()-t;
    addline("RTT1: %.2fms",diff/1000.0);

    return 5;
}

int sv_ping(char *buf) {
    int t,diff;

    t=*(unsigned int *)(buf+1);
    diff=SDL_GetTicks()-t;
    addline("RTT2: %.2fms",diff/1000.0);

    return 5;
}


void sv_scroll_right(struct map *cmap) {
    memmove(cmap,cmap+1,sizeof(struct map)*((DIST*2+1)*(DIST*2+1)-1));
}

void sv_scroll_left(struct map *cmap) {
    memmove(cmap+1,cmap,sizeof(struct map)*((DIST*2+1)*(DIST*2+1)-1));
}

void sv_scroll_down(struct map *cmap) {
    memmove(cmap,cmap+(DIST*2+1),sizeof(struct map)*((DIST*2+1)*(DIST*2+1)-(DIST*2+1)));
}

void sv_scroll_up(struct map *cmap) {
    memmove(cmap+(DIST*2+1),cmap,sizeof(struct map)*((DIST*2+1)*(DIST*2+1)-(DIST*2+1)));
}

void sv_scroll_leftup(struct map *cmap) {
    memmove(cmap+(DIST*2+1)+1,cmap,sizeof(struct map)*((DIST*2+1)*(DIST*2+1)-(DIST*2+1)-1));
}

void sv_scroll_leftdown(struct map *cmap) {
    memmove(cmap,cmap+(DIST*2+1)-1,sizeof(struct map)*((DIST*2+1)*(DIST*2+1)-(DIST*2+1)+1));
}

void sv_scroll_rightup(struct map *cmap) {
    memmove(cmap+(DIST*2+1)-1,cmap,sizeof(struct map)*((DIST*2+1)*(DIST*2+1)-(DIST*2+1)+1));
}

void sv_scroll_rightdown(struct map *cmap) {
    memmove(cmap,cmap+(DIST*2+1)+1,sizeof(struct map)*((DIST*2+1)*(DIST*2+1)-(DIST*2+1)-1));
}

void sv_setval(unsigned char *buf,int nr) {
    int n;

    n=buf[1];
    if (n<0 || n>=(*game_v_max)) return;

    if (nr!=0 || n!=V_PROFESSION) value[nr][n]=*(short *)(buf+2);

    update_skltab=1;
}

void sv_sethp(unsigned char *buf) {
    hp=*(short *)(buf+1);
}

void sv_endurance(unsigned char *buf) {
    endurance=*(short *)(buf+1);
}

void sv_lifeshield(unsigned char *buf) {
    lifeshield=*(short *)(buf+1);
}

void sv_setmana(unsigned char *buf) {
    mana=*(short *)(buf+1);
}

void sv_setrage(unsigned char *buf) {
    rage=*(short *)(buf+1);
}

void sv_setitem(unsigned char *buf) {
    int n;

    n=buf[1];
    if (n<0 || n>=INVENTORYSIZE) return;

    item[n]=*(unsigned int *)(buf+2);
    item_flags[n]=*(unsigned int *)(buf+6);

    hover_invalidate_inv(n);
}

void sv_setorigin(unsigned char *buf) {
    originx=*(unsigned short int *)(buf+1);
    originy=*(unsigned short int *)(buf+3);
}

void sv_settick(unsigned char *buf) {
    tick=*(unsigned int *)(buf+1);
}

void sv_mirror(unsigned char *buf) {
    mirror=newmirror=*(unsigned int *)(buf+1);
}

void sv_realtime(unsigned char *buf) {
    realtime=*(unsigned int *)(buf+1);
}

void sv_speedmode(unsigned char *buf) {
    pspeed=buf[1];
}

// unused in vanilla server
void sv_fightmode(unsigned char *buf) {
}

void sv_setcitem(unsigned char *buf) {
    csprite=*(unsigned int *)(buf+1);
    cflags=*(unsigned int *)(buf+5);
}

void sv_act(unsigned char *buf) {

    act=*(unsigned short int *)(buf+1);
    actx=*(unsigned short int *)(buf+3);
    acty=*(unsigned short int *)(buf+5);

    if (act) teleporter=0;
}

int sv_text(unsigned char *buf) {
    int len;
    char line[1024];

    len=*(unsigned short *)(buf+1);
    if (len<1000) {
        memcpy(line,buf+3,len);
        line[len]=0;
        if (line[0]=='#') {
            if (!isdigit(line[1])) {
                strcpy(tutor_text,line+1);
                show_tutor=1;
            } else if (line[1]=='1') {
                strcpy(look_name,line+2);
            } else if (line[1]=='2') {
                strcpy(look_desc,line+2);
            } else if (line[1]=='3') {
                strcpy(pent_str[0],line+2);
                pent_str[1][0]=pent_str[2][0]=pent_str[3][0]=pent_str[4][0]=pent_str[5][0]=pent_str[6][0]=0;
            } else if (line[1]=='4') {
                strcpy(pent_str[1],line+2);
            } else if (line[1]=='5') {
                strcpy(pent_str[2],line+2);
            } else if (line[1]=='6') {
                strcpy(pent_str[3],line+2);
            } else if (line[1]=='7') {
                strcpy(pent_str[4],line+2);
            } else if (line[1]=='8') {
                strcpy(pent_str[5],line+2);
            } else if (line[1]=='9') {
                strcpy(pent_str[6],line+2);
            }
        } else {
            if (!hover_capture_text(line))
                addline("%s",line);
        }
    }

    return len+3;
}

int svl_text(unsigned char *buf) {
    int len;

    len=*(unsigned short *)(buf+1);
    return len+3;
}

int sv_conname(unsigned char *buf) {
    int len;

    len=*(unsigned char *)(buf+1);
    if (len<80) {
        memcpy(con_name,buf+2,len);
        con_name[len]=0;
    }

    return len+2;
}

int svl_conname(unsigned char *buf) {
    int len;

    len=*(unsigned char *)(buf+1);

    return len+2;
}

int sv_exit(unsigned char *buf) {
    int len;
    char line[1024];

    len=*(unsigned char *)(buf+1);
    if (len<=200) {
        memcpy(line,buf+2,len);
        line[len]=0;
        addline("Server demands exit: %s",line);
    }
    kicked_out=1;

    return len+2;
}

int svl_exit(unsigned char *buf) {
    int len;

    len=*(unsigned char *)(buf+1);
    return len+2;
}

int sv_name(unsigned char *buf) {
    int len,cn;

    len=buf[12];
    cn=*(unsigned short *)(buf+1);

    if (cn<1 || cn>=MAXCHARS) addline("illegal cn %d in sv_name",cn);
    else {
        memcpy(player[cn].name,buf+13,len);
        player[cn].name[len]=0;

        player[cn].level=*(unsigned char *)(buf+3);
        player[cn].c1=*(unsigned short *)(buf+4);
        player[cn].c2=*(unsigned short *)(buf+6);
        player[cn].c3=*(unsigned short *)(buf+8);
        player[cn].clan=*(unsigned char *)(buf+10);
        player[cn].pk_status=*(unsigned char *)(buf+11);
    }

    return len+13;
}

int svl_name(unsigned char *buf) {
    int len;

    len=buf[12];

    return len+13;
}

int find_ceffect(int fn) {
    int n;

    for (n=0; n<MAXEF; n++) {
        if (ueffect[n] && ceffect[n].generic.nr==fn) {
            return n;
        }
    }
    return -1;
}

int is_char_ceffect(int type) {
    switch (type) {
        case 1:         return 1;
        case 2:         return 0;
        case 3:         return 1;
        case 4:         return 0;
        case 5:         return 1;
        case 6:         return 0;
        case 7:         return 0;
        case 8:         return 1;
        case 9:         return 1;
        case 10:        return 1;
        case 11:        return 1;
        case 12:        return 1;
        case 13:        return 0;
        case 14:        return 1;
        case 15:        return 0;
        case 16:        return 0;
        case 17:        return 0;
        case 22:	return 1;
        case 23:	return 1;

    }
    return 0;
}

int find_cn_ceffect(int cn,int skip) {
    int n;

    for (n=0; n<MAXEF; n++) {
        if (ueffect[n] && is_char_ceffect(ceffect[n].generic.type) && ceffect[n].flash.cn==cn) {
            if (skip) { skip--; continue; }
            return n;
        }
    }
    return -1;
}

int sv_ceffect(unsigned char *buf) {
    int nr,type,len=0; //,fn,arg;

    nr=buf[1];
    type=((struct cef_generic *)(buf+2))->type;

    switch (type) {
        case 1:		len=sizeof(struct cef_shield); break;
        case 2:		len=sizeof(struct cef_ball); break;
        case 3:		len=sizeof(struct cef_strike); break;
        case 4:		len=sizeof(struct cef_fireball); break;
        case 5:		len=sizeof(struct cef_flash); break;

        case 7:		len=sizeof(struct cef_explode); break;
        case 8:		len=sizeof(struct cef_warcry); break;
        case 9:		len=sizeof(struct cef_bless); break;
        case 10:	len=sizeof(struct cef_heal); break;
        case 11:	len=sizeof(struct cef_freeze); break;
        case 12:	len=sizeof(struct cef_burn); break;
        case 13:	len=sizeof(struct cef_mist); break;
        case 14:	len=sizeof(struct cef_potion); break;
        case 15:	len=sizeof(struct cef_earthrain); break;
        case 16:	len=sizeof(struct cef_earthmud); break;
        case 17:	len=sizeof(struct cef_edemonball); break;
        case 18:	len=sizeof(struct cef_curse); break;
        case 19:	len=sizeof(struct cef_cap); break;
        case 20:	len=sizeof(struct cef_lag); break;
        case 21:	len=sizeof(struct cef_pulse); break;
        case 22:	len=sizeof(struct cef_pulseback); break;
        case 23:	len=sizeof(struct cef_firering); break;
        case 24:	len=sizeof(struct cef_bubble); break;


        default:	note("unknown effect %d",type); break;
    }

    if (nr<0 || nr>=MAXEF) { fail("sv_ceffect: invalid nr %d\n",nr); exit(-1); }

    memcpy(ceffect+nr,buf+2,len);

    return len+2;
}

void sv_ueffect(unsigned char *buf) {
    int n,i,b;

    for (n=0; n<MAXEF; n++) {
        i=n/8;
        b=1<<(n&7);
        if (buf[i+1]&b) ueffect[n]=1;
        else ueffect[n]=0;
    }
}

int svl_ceffect(unsigned char *buf) {
    int nr,type,len=0;

    nr=buf[1];
    type=((struct cef_generic *)(buf+2))->type;

    switch (type) {
        case 1:		len=sizeof(struct cef_shield); break;
        case 2:		len=sizeof(struct cef_ball); break;
        case 3:		len=sizeof(struct cef_strike); break;
        case 4:		len=sizeof(struct cef_fireball); break;
        case 5:		len=sizeof(struct cef_flash); break;

        case 7:		len=sizeof(struct cef_explode); break;
        case 8:		len=sizeof(struct cef_warcry); break;
        case 9:		len=sizeof(struct cef_bless); break;
        case 10:	len=sizeof(struct cef_heal); break;
        case 11:	len=sizeof(struct cef_freeze); break;
        case 12:	len=sizeof(struct cef_burn); break;
        case 13:	len=sizeof(struct cef_mist); break;
        case 14:	len=sizeof(struct cef_potion); break;
        case 15:	len=sizeof(struct cef_earthrain); break;
        case 16:	len=sizeof(struct cef_earthmud); break;
        case 17:	len=sizeof(struct cef_edemonball); break;
        case 18:	len=sizeof(struct cef_curse); break;
        case 19:	len=sizeof(struct cef_cap); break;
        case 20:	len=sizeof(struct cef_lag); break;
        case 21:	len=sizeof(struct cef_pulse); break;
        case 22:	len=sizeof(struct cef_pulseback); break;
        case 23:	len=sizeof(struct cef_firering); break;
        case 24:	len=sizeof(struct cef_bubble); break;


        default:	note("unknown effect %d",type); break;

    }

    if (nr<0 || nr>=MAXEF) { fail("svl_ceffect: invalid nr %d\n",nr); exit(-1); }

    return len+2;
}

void sv_container(unsigned char *buf) {
    int nr;

    nr=buf[1];
    if (nr<0 || nr>=CONTAINERSIZE) { fail("illegal nr %d in sv_container!",nr);  exit(-1); }

    container[nr]=*(unsigned int *)(buf+2);
    hover_invalidate_con(nr);
}

void sv_price(unsigned char *buf) {
    int nr;

    nr=buf[1];
    if (nr<0 || nr>=CONTAINERSIZE) { fail("illegal nr %d in sv_price!",nr);  exit(-1); }

    price[nr]=*(unsigned int *)(buf+2);
}

void sv_itemprice(unsigned char *buf) {
    int nr;

    nr=buf[1];
    if (nr<0 || nr>=CONTAINERSIZE) { fail("illegal nr %d in sv_itemprice!",nr);  exit(-1); }

    itemprice[nr]=*(unsigned int *)(buf+2);
}

void sv_cprice(unsigned char *buf) {
    cprice=*(unsigned int *)(buf+1);
}

void sv_gold(unsigned char *buf) {
    gold=*(unsigned int *)(buf+1);
}

void sv_concnt(unsigned char *buf) {
    int nr;

    nr=buf[1];
    if (nr<0 || nr>CONTAINERSIZE) { fail("illegal nr %d in sv_contcnt!",nr);  exit(-1); }

    con_cnt=nr;
}

void sv_contype(unsigned char *buf) {
    con_type=buf[1];
}

void sv_exp(unsigned char *buf) {
    experience=*(unsigned long *)(buf+1);
    update_skltab=1;
}

void sv_exp_used(unsigned char *buf) {
    experience_used=*(unsigned long *)(buf+1);
    update_skltab=1;
}

void sv_mil_exp(unsigned char *buf) {
    mil_exp=*(unsigned long *)(buf+1);
}

void sv_cycles(unsigned char *buf) {
    int c;

    c=*(unsigned long *)(buf+1);

    server_cycles=server_cycles*0.99+c*0.01;
}

void sv_lookinv(unsigned char *buf) {
    int n;

    looksprite=*(unsigned int *)(buf+1);
    lookc1=*(unsigned int *)(buf+5);
    lookc2=*(unsigned int *)(buf+9);
    lookc3=*(unsigned int *)(buf+13);
    for (n=0; n<12; n++) {
        lookinv[n]=*(unsigned int *)(buf+17+n*4);
    }
    show_look=1;
}

void sv_server(unsigned char *buf) {
    change_area=1;
    // TODO: The following line is needed if the area servers are not all on the same IP
    // address. BUT the vanilla server has a wrong IP hardcoded and this breaks the clients
    // for the most common case of a single host running all areas. So. Commented out:

    //target_server=*(unsigned int *)(buf+1);

    target_port=*(unsigned short *)(buf+5);
}

void sv_logindone(void) {
    login_done=1;
    bzero_client(1);
}

void sv_special(unsigned char *buf) {
    unsigned int type,opt1,opt2;

    type=*(unsigned int *)(buf+1);
    opt1=*(unsigned int *)(buf+5);
    opt2=*(unsigned int *)(buf+9);

    switch (type) {
        case 0:		display_gfx=opt1; display_time=tick; break;
        default:	if (type>0 && type<1000) play_sound(type,opt1,opt2);
            break;
    }
}

void sv_teleport(unsigned char *buf) {
    int n,i,b;

    for (n=0; n<64+32; n++) {
        i=n/8;
        b=1<<(n&7);
        if (buf[i+1]&b) may_teleport[n]=1;
        else may_teleport[n]=0;
    }
    teleporter=1;
    newmirror=mirror;
}

void sv_prof(unsigned char *buf) {
    int n,cnt=0;

    for (n=0; n<P_MAX; n++) {
        cnt+=(value[1][n+V_PROFBASE]=buf[n+1]);
    }
    value[0][V_PROFESSION]=cnt;

    update_skltab=1;
}

DLL_EXPORT struct quest quest[MAXQUEST];
DLL_EXPORT struct shrine_ppd shrine;

void sv_questlog(unsigned char *buf) {
    int size;

    size=sizeof(struct quest)*MAXQUEST;

    memcpy(quest,buf+1,size);
    memcpy(&shrine,buf+1+size,sizeof(struct shrine_ppd));
}

void save_unique(void);
void load_unique(void);

void sv_unique(unsigned char *buf) {
    if (unique!=*(unsigned int *)(buf+1)) {
        unique=*(unsigned int *)(buf+1);
        #ifdef STORE_UNIQUE
        save_unique();
        #endif
    }
}

void sv_protocol(unsigned char *buf) {
    protocol_version=buf[1];
    //note("Astonia Protocol Version %d established!",protocol_version);
}

int svl_vnquest(unsigned char *buf) {
    int len;

    len=*(uint16_t*)(buf+1);

    return 3+len+sizeof(struct vn_quest);
}

int vnquest_helper(char **dest,char *ptr,int size) {
    if (*dest) xfree(*dest);

    if (size) {
        *dest=xmalloc(size+1,MEM_TEMP8);
        memcpy(*dest,ptr,size);
        (*dest)[size]=0;
    } else *dest=NULL;

    return size;
}

int sv_vnquest(unsigned char *buf) {
    int len;
    struct vn_quest *vq;
    char *ptr;

    vq=(void*)(buf+3);
    ptr=buf+3+sizeof(struct vn_quest);

    vnq.ID=vq->ID;
    vnq.sprite=vq->sprite;

    ptr+=vnquest_helper(&vnq.title,ptr,vq->title);
    ptr+=vnquest_helper(&vnq.para1,ptr,vq->para1);
    ptr+=vnquest_helper(&vnq.para2,ptr,vq->para2);
    ptr+=vnquest_helper(&vnq.para3,ptr,vq->para3);
    ptr+=vnquest_helper(&vnq.line1,ptr,vq->line1);
    ptr+=vnquest_helper(&vnq.line2,ptr,vq->line2);
    ptr+=vnquest_helper(&vnq.line3,ptr,vq->line3);
    ptr+=vnquest_helper(&vnq.line4,ptr,vq->line4);
    ptr+=vnquest_helper(&vnq.butt1,ptr,vq->butt1);
    ptr+=vnquest_helper(&vnq.butt2,ptr,vq->butt2);

    len=*(uint16_t*)(buf+1);

    return 3+len+sizeof(struct vn_quest);
}

void process(unsigned char *buf,int size) {
    int len=0,panic=0,last=-1;

    while (size>0 && panic++<20000) {
        if ((buf[0]&(64+128))==SV_MAP01) len=sv_map01(buf,&last,map);
        else if ((buf[0]&(64+128))==SV_MAP10) len=sv_map10(buf,&last,map);
        else if ((buf[0]&(64+128))==SV_MAP11) len=sv_map11(buf,&last,map);
        else switch (buf[0]) {
                case SV_SCROLL_UP:              sv_scroll_up(map); len=1; break;
                case SV_SCROLL_DOWN:            sv_scroll_down(map); len=1; break;
                case SV_SCROLL_LEFT:            sv_scroll_left(map); len=1; break;
                case SV_SCROLL_RIGHT:           sv_scroll_right(map); len=1; break;
                case SV_SCROLL_LEFTUP:          sv_scroll_leftup(map); len=1; break;
                case SV_SCROLL_LEFTDOWN:        sv_scroll_leftdown(map); len=1; break;
                case SV_SCROLL_RIGHTUP:         sv_scroll_rightup(map); len=1; break;
                case SV_SCROLL_RIGHTDOWN:       sv_scroll_rightdown(map); len=1; break;

                case SV_SETVAL0:                sv_setval(buf,0); len=4; break;
                case SV_SETVAL1:                sv_setval(buf,1); len=4; break;

                case SV_SETHP:                  sv_sethp(buf); len=3; break;
                case SV_SETMANA:                sv_setmana(buf); len=3; break;
                case SV_SETRAGE:                sv_setrage(buf); len=3; break;
                case SV_ENDURANCE:		        sv_endurance(buf); len=3; break;
                case SV_LIFESHIELD:		        sv_lifeshield(buf); len=3; break;

                case SV_SETITEM:                sv_setitem(buf); len=10; break;

                case SV_SETORIGIN:              sv_setorigin(buf); len=5; break;
                case SV_SETTICK:                sv_settick(buf); len=5; break;
                case SV_SETCITEM:               sv_setcitem(buf); len=9; break;

                case SV_ACT:                    if (!(game_options&GO_PREDICT)) sv_act(buf);
                                                len=7; break;
                case SV_EXIT:			        len=sv_exit(buf); break;
                case SV_TEXT:                   len=sv_text(buf); break;

                case SV_NAME:			        len=sv_name(buf); break;

                case SV_CONTAINER:		        sv_container(buf); len=6; break;
                case SV_PRICE:			        sv_price(buf); len=6; break;
                case SV_CPRICE:			        sv_cprice(buf); len=5; break;
                case SV_CONCNT:			        sv_concnt(buf); len=2; break;
                case SV_ITEMPRICE:		        sv_itemprice(buf); len=6; break;
                case SV_CONTYPE:		        sv_contype(buf); len=2; break;
                case SV_CONNAME:		        len=sv_conname(buf); break;

                case SV_GOLD:			        sv_gold(buf); len=5; break;

                case SV_EXP:	 		        sv_exp(buf); len=5; break;
                case SV_EXP_USED:		        sv_exp_used(buf); len=5; break;
                case SV_MIL_EXP:	 	        sv_mil_exp(buf); len=5; break;
                case SV_LOOKINV:		        sv_lookinv(buf); len=17+12*4; break;
                case SV_CYCLES:			        sv_cycles(buf); len=5; break;
                case SV_CEFFECT:		        len=sv_ceffect(buf); break;
                case SV_UEFFECT:		        sv_ueffect(buf); len=9; break;

                case SV_SERVER:			        sv_server(buf); len=7; break;

                case SV_REALTIME:               sv_realtime(buf); len=5; break;

                case SV_SPEEDMODE:		        sv_speedmode(buf); len=2; break;
                case SV_FIGHTMODE:		        sv_fightmode(buf); len=2; break;
                case SV_LOGINDONE:		        sv_logindone(); len=1; break;
                case SV_SPECIAL:		        sv_special(buf); len=13; break;
                case SV_TELEPORT:		        sv_teleport(buf); len=13; break;

                case SV_MIRROR:                 sv_mirror(buf); len=5; break;
                case SV_PROF:			        sv_prof(buf); len=21; break;
                case SV_PING:			        len=sv_ping(buf); break;
                case SV_UNIQUE:			        sv_unique(buf); len=5; break;
                case SV_QUESTLOG:		        sv_questlog(buf); len=101+sizeof(struct shrine_ppd); break;
                case SV_PROTOCOL:               sv_protocol(buf); len=2; break;
                case SV_VNQUEST:                len=sv_vnquest(buf); break;

                default:                        len=amod_process(buf);
                                                if (!len) {
                                                    fail("got illegal command %d",buf[0]);
                                                    exit(101);
                                                }
                                                break;
            }

        size-=len; buf+=len;
    }

    if (size) {
        fail("PANIC! size=%d",size); exit(102);
    }
}

int prefetch(unsigned char *buf,int size) {
    int len=0,panic=0,last=-1;
    static int prefetch_tick=0;

    while (size>0 && panic++<20000) {
        if ((buf[0]&(64+128))==SV_MAP01) len=sv_map01(buf,&last,map2);  // ANKH
        else if ((buf[0]&(64+128))==SV_MAP10) len=sv_map10(buf,&last,map2);  // ANKH
        else if ((buf[0]&(64+128))==SV_MAP11) len=sv_map11(buf,&last,map2);  // ANKH
        else switch (buf[0]) {
                case SV_SCROLL_UP:              sv_scroll_up(map2); len=1; break;
                case SV_SCROLL_DOWN:            sv_scroll_down(map2); len=1; break;
                case SV_SCROLL_LEFT:            sv_scroll_left(map2); len=1; break;
                case SV_SCROLL_RIGHT:           sv_scroll_right(map2); len=1; break;
                case SV_SCROLL_LEFTUP:          sv_scroll_leftup(map2); len=1; break;
                case SV_SCROLL_LEFTDOWN:        sv_scroll_leftdown(map2); len=1; break;
                case SV_SCROLL_RIGHTUP:         sv_scroll_rightup(map2); len=1; break;
                case SV_SCROLL_RIGHTDOWN:       sv_scroll_rightdown(map2); len=1; break;

                case SV_SETVAL0:                len=4; break;
                case SV_SETVAL1:                len=4; break;

                case SV_SETHP:                  len=3; break;
                case SV_SETMANA:                len=3; break;
                case SV_SETRAGE:                len=3; break;
                case SV_ENDURANCE:		        len=3; break;
                case SV_LIFESHIELD:		        len=3; break;

                case SV_SETITEM:                if (game_options&GO_PREDICT) sv_setitem(buf);
                                                len=10; break;

                case SV_SETORIGIN:              len=5; break;
                case SV_SETTICK:                prefetch_tick=*(unsigned int *)(buf+1); len=5; break;
                case SV_SETCITEM:               if (game_options&GO_PREDICT) sv_setcitem(buf);
                                                len=9; break;

                case SV_ACT:                    if (game_options&GO_PREDICT) sv_act(buf);
                                                len=7; break;

                case SV_TEXT:                   len=svl_text(buf); break;
                case SV_EXIT:                   len=svl_exit(buf); break;

                case SV_NAME:			        len=svl_name(buf); break;

                case SV_CONTAINER:		        len=6; break;
                case SV_PRICE:			        len=6; break;
                case SV_CPRICE:			        len=5; break;
                case SV_CONCNT:			        len=2; break;
                case SV_ITEMPRICE:		        len=6; break;
                case SV_CONTYPE:		        len=2; break;
                case SV_CONNAME:		        len=svl_conname(buf); break;

                case SV_MIRROR:                 len=5; break;

                case SV_GOLD:			        len=5; break;

                case SV_EXP:	 		        len=5; break;
                case SV_EXP_USED:		        len=5; break;
                case SV_MIL_EXP:		        len=5; break;
                case SV_LOOKINV:		        len=17+12*4; break;
                case SV_CYCLES:			        len=5; break;
                case SV_CEFFECT:		        len=svl_ceffect(buf); break;
                case SV_UEFFECT:		        len=9; break;

                case SV_SERVER:			        len=7; break;
                case SV_REALTIME:               len=5; break;
                case SV_SPEEDMODE:		        len=2; break;
                case SV_FIGHTMODE:		        len=2; break;
                case SV_LOGINDONE:              bzero(map2,sizeof(map2)); len=1; break;
                case SV_SPECIAL:		        len=13; break;
                case SV_TELEPORT:		        len=13; break;
                case SV_PROF:			        len=21; break;
                case SV_PING:			        len=svl_ping(buf); break;
                case SV_UNIQUE:			        len=5; break;
                case SV_QUESTLOG:		        len=101+sizeof(struct shrine_ppd); break;
                case SV_PROTOCOL:               len=2; break;
                case SV_VNQUEST:                len=svl_vnquest(buf); break;

                default:                        len=amod_prefetch(buf);
                                                if (!len) {
                                                    fail("got illegal command %d",buf[0]);
                                                    exit(103);
                                                }
                                                break;
            }

        size-=len; buf+=len;
    }

    if (size) {
        fail("2 PANIC! size=%d",size); exit(104);
    }

    prefetch_tick++;

    return prefetch_tick;
}

DLL_EXPORT void client_send(void *buf,int len) {
    if (len>MAX_OUTBUF-outused) return;

    memcpy(outbuf+outused,buf,len);
    outused+=len;
}

void cmd_move(int x,int y) {
    char buf[16];

    buf[0]=CL_MOVE;
    *(unsigned short int *)(buf+1)=x;
    *(unsigned short int *)(buf+3)=y;
    client_send(buf,5);
}

void cmd_ping(void) {
    char buf[16];

    buf[0]=CL_PING;
    *(unsigned int *)(buf+1)=SDL_GetTicks();
    client_send(buf,5);
}

void cmd_swap(int with) {
    char buf[16];

    buf[0]=CL_SWAP;
    buf[1]=with;
    client_send(buf,2);

    // if two items with identical flags & sprites are swapped the server won't
    // update them so we need to clear the slot in the cache
    hover_invalidate_inv_delayed(with);
}

void cmd_fastsell(int with) {
    char buf[16];

    buf[0]=CL_FASTSELL;
    buf[1]=with;
    client_send(buf,2);
}

void cmd_use_inv(int with) {
    char buf[16];

    buf[0]=CL_USE_INV;
    buf[1]=with;
    client_send(buf,2);
    hover_invalidate_inv(with);
}

void cmd_take(int x,int y) {
    char buf[16];

    buf[0]=CL_TAKE;
    *(unsigned short int *)(buf+1)=x;
    *(unsigned short int *)(buf+3)=y;
    client_send(buf,5);
}

void cmd_look_map(int x,int y) {
    char buf[16];

    buf[0]=CL_LOOK_MAP;
    *(unsigned short int *)(buf+1)=x;
    *(unsigned short int *)(buf+3)=y;
    client_send(buf,5);
}

void cmd_look_item(int x,int y) {
    char buf[16];

    buf[0]=CL_LOOK_ITEM;
    *(unsigned short int *)(buf+1)=x;
    *(unsigned short int *)(buf+3)=y;
    client_send(buf,5);
}

void cmd_look_inv(int pos) {
    char buf[16];

    buf[0]=CL_LOOK_INV;
    *(unsigned char *)(buf+1)=pos;
    client_send(buf,2);
}

void cmd_look_char(int cn) {
    char buf[16];

    buf[0]=CL_LOOK_CHAR;
    *(unsigned short *)(buf+1)=cn;
    client_send(buf,3);
}

void cmd_use(int x,int y) {
    char buf[16];

    buf[0]=CL_USE;
    *(unsigned short int *)(buf+1)=x;
    *(unsigned short int *)(buf+3)=y;
    client_send(buf,5);
}

void cmd_drop(int x,int y) {
    char buf[16];

    buf[0]=CL_DROP;
    *(unsigned short int *)(buf+1)=x;
    *(unsigned short int *)(buf+3)=y;
    client_send(buf,5);
}

void cmd_speed(int mode) {
    char buf[16];

    buf[0]=CL_SPEED;
    buf[1]=mode;
    client_send(buf,2);
}

void cmd_teleport(int nr) {
    char buf[64];

    if (nr>100) {   // ouch
        newmirror=nr-100;
        return;
    }

    buf[0]=CL_TELEPORT;
    buf[1]=nr;
    buf[2]=newmirror;
    client_send(buf,3);
}

void cmd_stop(void) {
    char buf[16];

    buf[0]=CL_STOP;
    client_send(buf,1);
}

void cmd_kill(int cn) {
    char buf[16];

    buf[0]=CL_KILL;
    *(unsigned short int *)(buf+1)=cn;
    client_send(buf,3);
}

void cmd_give(int cn) {
    char buf[16];

    buf[0]=CL_GIVE;
    *(unsigned short int *)(buf+1)=cn;
    client_send(buf,3);
}

void cmd_some_spell(int spell,int x,int y,int chr) {
    char buf[16],len;

    switch (spell) {
        case CL_BLESS:
        case CL_HEAL:
            buf[0]=spell;
            *(unsigned short *)(buf+1)=chr;
            len=3;
            break;

        case CL_FIREBALL:
        case CL_BALL:
            buf[0]=spell;
            if (x) {
                *(unsigned short *)(buf+1)=x;
                *(unsigned short *)(buf+3)=y;
            } else {
                *(unsigned short *)(buf+1)=0;
                *(unsigned short *)(buf+3)=chr;
            }

            len=5;
            break;

        case CL_MAGICSHIELD:
        case CL_FLASH:
        case CL_WARCRY:
        case CL_FREEZE:
        case CL_PULSE:
            buf[0]=spell;
            len=1;
            break;
        default:
            addline("WARNING: unknown spell %d\n",spell);
            return;
    }

    client_send(buf,len);
}

void cmd_raise(int vn) {
    char buf[16];

    buf[0]=CL_RAISE;
    *(unsigned short int *)(buf+1)=vn;
    client_send(buf,3);
}

void cmd_take_gold(int vn) {
    char buf[16];

    buf[0]=CL_TAKE_GOLD;
    *(unsigned int *)(buf+1)=vn;
    client_send(buf,5);
}

void cmd_drop_gold(void) {
    char buf[16];

    buf[0]=CL_DROP_GOLD;
    client_send(buf,1);
}

void cmd_junk_item(void) {
    char buf[16];

    buf[0]=CL_JUNK_ITEM;
    client_send(buf,1);
}

void cmd_text(char *text) {
    char buf[512];
    int len;

    if (!text) return;

    buf[0]=CL_TEXT;

    for (len=0; text[len] && text[len]!='�' && len<254; len++) buf[len+2]=text[len];

    buf[2+len]=0;
    buf[1]=len+1;

    client_send(buf,len+3);
}

void cmd_log(char *text) {
    char buf[512];
    int len;

    if (!text) return;

    buf[0]=CL_LOG;

    for (len=0; text[len] && len<254; len++) buf[len+2]=text[len];

    buf[2+len]=0;
    buf[1]=len+1;

    client_send(buf,len+3);
}

void cmd_con(int pos) {
    char buf[16];

    buf[0]=CL_CONTAINER;
    buf[1]=pos;
    client_send(buf,2);
}

void cmd_con_fast(int pos) {
    char buf[16];

    buf[0]=CL_CONTAINER_FAST;
    buf[1]=pos;
    client_send(buf,2);
}

void cmd_look_con(int pos) {
    char buf[16];

    buf[0]=CL_LOOK_CONTAINER;
    buf[1]=pos;
    client_send(buf,2);
}

void cmd_getquestlog(void) {
    char buf[16];

    buf[0]=CL_GETQUESTLOG;
    client_send(buf,1);
}

void cmd_reopen_quest(int nr) {
    char buf[16];

    buf[0]=CL_REOPENQUEST;
    buf[1]=nr;
    client_send(buf,2);
}

void bzero_client(int part) {
    if (part==0) {
        lasttick=0;
        lastticksize=0;

        bzero(queue,sizeof(queue));
        q_in=q_out=q_size=0;

        server_cycles=0;

        zsinit=0;
        bzero(&zs,sizeof(zs));

        ticksize=0;
        inused=0;
        indone=0;
        login_done=0;
        bzero(inbuf,sizeof(inbuf));

        outused=0;
        bzero(outbuf,sizeof(outbuf));
    }

    if (part==1) {
        act=0;
        actx=0;
        acty=0;

        cflags=0;
        csprite=0;

        originx=0;
        originy=0;
        bzero(map,sizeof(map));

        bzero(value,sizeof(value));
        bzero(item,sizeof(item));
        bzero(item_flags,sizeof(item_flags));
        hp=0;
        mana=0;
        endurance=0;
        lifeshield=0;
        experience=0;
        experience_used=0;
        mil_exp=0;
        gold=0;

        bzero(player,sizeof(player));

        bzero(ceffect,sizeof(ceffect));
        bzero(ueffect,sizeof(ueffect));

        con_cnt=0;
        bzero(container,sizeof(container));
        bzero(price,sizeof(price));
        bzero(itemprice,sizeof(itemprice));
        cprice=0;

        bzero(lookinv,sizeof(lookinv));
        show_look=0;

        pspeed=0;
        pent_str[0][0]=pent_str[1][0]=pent_str[2][0]=pent_str[3][0]=pent_str[4][0]=pent_str[5][0]=pent_str[6][0]=0;

        bzero(may_teleport,sizeof(may_teleport));

        amod_areachange();
        minimap_clear();
    }
}

int close_client(void) {
    if (sock) { SDLNet_TCP_Close(sock); sock=NULL; }
    if (zsinit) { inflateEnd(&zs); zsinit=0; }

    sockstate=0;
    socktime=0;

    bzero_client(0);
    bzero_client(1);

    return 0;
}

#define MAXPASSWORD	16
void decrypt(char *name,char *password) {
    int i;
    static char secret[4][MAXPASSWORD]={
        "\000cgf\000de8etzdf\000dx",
        "jrfa\000v7d\000drt\000edm",
        "t6zh\000dlr\000fu4dms\000",
        "jkdm\000u7z5g\000j77\000g"
    };

    for (i=0; i<MAXPASSWORD; i++) {
        password[i]=password[i]^secret[name[1]%4][i]^name[i%3];
    }
}

int net_init(void) {
    return SDLNet_Init();
}

int net_exit(void) {
    SDLNet_Quit();
    return 0;
}

void send_info(TCPsocket sock) {
    char buf[12];

    /* TODO: Figure out local address */
    /* *(unsigned int *)(buf+0)=sock->localAddress.host; */
    *(unsigned int *)(buf+4)=SDLNet_TCP_GetPeerAddress(sock)->host;

    #ifdef STORE_UNIQUE
    load_unique();
    #endif

    *(unsigned int *)(buf+8)=unique;

    SDLNet_TCP_Send(sock,buf,12);
}

int poll_network(void) {
    int n;

    // something fatal failed (sockstate will somewhen tell you what)
    if (sockstate<0) {
        return -1;
    }

    // create nonblocking socket
    if (sockstate==0 && !kicked_out) {

        IPaddress addr;

        if (SDL_GetTicks()<socktime) return 0;

        // reset socket
        if (sock) { SDLNet_TCP_Close(sock); sock=NULL; }
        if (zsinit) { inflateEnd(&zs); zsinit=0; }

        change_area=0;
        kicked_out=0;

        // reset client
        bzero_client(0);

        if (!socktimeout) {
            socktimeout=time(NULL);
        }

        // connect to server
        if (target_server==NULL) {
            fail("Server URL not specified.");
            return -2;
        } else if(SDLNet_ResolveHost(&addr,target_server,target_port)!=0) {
            fail("Could not resolve server %s.",target_server);
            return -2;
        }

        note("Using login server at %s:%u",SDLNet_ResolveIP(&addr),SDLNet_Read16(&addr.port));

        sock=SDLNet_TCP_Open(&addr);
        if (!sock) {
            fail("creating socket failed (%s)\n",SDLNet_GetError());
            sockstate=-1;   // fail - no retry
            return -1;
        }
        // statechange
        sockstate=2;
    }

    // connected - send password and initialize compression buffers
    if (sockstate==2) {
        char tmp[256];

        // initialize compression
        if (inflateInit(&zs)) {
            note("zsinit failed");
            sockstate=-5;   // fail - no retry
            return -1;
        }
        zsinit=1;

        bzero(tmp,sizeof(tmp));
        strcpy(tmp,username);
        SDLNet_TCP_Send(sock,tmp,40);

        // send password
        bzero(tmp,sizeof(tmp));
        strcpy(tmp,password);
        decrypt(username,tmp);
        SDLNet_TCP_Send(sock,tmp,16);

        *(unsigned int *)(tmp)=(0x8fd46100|0x01);   // magic code + version 1
        SDLNet_TCP_Send(sock,tmp,4);
        send_info(sock);

        // statechange
        sockstate=3;
    }

    // here we go ...
    if (change_area) {
        sockstate=0;
        socktimeout=time(NULL);
        return 0;
    }

    if (kicked_out) {
        sockstate=-6;   // fail - no retry
        close_client();
        return -1;
    }

    // check if we have one tick, so we can reset the map and move to state 4 !!! note that this state has no return statement, so we still read and write)
    if (sockstate==3) {
        if (login_done) { //if (lasttick>1) {
            // statechange
            //note("go ahead (left at tick=%d)",tick);
            //bzero_client(1);
            sockstate=4;
        }
    }

    // send
    if (outused && sockstate==4) {
        n=SDLNet_TCP_Send(sock,outbuf,outused);

        if (n<outused) {
            addline("connection lost during write (%s)\n",SDLNet_GetError());
            sockstate=0;
            socktimeout=time(NULL);
            return -1;
        }

        memmove(outbuf,outbuf+n,outused-n);
        outused-=n;
        sent_bytes+=n;
    }

    // recv
    n=SDLNet_TCP_Recv(sock,(char *)inbuf+inused,MAX_INBUF-inused);
    if (n<=0) {
        addline("connection lost during read (%s)\n",SDLNet_GetError());
        sockstate=0;
        socktimeout=time(NULL);
        return -1;
    }
    inused+=n;
    rec_bytes+=n;

    // count ticks
    while (1) {
        if (inused>=lastticksize+1 && *(inbuf+lastticksize)&0x40) {
            lastticksize+=1+(*(inbuf+lastticksize)&0x3F);
        } else if (inused>=lastticksize+2) {
            lastticksize+=2+(SDLNet_Read16(inbuf+lastticksize)&0x3FFF);
        } else break;

        lasttick++;
    }

    return 0;
}

void auto_tick(struct map *cmap) {
    int x,y,mn;

    // automatically tick map
    for (y=0; y<MAPDY; y++) {
        for (x=0; x<MAPDX; x++) {

            mn=mapmn(x,y);
            if (!(cmap[mn].csprite)) continue;

            cmap[mn].step++;
            if (cmap[mn].step<cmap[mn].duration) continue;
            cmap[mn].step=0;
        }
    }
}

int next_tick(void) {
    int ticksize;
    int size,ret,attick;

    // no room for next tick, leave it in in-queue
    if (q_size==Q_SIZE) return 0;

    // do we have a new tick
    if (inused>=1 && (*(inbuf)&0x40)) {
        ticksize=1+(*(inbuf)&0x3F);
        if (inused<ticksize) return 0;
        indone=1;
    } else if (inused>=2 && !(*(inbuf)&0x40)) {
        ticksize=2+(SDLNet_Read16(inbuf)&0x3FFF);
        if (inused<ticksize) return 0;
        indone=2;
    } else {
        return 0;
    }

    // decompress
    if (*inbuf&0x80) {

        zs.next_in=inbuf+indone;
        zs.avail_in=ticksize-indone;

        zs.next_out=queue[q_in].buf;
        zs.avail_out=sizeof(queue[q_in].buf);

        ret=inflate(&zs,Z_SYNC_FLUSH);
        if (ret!=Z_OK) {
            warn("Compression error %d\n",ret);
            quit=1;
            return 0;
        }

        if (zs.avail_in) { warn("HELP (%d)\n",zs.avail_in); return 0; }

        size=sizeof(queue[q_in].buf)-zs.avail_out;
    } else {
        size=ticksize-indone;
        memcpy(queue[q_in].buf,inbuf+indone,size);
    }
    attick=queue[q_in].size=size;

    auto_tick(map2);
    attick=prefetch(queue[q_in].buf,queue[q_in].size);

    q_in=(q_in+1)%Q_SIZE;
    q_size++;

    // remove tick from inbuf
    if (inused-ticksize>=0) memmove(inbuf,inbuf+ticksize,inused-ticksize);
    else note("kuckuck!");
    inused=inused-ticksize;

    // adjust some values
    lasttick--;
    lastticksize-=ticksize;

    return attick;
}

int do_tick(void) {
    // process tick
    if (q_size>0) {

        auto_tick(map);
        process(queue[q_out].buf,queue[q_out].size);
        q_out=(q_out+1)%Q_SIZE;
        q_size--;
        hover_capture_tick();

        // increase tick
        tick++;
        if (tick%TICKS==0) realtime++;

        return 1;
    }

    return 0;
}

void cl_ticker(void) {
    char buf[256];

    buf[0]=CL_TICKER;
    *(unsigned int *)(buf+1)=tick;
    client_send(buf,5);
}

// X exp yield level Y
DLL_EXPORT int exp2level(int val) {
    if (val<1) return 1;

    return max(1,(int)(sqrt(sqrt(val))));
}

// to reach level X you need Y exp
DLL_EXPORT int level2exp(int level) {
    return pow(level,4);
}

DLL_EXPORT int mapmn(int x,int y) {
    if (x<0 || y<0 || x>=MAPDX || y>=MAPDY) {
        return -1;
    }
    return (x+y*MAPDX);
}

