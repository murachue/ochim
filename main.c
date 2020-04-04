#include <stdint.h>
#include <string.h>
#include <math.h>
#include "regs.h"
#include "cache.h"
#include "rdp.h"
#include "hotload.h"

#define QUOTE(s) #s
#define STRINGIFY(s) QUOTE(s)
#define CLAMP(l,v,h) ((v) < (l) ? (l) : (h) < (v) ? (h) : (v))
#define SGN(v) (((v) < 0) ? -1 : ((v) == 0) ? 0 : 1)
#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))
#define ROUNDIV(dend,sor) ((2 * (dend) / (sor) + 1) / 2)
#define EXTSIZE(sym) ((uintptr_t)(&sym##_size))

#ifdef DEBUG
#define PROFILE
#define assert(expr) if(!(expr)){fatalhandler();}
#else
#define assert(expr) /*nothing*/
#endif

enum STATUS {
	STATUS_IE  = 1 << 0,
	STATUS_IM2 = 1 << 10,
	STATUS_IM4 = 1 << 12,
	STATUS_CU1 = 1 << 29,
};

enum CAUSE {
	CAUSE_IP2 = 1 << 10,
	CAUSE_IP4 = 1 << 12,
};

enum SI_BUTTONS {
	SI_BUTTON_A     = 0x8000,
	SI_BUTTON_B     = 0x4000,
	SI_BUTTON_Z     = 0x2000,
	SI_BUTTON_START = 0x1000,
	SI_BUTTON_HATU  = 0x0800,
	SI_BUTTON_HATD  = 0x0400,
	SI_BUTTON_HATL  = 0x0200,
	SI_BUTTON_HATR  = 0x0100,
	SI_BUTTON_L     = 0x0020,
	SI_BUTTON_R     = 0x0010,
	SI_BUTTON_CU    = 0x0008,
	SI_BUTTON_CD    = 0x0004,
	SI_BUTTON_CL    = 0x0002,
	SI_BUTTON_CR    = 0x0001,
};

struct xyoffset {
	int8_t x, y;
};
typedef struct xyoffset wallkick_t[4][2][4]; // 4 rotations 2 wises 4 tries

extern char rspaudio_text[], rspaudio_data[];
// these have means only in address (absolute symbol). do not use directly but EXTSIZE().
extern char rspaudio_text_size, rspaudio_data_size;
extern char ochimtex[], minotex[], turutex[], numtex[], gtypetex[], guitex[], back2[];
extern int8_t erase1[], erase2[], garbage[], go[], holdfail[], hold[], initialhold[], initialrotate[], linefall[], lock[], ready[], rotate[], step[], point1[], point2[], point3[], point4[], rakka[], regret[], se000026[], ssshk1_r[], move[], sewol001[], brmpshrr[], shakin[], tamaf[], tamaido[], pageenter2[], pageexit2[];
// these have means only in address (absolute symbol). do not use directly but EXTSIZE().
extern char erase1_size, erase2_size, garbage_size, go_size, holdfail_size, hold_size, initialhold_size, initialrotate_size, linefall_size, lock_size, ready_size, rotate_size, step_size, point1_size, point2_size, point3_size, point4_size, rakka_size, regret_size, se000026_size, ssshk1_r_size, move_size, sewol001_size, brmpshrr_size, shakin_size, tamaf_size, tamaido_size, pageenter2_size, pageexit2_size;

#define MINOTEX_FMT DP_FORMAT_IA
#define MINOTEX_SIZE DP_SIZE_8B
#define TURUTEX_FMT DP_FORMAT_IA
#define TURUTEX_SIZE DP_SIZE_8B
#define OCHIMTEX_FMT DP_FORMAT_IA
#define OCHIMTEX_SIZE DP_SIZE_8B
#define NUMTEX_FMT DP_FORMAT_IA
#define NUMTEX_SIZE DP_SIZE_8B
#define GTYPETEX_FMT DP_FORMAT_RGBA
#define GTYPETEX_SIZE DP_SIZE_16B
#define GUITEX_FMT DP_FORMAT_IA
#define GUITEX_SIZE DP_SIZE_8B

static uint16_t prebuttons[4];
static uint16_t __attribute((aligned(64))) cfb[1][640*480];
static int16_t __attribute((aligned(8))) ab[2][368 * 2]; // 22050/60=367.5=~368, 368*2[ch]*16[bit]/8%8[balign]=0
static volatile int32_t nextabi, async;
static uint16_t *updatingcfb, *showingcfb;
static uint8_t __attribute((aligned(8))) si_buf[64];
static volatile uint32_t vsync, curfield;
static uint32_t controller_inputs[4];
static uint32_t randseed = 2463534242;
// beware of modifying struct achannel... you also need to modify rspaudio.S
static volatile struct achannel {
	int8_t *psample;
	int16_t len;
	int16_t looplen;
	int16_t rate;
	int16_t vol;
	// rspaudio states
	int16_t scale;
	int16_t frac;
} __attribute((aligned(8))) achannels[32];

static const wallkick_t wallkick_jlstz = {
	{{{-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}, // 3>0
		{{1, 0}, {1, -1}, {0, 2}, {1, 2}}}, // 1>0
	{{{-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}, // 0>1
		{{-1, 0}, {-1, 1}, {0, -2}, {-1, -2}}}, // 2>1
	{{{1, 0}, {1, -1}, {0, 2}, {1, 2}}, // 1>2
		{{-1, 0}, {-1, -1}, {0, 2}, {-1, 2}}}, // 3>2
	{{{1, 0}, {1, 1}, {0, -2}, {1, -2}}, // 2>3
		{{1, 0}, {1, 1}, {0, -2}, {1, -2}}} // 0>3
};
static const wallkick_t wallkick_i = {
	{{{1, 0}, {-2, 0}, {1, -2}, {-2, 1}}, // 3>0
		{{2, 0}, {-1, 0}, {2, 1}, {-1, -2}}}, // 1>0
	{{{-2, 0}, {1, 0}, {-2, -1}, {1, 2}}, // 0>1
		{{1, 0}, {-2, 0}, {1, -2}, {-2, 1}}}, // 2>1
	{{{-1, 0}, {2, 0}, {-1, 2}, {2, -1}}, // 1>2
		{{-2, 0}, {1, 0}, {-2, -1}, {1, 2}}}, // 3>2
	{{{2, 0}, {-1, 0}, {2, 1}, {-1, -2}}, // 2>3
		{{-1, 0}, {2, 0}, {-1, 2}, {2, -1}}} // 0>3
};
static const struct piece {
	int8_t block;
	int8_t blocks[4][4][4];
	const wallkick_t *pwallkick;
} pieces[7] = {
	{1, {{{0, 0, 0, 0},{0, 0, 0, 0},{1, 1, 1, 1},{0, 0, 0, 0}}, // I
		 {{0, 0, 1, 0},{0, 0, 1, 0},{0, 0, 1, 0},{0, 0, 1, 0}},
		 {{0, 0, 0, 0},{1, 1, 1, 1},{0, 0, 0, 0},{0, 0, 0, 0}},
		 {{0, 1, 0, 0},{0, 1, 0, 0},{0, 1, 0, 0},{0, 1, 0, 0}}
	 }, &wallkick_i},
	{2, {{{0, 0, 0, 0},{0, 0, 0, 0},{2, 2, 2, 0},{2, 0, 0, 0}}, // J
		 {{0, 0, 0, 0},{0, 2, 0, 0},{0, 2, 0, 0},{0, 2, 2, 0}},
		 {{0, 0, 0, 0},{0, 0, 2, 0},{2, 2, 2, 0},{0, 0, 0, 0}},
		 {{0, 0, 0, 0},{2, 2, 0, 0},{0, 2, 0, 0},{0, 2, 0, 0}}
	 }, &wallkick_jlstz},
	{3, {{{0, 0, 0, 0},{0, 0, 0, 0},{3, 3, 3, 0},{0, 0, 3, 0}}, // L
		 {{0, 0, 0, 0},{0, 3, 3, 0},{0, 3, 0, 0},{0, 3, 0, 0}},
		 {{0, 0, 0, 0},{3, 0, 0, 0},{3, 3, 3, 0},{0, 0, 0, 0}},
		 {{0, 0, 0, 0},{0, 3, 0, 0},{0, 3, 0, 0},{3, 3, 0, 0}}
	 }, &wallkick_jlstz},
	{4, {{{0, 0, 0, 0},{0, 0, 0, 0},{0, 4, 4, 0},{0, 4, 4, 0}}, // O
		 {{0, 0, 0, 0},{0, 0, 0, 0},{0, 4, 4, 0},{0, 4, 4, 0}},
		 {{0, 0, 0, 0},{0, 0, 0, 0},{0, 4, 4, 0},{0, 4, 4, 0}},
		 {{0, 0, 0, 0},{0, 0, 0, 0},{0, 4, 4, 0},{0, 4, 4, 0}}
	 }, 0},
	{5, {{{0, 0, 0, 0},{0, 0, 0, 0},{5, 5, 0, 0},{0, 5, 5, 0}}, // S
		 {{0, 0, 0, 0},{0, 0, 5, 0},{0, 5, 5, 0},{0, 5, 0, 0}},
		 {{0, 0, 0, 0},{5, 5, 0, 0},{0, 5, 5, 0},{0, 0, 0, 0}},
		 {{0, 0, 0, 0},{0, 5, 0, 0},{5, 5, 0, 0},{5, 0, 0, 0}}
	 }, &wallkick_jlstz},
	{6, {{{0, 0, 0, 0},{0, 0, 0, 0},{6, 6, 6, 0},{0, 6, 0, 0}}, // T
		 {{0, 0, 0, 0},{0, 6, 0, 0},{0, 6, 6, 0},{0, 6, 0, 0}},
		 {{0, 0, 0, 0},{0, 6, 0, 0},{6, 6, 6, 0},{0, 0, 0, 0}},
		 {{0, 0, 0, 0},{0, 6, 0, 0},{6, 6, 0, 0},{0, 6, 0, 0}}
	 }, &wallkick_jlstz},
	{7, {{{0, 0, 0, 0},{0, 0, 0, 0},{0, 7, 7, 0},{7, 7, 0, 0}}, // Z
		 {{0, 0, 0, 0},{0, 7, 0, 0},{0, 7, 7, 0},{0, 0, 7, 0}},
		 {{0, 0, 0, 0},{0, 7, 7, 0},{7, 7, 0, 0},{0, 0, 0, 0}},
		 {{0, 0, 0, 0},{7, 0, 0, 0},{7, 7, 0, 0},{0, 7, 0, 0}}
	 }, &wallkick_jlstz}
};

#ifdef PROFILE
static struct {
	int32_t prev;
	int32_t offset;

	int32_t start;
	int32_t input;
	int32_t drawcmd;
	int32_t drawwait;
	int32_t frame; /* special */
	int32_t amix;
	int32_t hotload;
	int32_t halt;
	int32_t end;
} curprofile, oldprofile;
#endif

struct mino {
	int8_t field[22][10];
	int32_t piece, rotate;
	int32_t x, y;
	int32_t last4[4];
	int32_t next;
	int32_t hold;
	int holded;
	int32_t combo;
	int32_t goys[10]; // s15.16

	enum {
		MINO_READY,
		MINO_PLACING,
		MINO_GROUNDING,
		MINO_FLASHING,
		MINO_ERASING,
		MINO_ARE,
		MINO_AREROCKED,
		MINO_GAMEOVER,
	} phase;
	int32_t phaset;

	int32_t fallspeed;
	int32_t softdropmul;
	int32_t groundwait;
	int32_t are;
	int32_t erasewait;
	int32_t rocksperturn;
	int32_t initrocks;
};

struct turu {
	int8_t field[13][6];
	int8_t floating[13][6];
	int32_t piece[2], rotate, wantrotate, next[2];
	int32_t x, y;
	int32_t combo;
	enum {
		TURU_READY,
		TURU_PLACING,
		TURU_FALLING,
		TURU_BOUNCING,
		TURU_ERASING,
		TURU_ROCKFALLING,
		TURU_ROCKSETTLING,
		TURU_GAMEOVER,
	} phase;
	int32_t phaset;
	uint32_t colors[8];
	int32_t csx, csy, psx, psy; // core/pair shift
	int32_t cdx, cdy, pdx, pdy; // core/pair (shift)delta
	int32_t falldy;
	int32_t hiddenrocks;
	int32_t rocki;
	int32_t goys[6]; // s15.16

	int32_t ncolors;
	int32_t fallspeed;
	int32_t groundwait;
	int32_t erasewait;
	int32_t rocksperturn;
	int32_t initrocks;
};

struct player {
	struct player *next;
	int32_t id;

	enum {
		GAMETYPE_MINO,
		GAMETYPE_TURU,
	} gametype;
	struct mino mino;
	struct turu turu;

	int32_t repeatt;

	int32_t rocks; // this can be temporarily negative to indicate "I should do counter attack later" (when flyrock reached me later)
	int32_t pendrocks;
	uint32_t rockingfrom;
	enum {
		ROCK_SOLID,
		ROCK_SHRINKING,
		ROCK_EXPANDING
	} rockstate;
	int32_t rockt; // s15.16

	int32_t score;
	enum {
		PLAYER_CONFIG,
		PLAYER_READY,
		PLAYER_INGAME,
		PLAYER_PAUSE,
		PLAYER_GAMEOVER,
	} gamestate;
	int32_t gamestatet;
	int leader;

	int32_t confitem;

	int32_t das;
	int32_t repeatwait;
};

struct flyrock {
	// coords are all s15.16
	int32_t x, y;
	int32_t dx, dy;
	int32_t dx2, dy2;
	int32_t tx, ty;

	int32_t nrocks; // just for effect.
	struct player *dstplayer, *srcplayer;
};

static struct {
	struct player players[4];
	struct flyrock flyingrocks[64]; // max 16 flyrocks per player
	int32_t t;
	int bgtex;
	int gamedraw;
} gstate;

extern void halt(void);

static void init_video(void) {
	showingcfb = &cfb[0][0];
	updatingcfb = &cfb[0][0];

	VI->control  = 0x0000025E;
	VI->dramaddr = (uint32_t)showingcfb;
	VI->width    = 640;
	VI->intr     = 520; // almost end of screen
	VI->current  = 0; // resetintr
	VI->timing   = 0x03e52239; // colorburst=03e vsync=5 colorburst=22 hsync=39
	VI->vsync    = 524;
	VI->hsync    = (0 << 16) | 3093; // in 1/4px
	VI->hsync_leap = (3093 << 16) | 3093;
	VI->hrange   = (108 << 16) | 748;
	VI->vrange   = (35 << 16) | 509;
	VI->vburstrange = (14 << 16) | 516;
	VI->xscale   = ((1024 * 0 / 1) << 16) | (1024 * 1 / 1); // *1.0 + 0.0
	VI->yscale   = ((1024 * 0 / 1) << 16) | (1024 * 2 / 1); // *2.0 + 0.0
}

static void init_audio(void) {
	bzero(ab, sizeof(ab));
	AI->control = 1; // DMA enable
	AI->status = 0; // clear interrupt if raised
	AI->dacrate = ROUNDIV(48681812/*NTSC clockrate*/, 22050) - 1;
	AI->bitrate = 16 - 1;
	nextabi = 0;

	bzero((void*)achannels, sizeof(achannels)); // ok volatile
	dwbinval((void*)achannels, sizeof(achannels)); // ok volatile

	// kick first
	AI->dramaddr = (uint32_t)&ab[0];
	AI->len = sizeof(ab[0]);
}

static inline uint32_t getsr(void) {
	register uint32_t r;
	__asm volatile("mfc0 %0, $12" : "=r"(r));
	return r;
}

static inline void setsr(uint32_t v) {
	__asm volatile("mtc0 %0, $12" :: "r"(v));
}

static inline uint32_t getcause(void) {
	register uint32_t r;
	__asm volatile("mfc0 %0, $13" : "=r"(r));
	return r;
}

static inline uintptr_t getepc(void) {
	register uint32_t r;
	__asm volatile(STRINGIFY(MAFC0) " %0, $14" : "=r"(r));
	return r;
}

static inline void setepc(uintptr_t v) {
	__asm volatile(STRINGIFY(MATC0) " %0, $14" : : "r"(v));
}

static inline uint32_t getcount(void) {
	register uint32_t r;
	__asm volatile("mfc0 %0, $9" : "=r"(r));
	return r;
}

#ifdef PROFILE
// (93750000/2)[1s] < (2^28)
static const int32_t prof_wrap = 1 << 28;

static int32_t prof_time_raw(void) {
	return getcount() & (prof_wrap - 1);
}

static int32_t prof_time(void) {
	uint32_t sr = getsr();
	setsr(sr & ~1);

	int32_t c = prof_time_raw();
	if(c < curprofile.prev) {
		curprofile.offset += prof_wrap;
	}
	curprofile.prev = c;

	setsr(sr);

	return curprofile.offset + c;
}

static void prof_newframe(void) {
	uint32_t sr = getsr();
	setsr(sr & ~1);

	curprofile.end = prof_time();

	oldprofile = curprofile;

	curprofile.offset = 0;
	curprofile.prev = curprofile.start = prof_time_raw(); /* using raw and chaining set to prev just for premature optimize */

	setsr(sr);
}

#define PROF(_name) do{ curprofile._name = prof_time(); }while(0)
#else
#define prof_time()
#define prof_newframe()
#define PROF(_name)
#endif

static void amix(volatile int16_t *pab) {
#if 0
	const int32_t absamples = ARRAY_SIZE(ab[0]) / 2;

	bzero((void*)pab, sizeof(ab[0])); // ok volatile
	for(int32_t c = 0; c < ARRAY_SIZE(achannels); c++) {
		if(!achannels[c].len) {
			continue;
		}

		int8_t *pchs = (void*)((uint32_t)achannels[c].psample | 0x80000000);
		int32_t chlen = achannels[c].len;
		int32_t mixlen = chlen < absamples ? chlen : absamples;
		int32_t comax = 0, scale = achannels[c].scale;
		for(int32_t i = 0; i < mixlen; i++) {
			int8_t co = pchs[i / 2];
			if(i % 2 == 1) {
				co <<= 4;
			}
			co >>= 4;
			int32_t rs = pab[i*2] + co * scale;
			int16_t s = CLAMP(-32768, rs, 32767);
			pab[i*2] = pab[i*2+1] = s;
			co = co < 0 ? -co : co;
			co = (co - 3) < 0 ? 0 : (co - 3);
			comax = comax < co ? co : comax;
			if((i & 7) == 7) {
				static const int32_t factor[5] = {0x0E90, 0x1360, 0x1770, 0x1C50, 0x2250};
				scale = (scale * factor[comax]) >> 12;
				comax = 0;
			}
		}
		achannels[c].psample += mixlen / 2;
		achannels[c].len -= mixlen;
		achannels[c].scale = scale;
	}

	dwbinval((void*)pab, sizeof(ab[0])); // ok volatile
#else
	SP->status = SP_STATUS_SET_HALT;
	while(!(SP->status & SP_STATUS_HALT)) /*busyloop*/ ;

	SP->spmemaddr = 0x0000;
	SP->dramaddr = (uint32_t)rspaudio_data & 0x00FFffff;
	SP->dram2spmem = EXTSIZE(rspaudio_data) - 1;
	while(SP->status & SP_STATUS_DMA_BUSY) /*busyloop*/ ;
	SP->spmemaddr = 0x1000;
	SP->dramaddr = (uint32_t)rspaudio_text & 0x00FFffff;
	SP->dram2spmem = EXTSIZE(rspaudio_text) - 1;
	while(SP->status & SP_STATUS_DMA_BUSY) /*busyloop*/ ;

	dwbinval((void*)achannels, sizeof(achannels)); // ok volatile
	dinval((void*)pab, sizeof(ab[0])); // ok volatile

	volatile uint32_t *p = (void*)&SP->dmem[0x080];
	p[0] = (uint32_t)achannels & 0x00FFffff;
	p[1] = (uint32_t)pab & 0x00FFffff;

	SP->pc = 0x000;
	SP->status = SP_STATUS_CLEAR_HALT | SP_STATUS_CLEAR_BROKE;

	while(!(SP->status & SP_STATUS_HALT)) /*busyloop*/ ;
#endif
}

static int aplay_(int8_t *psamples, uintptr_t size) {
	for(int32_t i = 0; i < ARRAY_SIZE(achannels); i++) {
		if(achannels[i].len) {
			continue;
		}

		achannels[i].psample = (void*)((uint32_t)psamples & 0x00FFffff);
		achannels[i].len = size * 2; // bytes2samples
		achannels[i].looplen = 0; // TODO
		achannels[i].rate = 0x0800; // TODO playrate*samplingrate(sr/22050)
		achannels[i].scale = 16;
		achannels[i].frac = 0;
		return 1;
	}
	return 0;
}
#define aplay(sym) aplay_(sym, EXTSIZE(sym))

void inthandler(void) {
	uint32_t ip = getcause() & getsr() & 0xFF00;

	if(ip & CAUSE_IP2) {
		// MI
		uint32_t irq = MI->intr & MI->intrmask;

		if(irq & MI_INTR_VI) {
			uint32_t cf = VI->current & 1;

			VI->current = 0; // ack

			//VI->yscale   = ((1024 * cf / 1) << 16) | (1024 * 2 / 1); // *2.0 + cf.0 does not work.
#ifndef EMU
			VI->dramaddr = (uint32_t)showingcfb + cf * 640 * 2;
#endif
			/* NOTE: do NOT update VI->intr here, it clears interlace field info in VI->current! */

			vsync = 1;
			gstate.t++;
			curfield = !cf;

			PROF(frame);
		}

		if(irq & MI_INTR_AI) {
			AI->status = 0; // ack
			nextabi = (nextabi + 1) % ARRAY_SIZE(ab);
			AI->dramaddr = (uint32_t)&ab[nextabi];
			AI->len = sizeof(ab[0]);
			async = 1;
		}
	}
	if(ip & CAUSE_IP4) {
		// PreNMI

		// stop audio
		AI->control = 0;

		// make SP prepared for reset. NOTE: or SP hangs after reset (does not halt/break as coded)
		// halt SP...
		SP->status = SP_STATUS_SET_HALT;
		while(!(SP->status & SP_STATUS_HALT)) /*busyloop*/ ;
		// ...then set PC. setting PC is very important, or it hangs!!! (why??? pipeline?)
		SP->pc = 0;

		// make display clean
		// TODO: DP->status |= DP_STATUS_SET_FLUSH and wait and clear flush
		while(DP->status & DP_STATUS_CMD_BUSY) /*busyloop*/ ;
		bzero(&cfb[0][0], 640*480*2);
		VI->dramaddr = (uint32_t)&cfb[0][0];
		VI->yscale = ((1024 * 0 / 2) << 16) | (1024 * 1 / 1); // *1.0 + 0.0 // workaround...?

		// wait for reset NMI...
		for(;;);
	}

	// if halting, exit from there
	if(getepc() == (uint32_t)halt) {
		setepc(getepc() + 8);
	}
}

void fatalhandler(void) {
	// edge red
	for(uint32_t i = 0; i < 640*30; i++) {
		cfb[0][i] = 0xF800;
	}
	for(uint32_t y = 30; y < 450; y++) {
		for(uint32_t x = 0; x < 30; x++) {
			cfb[0][y*640+x] = 0xF800;
		}
		for(uint32_t x = 610; x < 640; x++) {
			cfb[0][y*640+x] = 0xF800;
		}
	}
	for(uint32_t i = 640*450; i < 640*480; i++) {
		cfb[0][i] = 0xF800;
	}

	dwbinvalall();

	VI->dramaddr = (uint32_t)&cfb[0][0];

#ifndef EMU
	extern void stub_install(void); extern void stub(void); stub_install(); stub();
#else
	*(int*)0 = 0;
#endif

	for(;;);
}

static uint8_t *_si_put_read_buttons(uint8_t *p) {
	*p++ = 0xFF; // padding for aligning rx to word
	*p++ = 0x01; // tx=1bytes
	*p++ = 0x04; // rx=4bytes (after recv: become status)
	*p++ = 0x01; // tx[0]: cmd 0x01: get buttons/stick
	*p++ = 0xFF; // rx[0]
	*p++ = 0xFF; // rx[1]
	*p++ = 0xFF; // rx[2]
	*p++ = 0xFF; // rx[3]
	return p;
}

static void si_refresh_inputs(void) {
	{
		uint8_t *p = si_buf;
		bzero(si_buf, 64);
		p = _si_put_read_buttons(p);
		p = _si_put_read_buttons(p);
		p = _si_put_read_buttons(p);
		p = _si_put_read_buttons(p);
		*p++ = 0xFE; // end of commands
		si_buf[63] = 0x01; // go
	}

	dwbinval(si_buf, sizeof(si_buf)); // writeback & invalidate

	while(SI->status & (SI_STATUS_IOBUSY | SI_STATUS_DMABUSY)) /*nothing*/ ;

	SI->dramaddr = (uint32_t)si_buf & 0x00FFffff;
	SI->dram2pifram = 0x1FC007C0; // PJ64 requires 1FC0xxxx
	/* NOTE: don't touch si_buf from here till read back!! */
	while(SI->status & (SI_STATUS_IOBUSY | SI_STATUS_DMABUSY)) /*nothing*/ ;

	SI->dramaddr = (uint32_t)si_buf & 0x00FFffff;
	SI->pifram2dram = 0x1FC007C0;
	while(SI->status & (SI_STATUS_IOBUSY | SI_STATUS_DMABUSY)) /*nothing*/ ;

	for(int32_t i = 0; i < 4; i++) {
		if((si_buf[8 * i + 2] & 0xC0) != 0x00) { /* 80=no_controller 40=tx/rx_failed */
			controller_inputs[i] = -1; /* use virtually impossible value as "invalid" marker */
		} else {
			controller_inputs[i] = *((uint32_t*)(si_buf + 8 * i + 4));
		}
	}
}

static uint32_t rand(void) {
	randseed ^= randseed << 13;
	randseed ^= randseed >> 17;
	randseed ^= randseed << 5;
	return randseed;
}

static void randstir(uint32_t entropy) {
	randseed = rand() + entropy;
}

// TODO: if 480i can be one frame, remove "flipping" that is nonsense.
static void flip(void) {
	if(updatingcfb == &cfb[0][0]) {
		showingcfb = &cfb[0][0];
		updatingcfb = &cfb[0/*1*/][0];
	} else {
		showingcfb = &cfb[0/*1*/][0];
		updatingcfb = &cfb[0][0];
	}
}

static void add_score(struct player *player, int32_t score) {
	player->score += score;
	if(99999 < player->score) {
		player->score = 99999;
	}
}

static struct flyrock *rock_getfly(void) {
	struct flyrock *fr = gstate.flyingrocks;
	for(int32_t i = 0; i < ARRAY_SIZE(gstate.flyingrocks); i++, fr++) {
		if(!fr->dstplayer) {
			return fr;
		}
	}
	fatalhandler(); // ENOMEM
	return 0; // NOTREACHED
}

static void rock_pid2xy(int32_t pid, int32_t *x, int32_t *y) {
	*x = (12 + (14*10 + 15) * pid + 14*10/2) << 16;
	*y = (30 + 14*2) << 16;
}

static void rock_makefly(struct player *dstplayer, struct player *srcplayer, int32_t srcx, int32_t srcy, int32_t nrocks) {
	dstplayer->rockingfrom |= 1 << srcplayer->id;

	struct flyrock *fr = rock_getfly();
	fr->x = srcx;
	fr->y = srcy;
	fr->dx = 0;
	fr->dy = 0;
	rock_pid2xy(dstplayer->id, &fr->tx, &fr->ty);
	double r = atan2(fr->ty - fr->y, fr->tx - fr->x);
	fr->dx2 = cos(r) * 0x10000;
	fr->dy2 = sin(r) * 0x10000;
	fr->dstplayer = dstplayer;

	fr->nrocks = nrocks;
	fr->srcplayer = srcplayer;
}

static void rock_update(struct player *player, int32_t delta) {
	player->pendrocks += delta;

	switch(player->rockstate) {
	case ROCK_SOLID:
		if(0 < player->rocks) {
			// update
			player->rockstate = ROCK_SHRINKING;
			player->rockt = 0x10000;
		} else {
			// new, or waiting to counter
			player->rocks += player->pendrocks;
			player->pendrocks = 0;

			if(0 < player->rocks) {
				// new
				player->rockstate = ROCK_EXPANDING;
				player->rockt = 0;
			}
		}
		break;
	case ROCK_SHRINKING:
		// do nothing
		break;
	case ROCK_EXPANDING:
		player->rockstate = ROCK_SHRINKING;
		// left rockt as is.
		break;
	}
}

static void rock_gen(struct player *player, int32_t srcx, int32_t srcy, int32_t nrocks) {
	if(nrocks == 0) {
		return;
	}

	// update pendrocks in this func to be able to counter before flyrocks are reached.

	if(0 < player->rocks + player->pendrocks) {
		// counter (and possibly attack)
		rock_update(player, -nrocks);
		rock_makefly(player, player, srcx, srcy, nrocks); // counter is fly to self
		return;
	}

	// attack
	for(struct player *dp = player->next; dp != player; dp = dp->next) {
		if(dp->gamestate != PLAYER_INGAME) {
			continue;
		}

		dp->pendrocks += nrocks; // add now for quick counter
		rock_makefly(dp, player, srcx, srcy, nrocks);
	}
}

static void rock_step(void) {
	for(int32_t i = 0; i < ARRAY_SIZE(gstate.players); i++) {
		struct player *player = &gstate.players[i];
		switch(player->rockstate) {
		case ROCK_SOLID:
			break;
		case ROCK_SHRINKING:
			player->rockt -= 0x10000 / 10;
			if(player->rockt <= 0) {
				player->rocks += player->pendrocks;
				player->pendrocks = 0;

				if(0 < player->rocks) {
					player->rockstate = ROCK_EXPANDING;
					player->rockt = 0;
				} else {
					player->rockstate = ROCK_SOLID;
					player->rockt = 0;
				}
			}
			break;
		case ROCK_EXPANDING:
			player->rockt += 0x10000 / 30;
			if(0x10000 <= player->rockt) {
				player->rockstate = ROCK_SOLID;
				player->rockt = 0;
			}
			break;
		}
	}

	struct flyrock *fr = gstate.flyingrocks;
	for(int32_t i = 0; i < ARRAY_SIZE(gstate.flyingrocks); i++, fr++) {
		struct player *dp = fr->dstplayer;
		if(!dp) {
			continue;
		}

		if(SGN(fr->x - fr->tx) != SGN(fr->x + fr->dx - fr->tx) || SGN(fr->y - fr->ty) != SGN(fr->y + fr->dy - fr->ty)) {
			// target reached, effect!
			rock_update(dp, 0); // just for start rock animation

			// free
			fr->dstplayer = 0;

			int32_t nremrocks = -(dp->rocks + dp->pendrocks);
			if(fr->srcplayer == dp) {
				if(0 < nremrocks) {
					// this is countering against my rocks. let's counter attack to other!
					int32_t x, y;
					rock_pid2xy(dp->id, &x, &y);
					// nullify before rock_gen to avoid recursive counter
					dp->rocks = 0;
					dp->pendrocks = 0;
					rock_gen(dp, x, y, nremrocks);
				}
				aplay(sewol001);
			} else {
				aplay(regret);
			}
		} else {
			// still flying
			fr->x += fr->dx;
			fr->y += fr->dy;
			fr->dx += fr->dx2;
			fr->dy += fr->dy2;
		}
	}
}

static void mino_gennext(struct mino *mino) {
	int32_t p;
	for(int32_t try = 0; try < 6; try++) {
		p = rand() % 7;
		int32_t i;
		for(i = 0; i < 4; i++) {
			if(p == mino->last4[i]) {
				break;
			}
		}
		if(i < 4) {
			continue;
		}
		break;
	}
	mino->next = p;

	for(int32_t i = 0; i < 3; i++) {
		mino->last4[i] = mino->last4[i + 1];
	}
	mino->last4[3] = mino->next;
}

static int mino_placable(struct mino *mino, int32_t x, int32_t y, int32_t rot) {
	for(int32_t dy = 0; dy < 4; dy++) {
		for(int32_t dx = 0; dx < 4; dx++) {
			if(!pieces[mino->piece].blocks[rot][dy][dx]) {
				continue;
			}

			// edge
			if(x + dx < 0) { return 0; }
			if(10 <= x + dx) { return 0; }
			if(y + dy < 0) { return 0; }

			// field
			if(mino->field[y + dy][x + dx]) { return 0; }
		}
	}
	return 1;
}

static int mino_grounded(struct mino *mino) {
	return mino_placable(mino, mino->x, mino->y, mino->rotate) && !mino_placable(mino, mino->x, mino->y - 1, mino->rotate);
}

static void mino_moved(struct mino *mino, int dont_reset_fallt) {
	if(mino_grounded(mino)) {
		// always reset timer
		mino->phase = MINO_GROUNDING;
		mino->phaset = mino->groundwait;
		// TODO forbid eternaloop
		aplay(step);
	} else {
		if(mino->phase != MINO_PLACING || !dont_reset_fallt) {
			mino->phase = MINO_PLACING;
			mino->phaset = 60;
		}
	}
}

static void mino_rotate(struct mino *mino, int rot) {
	int32_t roted = (mino->rotate + rot) & 3;
	int32_t wise = rot < 0 ? 1 : 0;
	int was_grounded = mino_grounded(mino);

	if(mino_placable(mino, mino->x, mino->y, roted)) {
		mino->rotate = roted;
		mino_moved(mino, !was_grounded);
		return;
	}
	if(!pieces[mino->piece].pwallkick) {
		return;
	}
	for(int32_t i = 0; i < 4; i++) {
		const struct xyoffset *offset = &(*pieces[mino->piece].pwallkick)[roted][wise][i];
		if(!mino_placable(mino, mino->x + offset->x, mino->y + offset->y, roted)) {
			continue;
		}

		mino->rotate = roted;
		mino->x += offset->x;
		mino->y += offset->y;
		mino_moved(mino, !was_grounded);
		return;
	}
}

static void mino_slide(struct mino *mino, int dir) {
	if(!mino_placable(mino, mino->x + dir, mino->y, mino->rotate)) {
		return;
	}

	int was_grounded = mino_grounded(mino);

	mino->x += dir;

	mino_moved(mino, !was_grounded);
}

static void mino_freeze(struct player *player) {
	struct mino * const mino = &player->mino;
	// piece2field
	for(int32_t dy = 0; dy < 4; dy++) {
		for(int32_t dx = 0; dx < 4; dx++) {
			int8_t block = pieces[mino->piece].blocks[mino->rotate][dy][dx];
			if(block) {
				mino->field[mino->y + dy][mino->x + dx] = block;
			}
		}
	}

	// erase
	int32_t eraselines = 0;
	int32_t miny = 20, maxy = 0;
	for(int32_t dy = 0; dy < 4; dy++) {
		int32_t y = mino->y + dy;
		if(y < 0) {
			continue;
		}
		int32_t x;
		for(x = 0; x < 10; x++) {
			if(mino->field[y][x] == 0) {
				break;
			}
		}
		if(x == 10) {
			for(x = 0; x < 10; x++) {
				mino->field[y][x] = 0;
			}
			eraselines++;
			miny = y < miny ? y : miny;
			maxy = maxy < y ? y : maxy;
			// TODO effect
		}
	}
	if(eraselines) {
		static const int32_t scoretab[4] = {100, 200, 400, 600};
		add_score(player, scoretab[eraselines - 1]);
		mino->phase = MINO_ERASING;
		mino->phaset = mino->erasewait;

		mino->combo++;

		// TODO how many nrocks...
		int32_t x, y;
		rock_pid2xy(player->id, &x, &y);
		rock_gen(player, x + (-14*10/2 + (mino->x + 2) * 14) * 0x10000, y + (19 * 14 - miny * 14 - (maxy - miny + 1) * 14 / 2) * 0x10000, 4 * eraselines);

		if(eraselines < 2) {
			aplay(erase1);
		} else {
			aplay(erase2);
		}
	} else {
		mino->combo = 0;

		for(int32_t i = 0; i < ARRAY_SIZE(gstate.players); i++) {
			gstate.players[i].rockingfrom &= ~(1 << player->id);
		}

		if(5 < mino->are) {
			mino->phase = MINO_FLASHING;
			mino->phaset = 5;
		} else {
			// skip flashing if ARE are too short
			mino->phase = MINO_ARE;
			mino->phaset = 0;
		}
		aplay(lock);
	}

	// reset repeat on freeze
	player->repeatt = player->das;
}

static void mino_fall(struct player *player, int softdrop) {
	struct mino * const mino = &player->mino;

	int32_t fallspeed = mino->fallspeed * (softdrop ? mino->softdropmul : 1);

	mino->phaset -= fallspeed;
	if(0 < mino->phaset) {
		return;
	}

	// FIXME: this spec makes 20G no score, that is not intuitive.
	if(softdrop) {
		add_score(player, 1);
	}

	while(mino->phaset <= 0) {
		if(!mino_placable(mino, mino->x, mino->y - 1, mino->rotate)) {
			break;
		}
		mino->y--;
		mino->phaset += 60;
	}

	mino_moved(mino, 0);
}

static int32_t mino_lowesty(struct mino *mino) {
	int32_t y;
	for(y = mino->y; -3 < y; y--) {
		if(!mino_placable(mino, mino->x, y - 1, mino->rotate)) {
			return y;
		}
	}
	return y;
}

static void mino_harddrop(struct player *player) {
	struct mino * const mino = &player->mino;
	int32_t ly = mino_lowesty(mino);
	add_score(player, (mino->y - ly) * 2); // FIXME: this spec makes 20G no score, that is not intuitive.
	// note: we can't call mino_moved for sound (after y=ly), because aplay(step) even when already grounded.
	if(mino->y != ly) {
		aplay(step);
	}
	mino->y = ly;
	// NOTE: don't mino_freeze here, or player can rotate/shift after erased but also floating piece because rot/shift after here!!
	mino->phase = MINO_GROUNDING;
	mino->phaset = 1;
}

static void mino_tsumo(struct mino *mino) {
	mino->piece = mino->next;
	mino_gennext(mino);
}

static void mino_hold(struct mino *mino) {
	int8_t holdedpiece = mino->hold;

	mino->hold = mino->piece;

	if(holdedpiece == -1) {
		// first time
		mino_tsumo(mino);
	} else {
		// swap
		mino->piece = holdedpiece;
	}
}

static void mino_spawned(struct player *player, int32_t initrot) {
	struct mino * const mino = &player->mino;

	mino->x = 3;
	mino->y = 18;

	// TODO next sound

	// initrot if can
	mino->rotate = (initrot < 0) ? 3 : (0 < initrot) ? 1 : 0;
	if(mino->rotate != 0 && mino_placable(mino, mino->x, mino->y, mino->rotate)) {
		aplay(initialrotate);
	} else {
		mino->rotate = 0;
	}

	// TODO: gameover when holded??
	if(!mino_placable(mino, mino->x, mino->y, mino->rotate)) {
		mino->phase = MINO_GAMEOVER;
		mino->phaset = 0;
		for(int32_t i = 0; i < ARRAY_SIZE(mino->goys); i++) {
			mino->goys[i] = -(rand() % (0x10000 * 14));
		}
		aplay(brmpshrr);
		return;
	}

	mino_moved(mino, 0);
}

static void mino_spawn(struct player *player, int32_t initrot, int inithold) {
	struct mino * const mino = &player->mino;

	mino_tsumo(mino);

	mino->holded = 0;

	if(inithold) {
		mino_hold(mino);

		aplay(initialhold);
	}

	mino_spawned(player, initrot);
}

static void mino_falllines(struct mino *mino) {
	int32_t y;
	int32_t offy = 0;
	for(y = 0; y < 21; y++) {
		int32_t x;
		for(x = 0; x < 10; x++) {
			if(mino->field[y][x] != 0) {
				break;
			}
		}
		if(x == 10) {
			offy++;
		} else if(offy) {
			for(x = 0; x < 10; x++) {
				mino->field[y - offy][x] = mino->field[y][x];
			}
		}
	}
	for(y -= offy; y < 22; y++) {
		for(int32_t x = 0; x < 10; x++) {
			mino->field[y][x] = 0;
		}
	}

	// sounds even when nothing falls.
	aplay(linefall);
}

static void mino_tryhold(struct player *player, int initrot) {
	struct mino * const mino = &player->mino;
	// you can't hold twice in a row
	if(mino->holded) {
		aplay(holdfail);
		return;
	}

	mino->holded = 1;

	mino_hold(mino);
	aplay(hold);

	mino_spawned(player, initrot);
}

static void mino_rock(struct player *player) {
	struct mino *mino = &player->mino;
	assert(0 <= player->rocks);
	int32_t nlines = mino->rocksperturn < player->rocks ? mino->rocksperturn : player->rocks;

	// copy up
	for(int32_t y = 21; nlines <= y; y--) {
		for(int32_t x = 0; x < 10; x++) {
			mino->field[y][x] = mino->field[y - nlines][x];
		}
	}

	// TODO implement other patterns? (eg. random ~4 lines)

	// copy and fill first rock line
	{
		int32_t nempties = 0;
		for(int32_t x = 0; x < 10; x++) {
			if(mino->field[nlines][x] == 0) {
				nempties++;
			}
		}

		if(1 < nempties) {
			int32_t hole = rand() % nempties;

			// fill except one random x that is empty
			for(int32_t x = 0; x < 10; x++) {
				if(mino->field[nlines][x] != 0 || hole-- != 0) {
					mino->field[nlines - 1][x] = 8;
				}
			}

			nlines--;
		}
	}

	// fill with that pattern (but rock color)
	for(int32_t y = nlines; 0 < y; y--) {
		for(int32_t x = 0; x < 10; x++) {
			mino->field[y - 1][x] = mino->field[y][x] ? 8 : 0;
		}
	}

	mino->phase = MINO_AREROCKED;
	mino->phaset = 0;

	rock_update(player, -nlines);

	aplay(garbage);
}

static void turu_pairxy(int32_t *px, int32_t *py, int rot) {
	switch(rot) {
	case 0: (*py)++; return;
	case 1: (*px)++; return;
	case 2: (*py)--; return;
	case 3: (*px)--; return;
	}
}

static int32_t turu_spider(struct turu *turu, int32_t x, int32_t y, int32_t c) {
	if(x < 0 || 5 < x || y < 0 || 12 < y || turu->floating[y][x] != c) {
		return 0;
	}
	turu->floating[y][x] = (0 < c) ? -c : 0; // mark or clean before call children
	return 1 +
		turu_spider(turu, x, y - 1, c) +
		turu_spider(turu, x + 1, y, c) +
		turu_spider(turu, x, y + 1, c) +
		turu_spider(turu, x - 1, y, c);
}

// TODO seems optimizable about not changing x.
static void turu_simfall(struct turu *turu, int32_t x, int32_t y, int32_t c, struct xyoffset *xyo) {
	// don't overrun on y=12 rot=0
	if(12 < y) y = 12;

	for(; 0 < y; y--) {
		if(turu->floating[y - 1][x]) {
			break;
		}
	}
	turu->floating[y][x] = c;
	xyo->x = x;
	xyo->y = y;
}

// turu->floating must be prepared
static int32_t turu_mark_erasable(struct turu *turu, int32_t (*colors)[7], int32_t *bonus) {
	int32_t total = 0;

	// find erasable and left
	// TODO how I can separate uniqcolor/bonus from here without bothering with erase groups?
	for(int32_t y = 0; y < 13; y++) {
		for(int32_t x = 0; x < 6; x++) {
			int32_t f = turu->floating[y][x];
			if(f <= 0) { // skip if already-marked or empty.
				continue;
			}
			if(f == 7) { // remove rocks from erasable.
				turu->floating[y][x] = 0;
				continue;
			}
			int32_t count = turu_spider(turu, x, y, f);
			if(count < 4) {
				// remove from erasable.
				turu_spider(turu, x, y, -f);
			} else {
				// remember unique colors (if should we)
				if(colors) {
					// must breaks
					for(int32_t i = 0; i < 7; i++) {
						if((*colors)[i] == 0) {
							(*colors)[i] = f;
							break;
						}
						if((*colors)[i] == f) {
							break;
						}
					}
				}
				// 0,2,3,4,5,6,7,10...
				if(bonus) {
					*bonus += (count == 4) ? 0 : (count < 11) ? (count - 3) : 10;
				}
				total += count;
			}
		}
	}

	// marker2floating
	for(int32_t y = 0; y < 13; y++) {
		for(int32_t x = 0; x < 6; x++) {
			turu->floating[y][x] *= -1;
		}
	}

	return total;
}

static void turu_simerase(struct turu *turu) {
	struct xyoffset simpos[2]; // XXX dirty...

	memcpy(turu->floating, turu->field, sizeof(turu->field));

	// simulate current pieces falling
	int32_t x = turu->x, y = turu->y;
	if(turu->rotate != 2) {
		turu_simfall(turu, x, y, turu->piece[0], &simpos[0]);
		turu_pairxy(&x, &y, turu->rotate);
		turu_simfall(turu, x, y, turu->piece[1], &simpos[1]);
	} else {
		turu_pairxy(&x, &y, turu->rotate);
		turu_simfall(turu, x, y, turu->piece[1], &simpos[1]);
		turu_simfall(turu, turu->x, turu->y, turu->piece[0], &simpos[0]);
	}

	turu_mark_erasable(turu, 0, 0);

	// remove simulated pieces
	turu->floating[simpos[0].y][simpos[0].x] = 0;
	turu->floating[simpos[1].y][simpos[1].x] = 0;
}

static void turu_gennext(struct turu *turu) {
	turu->next[0] = 1 + rand() % turu->ncolors;
	turu->next[1] = 1 + rand() % turu->ncolors;
}

static void turu_spawn(struct turu *turu) {
	turu->piece[0] = turu->next[0];
	turu->piece[1] = turu->next[1];
	turu->x = 2;
	turu->y = 11;
	turu->rotate = 0;
	turu->wantrotate = 0;
	turu->cdx = 0;
	turu->cdy = 0;
	turu->pdx = 0;
	turu->pdy = 0;
	turu->phase = TURU_PLACING;
	turu->phaset = 60;
	turu->combo = 0;

	turu_gennext(turu);

	turu_simerase(turu);
}

static int32_t turu_erase(struct player *player) {
	struct turu * const turu = &player->turu;
	int32_t colors[7] = {0};
	int32_t bonus = 0;
	int32_t total;
	int32_t minx = 6, miny = 13, maxx = 0, maxy = 0;

	memcpy(turu->floating, turu->field, sizeof(turu->field));

	total = turu_mark_erasable(turu, &colors, &bonus);

	// additional mark for rocks that contacts to erase-turus
	for(int32_t y = 0; y < 13; y++) {
		for(int32_t x = 0; x < 6; x++) {
			if(turu->floating[y][x]) {
				minx = x < minx ? x : minx;
				miny = y < miny ? y : miny;
				maxx = maxx < x ? x : maxx;
				maxy = maxy < y ? y : maxy;
			}
			if(turu->field[y][x] != 7) { continue; }
			// XXX: this condition is ugly.
			if((y < 12 && 0 < turu->floating[y + 1][x] && turu->floating[y + 1][x] < 7) ||
					(x < 5 && 0 < turu->floating[y][x + 1] && turu->floating[y][x + 1] < 7) ||
					(0 < y && 0 < turu->floating[y - 1][x] && turu->floating[y - 1][x] < 7) ||
					(0 < x && 0 < turu->floating[y][x - 1] && turu->floating[y][x - 1] < 7)) {
				turu->floating[y][x] = 7;
				continue;
			}
		}
	}

	// this must breaks.
	for(int32_t i = 0; i < sizeof(colors); i++) {
		if(colors[i] == 0) {
			// 0,3,6,12,24,48
			bonus += (i < 2) ? 0 : (3 << (i - 2));
			break;
		}
	}

	if(total) {
		turu->combo++;
		// 0,8,16,32,64,96,128,...
		bonus += (turu->combo == 1) ? 0 : (turu->combo < 5) ? (8 << (turu->combo - 2)) : (64 + 32 * (turu->combo - 5));

		if(bonus == 0) {
			bonus = 1;
		}

		add_score(player, (total * 10) * bonus);

		// TODO how many nrocks...
		int32_t x, y;
		rock_pid2xy(player->id, &x, &y);
		rock_gen(player, x - (14*10/2 + minx * 23 + (maxx - minx + 1) * 23 / 2) * 0x10000, y + (11 * 23 - miny * 23 - (maxy - miny + 1) * 23 / 2) * 0x10000, (total * 10) * bonus / 70);

		if(turu->combo == 1) {
			aplay(point1);
		} else if(turu->combo == 2) {
			aplay(point2);
		} else if(turu->combo == 3) {
			aplay(point3);
		} else {
			aplay(point4);
		}
	}

	return total;
}

static void turu_falltest(struct player *player);

static void turu_bounced(struct player *player) {
	struct turu * const turu = &player->turu;
	// fix all floatings
	for(int32_t y = 0; y < 13; y++) {
		for(int32_t x = 0; x < 6; x++) {
			int32_t f = turu->floating[y][x];
			if(f) {
				if(turu->field[y][x]) {
					// overwrapping!?
					fatalhandler();
				}
				turu->field[y][x] = -f; // bouncing floats are negative. lets make positive.
				turu->floating[y][x] = 0;
			}
		}
	}

	// try erasing
	if(turu_erase(player)) {
		turu->phase = TURU_ERASING;
		turu->phaset = turu->erasewait;
		return;
	}

	for(int32_t i = 0; i < ARRAY_SIZE(gstate.players); i++) {
		gstate.players[i].rockingfrom &= ~(1 << player->id);
	}

	// gameover?
	if(turu->field[11][2]) {
		turu->phase = TURU_GAMEOVER;
		turu->phaset = 0;
		for(int32_t i = 0; i < ARRAY_SIZE(turu->goys); i++) {
			turu->goys[i] = -(rand() % (0x10000 * 23));
		}
		aplay(brmpshrr);
		return;
	}

	if(turu->combo == 0 && turu->phase == TURU_BOUNCING && 0 < player->rocks && player->rockingfrom == 0) {
		int32_t nrocks = turu->rocksperturn < player->rocks ? turu->rocksperturn : player->rocks;
		rock_update(player, -nrocks);
		turu->hiddenrocks = nrocks;
		turu->phase = TURU_ROCKFALLING;
		turu->phaset = 0;
		turu->falldy = 0;
		turu_falltest(player);
		aplay(rakka);
		return;
	}

	// TODO allclear? 2100pts(/70rate=6*5 rocks) on next erase

	turu_spawn(&player->turu);
}

static void turu_falltest(struct player *player) {
	struct turu * const turu = &player->turu;
	int floating = 0, grounded = 0, falling = 0;

	if(turu->phase == TURU_ROCKFALLING && 0 < turu->hiddenrocks) {
		static const int32_t xs[6] = {0, 3, 2, 5, 1, 4};
		int32_t nrocks = 6 < turu->hiddenrocks ? 6 : turu->hiddenrocks;
		turu->hiddenrocks -= nrocks;
		int32_t xi = turu->rocki;
		for(int32_t i = 0; i < nrocks; i++, xi = (xi + 1) % 6) {
			int32_t x = xs[xi];
			if(turu->field[12][x] || turu->floating[12][x]) {
				continue;
			}
			turu->floating[12][x] = 7;
		}
		if(nrocks < 6/*really? or turu->hiddenrocks == 0?*/) {
			turu->rocki = (turu->rocki + 1) % 6;
		}
	}

	for(int32_t y = 0; y < 13; y++) {
		for(int32_t x = 0; x < 6; x++) {
			int32_t f = turu->floating[y][x];
			if(f) {
				if(turu->field[y][x]) {
					// overwrapping!?
					fatalhandler();
				}
				if(y < 1 || turu->field[y - 1][x] || turu->floating[y - 1][x] < 0) {
					if(0 < f) {
						// make just bounce
						turu->floating[y][x] = -f;
						grounded = 1;
					}
				} else {
					turu->floating[y - 1][x] = f;
					turu->floating[y][x] = 0;
					falling = 1;
				}
				floating = 1;
			}
		}
	}

	if(grounded) {
		aplay(ssshk1_r);
	}

	if(floating) {
		if(falling) {
			turu->phaset += 60;
		} else if(turu->phase == TURU_FALLING) {
			// all floating are fixed: just bouncing time.
			turu->phase = TURU_BOUNCING;
			turu->phaset = turu->groundwait;
		} else {
			// all floating are fixed: cooling off.
			turu->phase = TURU_ROCKSETTLING;
			turu->phaset = turu->groundwait;
		}
	} else {
		// no floating turus. skip bouncing
		turu_bounced(player);
	}
}

static void turu_fallstart(struct player *player) {
	struct turu *turu = &player->turu;
	turu->phase = TURU_FALLING;
	turu->phaset = 0;
	turu->falldy = 0;
	turu_falltest(player);
}

static void turu_commiterase(struct player *player) {
	struct turu *turu = &player->turu;
	assert(turu->phase == TURU_ERASING);

	for(int32_t y = 0; y < 13; y++) {
		for(int32_t x = 0; x < 6; x++) {
			// erase field by marker,
			if(turu->floating[y][x]) {
				if(!turu->field[y][x]) {
					fatalhandler();
				}
				turu->field[y][x] = 0;
				turu->floating[y][x] = 0;
			}
			// then make fallable floating
			if(0 < y && turu->field[y][x] && !turu->field[y - 1][x]) {
				turu->floating[y][x] = turu->field[y][x];
				turu->field[y][x] = 0;
			}
		}
	}

	turu_fallstart(player);
}

static void turu_fix(struct player *player) {
	struct turu *turu = &player->turu;
	int32_t x = turu->x, y = turu->y;
	bzero(turu->floating, sizeof(turu->floating)); // removing simulates
	turu->floating[y][x] = turu->piece[0];
	turu_pairxy(&x, &y, turu->rotate);
	turu->floating[y][x] = turu->piece[1];
	turu_fallstart(player);
}

static int turu_placable(struct turu *turu, int32_t x, int32_t y, int32_t rot) {
	// core turu test
	if(x < 0 || 5 < x || y < 0 || 12 < y) {
		return 0;
	}
	if(turu->field[y][x] != 0) { return 0; }

	// pair turu test
	int32_t xx = x, yy = y;
	turu_pairxy(&xx, &yy, rot);
	if(xx < 0 || 5 < xx) { return 0; }
	if(yy < 0) { return 0; }
	if(yy < 13 && turu->field[yy][xx] != 0) { return 0; }

	return 1;
}

static void turu_harddrop(struct player *player) {
	struct turu *turu = &player->turu;
	for(int32_t y = turu->y - 1; 0 <= y; y--) {
		if(!turu_placable(turu, turu->x, y, turu->rotate)) {
			break;
		}
		turu->y = y;
	}
	turu_fix(player);
}

static void turu_maneuvered(struct turu *turu) {
	// reset fix time on continue-grounded TODO: -5?
	if(turu->phaset < -5) {
		turu->phaset = -5;
	}
	// TODO forbid eternaloop

	turu_simerase(turu);
}

// TODO this may be replaced with triangular function...
// TODO should consider concurrent slide+rot
static void turu_dxdy(struct turu *turu, int32_t csx, int32_t csy, int32_t psx, int32_t psy, int32_t dt) {
	turu->csx = csx;
	turu->csy = csy;
	turu->cdx = -SGN(csx) * dt;
	turu->cdy = -SGN(csy) * dt;
	turu->psx = psx;
	turu->psy = psy;
	turu->pdx = -SGN(psx) * dt;
	turu->pdy = -SGN(psy) * dt;
}

// TODO simplify
static void turu_rotate(struct turu *turu, int32_t rot) {
	switch(turu->rotate) {
	case 0:
		if(0 < rot) {
			if(turu_placable(turu, turu->x, turu->y, 1)) {
				// normal
				turu_dxdy(turu, 0, 0, -60, -60, 10);
				break;
			} else if(turu->y < 12 && turu_placable(turu, turu->x, turu->y + 1, 1)) {
				// climb-up
				turu->y++;
				turu_dxdy(turu, 0, 60 - turu->phaset, -60, -60, 10);
				turu->phaset = -60;
				// TODO forbid eternaloop
				break;
			} else if(turu_placable(turu, turu->x - 1, turu->y, 1)) {
				// kick-left
				turu->x--;
				turu_dxdy(turu, 60, 0, 0, -60, 10);
				break;
			}
		} else {
			if(turu_placable(turu, turu->x, turu->y, 3)) {
				// normal
				turu_dxdy(turu, 0, 0, 60, -60, 10);
				break;
			} else if(turu->y < 12 && turu_placable(turu, turu->x, turu->y + 1, 3)) {
				// climb-up
				turu->y++;
				turu_dxdy(turu, 0, 60 - turu->phaset, 60, -60, 10);
				turu->phaset = -60;
				// TODO forbid eternaloop
				break;
			} else if(turu_placable(turu, turu->x + 1, turu->y, 3)) {
				// kick-right
				turu->x++;
				turu_dxdy(turu, -60, 0, 0, -60, 10);
				break;
			}
		}
		if(turu->wantrotate == rot && turu_placable(turu, turu->x, turu->y, 2)) {
			// normal flip
			turu_dxdy(turu, 0, 0, 0, -120, 20);
		} else if(turu->wantrotate == rot && turu_placable(turu, turu->x, turu->y + 1, 2)) {
			// kick-up flip
			turu->y++;
			turu_dxdy(turu, 0, 60, 0, -60, 10);
			turu->phaset = -60;
			// TODO forbid eternaloop
		} else {
			turu->wantrotate = rot;
			return;
		}
		break;
	case 1:
		if(0 < rot) {
			if(turu_placable(turu, turu->x, turu->y, 2)) {
				// normal
				turu_dxdy(turu, 0, 0, 60, -60, 10);
			} else if(turu->y < 12 && turu_placable(turu, turu->x, turu->y + 1, 2)) {
				// kick-up
				turu->y++;
				turu_dxdy(turu, 0, 60 - turu->phaset, 60, -60, 10);
				turu->phaset = -60;
				// TODO forbid eternaloop
			} else {
				return;
			}
		} else {
			if(turu_placable(turu, turu->x, turu->y, 0)) {
				// normal
				turu_dxdy(turu, 0, 0, 60, 60, 10);
			} else {
				// I think rot=1->0 must be always succeed.
				fatalhandler();
			}
		}
		break;
	case 2:
		if(0 < rot) {
			if(turu_placable(turu, turu->x, turu->y, 3)) {
				// normal
				turu_dxdy(turu, 0, 0, 60, 60, 10);
				break;
			} else if(turu->y < 12 && turu_placable(turu, turu->x, turu->y + 1, 3)) {
				// snap-up
				turu->y++;
				turu_dxdy(turu, 0, 60 - turu->phaset, 60, 60, 10);
				turu->phaset = -60;
				// TODO forbid eternaloop
				break;
			} else if(turu_placable(turu, turu->x + 1, turu->y, 3)) {
				// kick-right
				turu->x++;
				turu_dxdy(turu, -60, 0, 0, 60, 10);
				break;
			}
		} else {
			if(turu_placable(turu, turu->x, turu->y, 1)) {
				// normal
				turu_dxdy(turu, 0, 0, -60, 60, 10);
				break;
			} else if(turu->y < 12 && turu_placable(turu, turu->x, turu->y + 1, 1)) {
				// snap-up
				turu->y++;
				turu_dxdy(turu, 0, 60 - turu->phaset, -60, 60, 10);
				turu->phaset = -60;
				// TODO forbid eternaloop
				break;
			} else if(turu_placable(turu, turu->x - 1, turu->y, 1)) {
				// kick-left
				turu->x--;
				turu_dxdy(turu, 60, 0, -60, 60, 10);
				break;
			}
		}
		if(turu->wantrotate == rot) {
			if(turu_placable(turu, turu->x, turu->y, 0)) {
				// normal flip
				turu_dxdy(turu, 0, 0, 0, 120, 20);
			} else {
				// I think rot=2->0 must be always succeed.
				fatalhandler();
			}
		} else {
			turu->wantrotate = rot;
			return;
		}
		break;
	case 3:
		if(0 < rot) {
			if(turu_placable(turu, turu->x, turu->y, 0)) {
				// normal
				turu_dxdy(turu, 0, 0, -60, 60, 10);
			} else {
				// I think rot=3->0 must be always succeed.
				fatalhandler();
			}
		} else {
			if(turu_placable(turu, turu->x, turu->y, 2)) {
				// normal
				turu_dxdy(turu, 0, 0, -60, -60, 10);
			} else if(turu->y < 12 && turu_placable(turu, turu->x, turu->y + 1, 2)) {
				// kick-up
				turu->y++;
				turu_dxdy(turu, 0, 60 - turu->phaset, -60, -60, 10);
				turu->phaset = -60;
				// TODO forbid eternaloop
			}
		}
		break;
	}

	// succeed_commons
	turu->rotate = (turu->rotate + turu->wantrotate + rot) & 3;
	turu->wantrotate = 0;
	turu_maneuvered(turu);

	aplay(rotate);

	return;
}

static void turu_slide(struct turu *turu, int32_t dir) {
	int32_t x = turu->x + dir;
	if(!turu_placable(turu, x, turu->y, turu->rotate)) {
		return;
	}

	turu->x = x;
	turu_dxdy(turu, -dir*60, 0, -dir*60, 0, 20);

	aplay(move);

	turu_maneuvered(turu);
}

static void turu_fallplace(struct player *player, int softdrop) {
	struct turu *turu = &player->turu;
	turu->phaset -= turu->fallspeed * (softdrop ? 15 : 1);
	if(turu->phaset <= 0) {
		if(turu_placable(turu, turu->x, turu->y - 1, turu->rotate)) {
			turu->phaset += 60;
			turu->y--;
			if(softdrop) {
				add_score(player, 1);
			}
		} else if(turu->phaset < -30) { // extra wait for fixing
			turu_fix(player);
		}
	}
}

static uint32_t turu_concode(struct turu *turu, int32_t x, int32_t y) {
	uint32_t code = 0;
	int8_t f = turu->field[y][x];
	if(f == 7) return 0; // rock is always alone
	if(y < 13 && turu->field[y + 1][x] == f) { code |= 1; }
	if(x <  6 && turu->field[y][x + 1] == f) { code |= 2; }
	if(0 <  y && turu->field[y - 1][x] == f) { code |= 4; }
	if(0 <  x && turu->field[y][x - 1] == f) { code |= 8; }
	return code;
}

static void player_default_config(struct player *player) {
	player->das = 12;
	player->repeatwait = 1;

	player->gamestate = PLAYER_CONFIG;

	player->confitem = 1; // game selection
	player->gametype = GAMETYPE_MINO;

	player->mino.fallspeed = 1;
	player->mino.softdropmul = 8;
	player->mino.groundwait = 60;
	player->mino.are = 40;
	player->mino.erasewait = 40;
	player->mino.rocksperturn = 7;
	player->mino.initrocks = 0;

	player->turu.ncolors = 4;
	player->turu.fallspeed = 1;
	player->turu.groundwait = 30;
	player->turu.erasewait = 60;
	player->turu.rocksperturn = 6*5;
	player->turu.initrocks = 0;
}

static void mino_init(struct player *player, int gameover) {
	struct mino * const mino = &player->mino;

	player->rocks = mino->initrocks;

	bzero(mino->field, sizeof(mino->field));
	// put S,Z,O into last4 to not to spawn first
	mino->last4[0] = 3; // O
	mino->last4[1] = 6; // Z
	mino->last4[2] = 4; // S
	mino->last4[3] = 6; // Z
	mino->hold = -1;
	mino->holded = 0;
	mino->combo = 0;

	mino->phase = MINO_READY;
	mino->phaset = 90;

	if(!gameover) {
		mino_gennext(mino);
	} else {
		mino->next = -1;
	}
}

static void turu_init(struct player *player, int gameover) {
	struct turu * const turu = &player->turu;

	player->rocks = turu->initrocks;

	bzero(turu->field, sizeof(turu->field));
	bzero(turu->floating, sizeof(turu->floating));
	turu->combo = 0;
	turu->phase = TURU_READY;
	turu->phaset = 90;
	turu_dxdy(turu, 0, 0, 0, 0, 0);
	turu->hiddenrocks = 0; // unnecessary?
	turu->rocki = 0;

	turu->colors[0] = 0x990000FF;
	turu->colors[1] = 0x999900FF;
	turu->colors[2] = 0x009900FF;
	turu->colors[3] = 0x009999FF;
	turu->colors[4] = 0x000099FF;
	turu->colors[5] = 0x990099FF;
	turu->colors[6] = 0x808080FF;
	turu->colors[7] = 0x404040FF;
	for(int32_t i = 0; i < 5; i++) {
		int32_t j = i + rand() % (6 - i);
		uint32_t tmp = turu->colors[j];
		turu->colors[j] = turu->colors[i];
		turu->colors[i] = tmp;
	}

	if(!gameover) {
		turu_gennext(turu);
	} else {
		turu->next[0] = 0; // empty
	}
}

static void player_init(struct player *player, int gameover) {
	player->score = 0;
	player->gamestate = gameover ? PLAYER_CONFIG : PLAYER_INGAME;

	player->repeatt = player->das;

	player->pendrocks = 0;
	player->rockingfrom = 0;
	player->rockstate = ROCK_SOLID;
	player->rockt = 0; // unnecessary

	switch(player->gametype) {
	case GAMETYPE_MINO:
		mino_init(player, gameover);
		break;
	case GAMETYPE_TURU:
		turu_init(player, gameover);
		break;
	}

	if(!gameover && player->leader) {
		aplay(ready);
	}
}

static void input(void) {
	si_refresh_inputs();

	for(int32_t ctrli = 0; ctrli < 4; ctrli++) {
		int32_t initrot = 0, inithold = 0;
		int softdrop = 0;

		struct player *player = &gstate.players[ctrli];
		struct mino * const mino = &player->mino;
		struct turu * const turu = &player->turu;

		if(controller_inputs[ctrli] != -1) {
			// connected, good response
			uint32_t controller_input = controller_inputs[ctrli];
			uint16_t pressbuttons = controller_input >> 16;
			int8_t x = controller_input >> 8;
			int8_t y = controller_input;

			randstir(controller_input);

			// 3dstick to hat
			pressbuttons |= (40 < y) << 11;
			pressbuttons |= (y < -40) << 10;
			pressbuttons |= (x < -40) << 9;
			pressbuttons |= (40 < x) << 8;

			uint16_t downbuttons = (prebuttons[ctrli] ^ pressbuttons) & pressbuttons;
			prebuttons[ctrli] = pressbuttons;

			int32_t filterlr = 1;

			if((pressbuttons & (SI_BUTTON_HATL | SI_BUTTON_HATR)) == 0) {
				player->repeatt = player->das;
			} else {
				if(downbuttons & (SI_BUTTON_HATL | SI_BUTTON_HATR)) {
					filterlr = 0;
				}
				--player->repeatt;
				if(player->repeatt <= 0) {
					filterlr = 0;
					player->repeatt = player->repeatwait; // repeat rewind
				}
			}

			if(pressbuttons & SI_BUTTON_B) {
				initrot = -1;
			}
			if(pressbuttons & SI_BUTTON_A) {
				initrot = 1;
			}
			if(pressbuttons & (SI_BUTTON_L | SI_BUTTON_R | SI_BUTTON_Z)) {
				inithold = 1;
			}

			switch(player->gamestate) {
			case PLAYER_INGAME:
				if(player->gametype == GAMETYPE_MINO && (mino->phase == MINO_PLACING || mino->phase == MINO_GROUNDING)) {
					if(downbuttons & SI_BUTTON_HATU) {
						mino_harddrop(player);
					}
					if(pressbuttons & SI_BUTTON_HATD) {
						softdrop = 1;
					}

					// first, rotation.
					if(downbuttons & SI_BUTTON_B) {
						mino_rotate(mino, -1);
					}
					if(downbuttons & SI_BUTTON_A) {
						mino_rotate(mino, 1);
					}

					// then, move.
					if(pressbuttons & SI_BUTTON_HATL) {
						if(!filterlr) {
							mino_slide(mino, -1);
						}
					}
					if(pressbuttons & SI_BUTTON_HATR) {
						if(!filterlr) {
							mino_slide(mino, 1);
						}
					}

					// finally, falls (later).

					if(downbuttons & (SI_BUTTON_L | SI_BUTTON_R | SI_BUTTON_Z)) {
						mino_tryhold(player, initrot);
					}
				} else if(player->gametype == GAMETYPE_TURU && turu->phase == TURU_PLACING) {
					if(downbuttons & SI_BUTTON_HATU) {
						turu_harddrop(player);
					}
					if(pressbuttons & SI_BUTTON_HATD) {
						softdrop = 1;
					}

					// first, rotation?
					if(downbuttons & SI_BUTTON_B) {
						turu_rotate(turu, -1);
					}
					if(downbuttons & SI_BUTTON_A) {
						turu_rotate(turu, 1);
					}

					// then, move?
					if(pressbuttons & SI_BUTTON_HATL) {
						if(!filterlr) {
							turu_slide(turu, -1);
						}
					}
					if(pressbuttons & SI_BUTTON_HATR) {
						if(!filterlr) {
							turu_slide(turu, 1);
						}
					}

					// finally, falls (later).

					if(downbuttons & (SI_BUTTON_L | SI_BUTTON_R | SI_BUTTON_Z)) {
						// hold?
						//turu_hold(turu);
					}
				}
				if(downbuttons & SI_BUTTON_START) {
					player->gamestate = PLAYER_PAUSE;
					aplay(pageenter2);
				}
				break;
			case PLAYER_CONFIG:
				{
					const int32_t items = 10;
					if(downbuttons & SI_BUTTON_HATU) {
						player->confitem = (player->confitem + items - 1) % items;
						aplay(tamaf);
					}
					if(downbuttons & SI_BUTTON_HATD) {
						player->confitem = (player->confitem + 1) % items;
						aplay(tamaf);
					}
					int delta = 0;
					if((pressbuttons & SI_BUTTON_HATL) && !filterlr) {
						delta = -1;
					}
					if((pressbuttons & SI_BUTTON_HATR) && !filterlr) {
						delta = 1;
					}
					if(pressbuttons & (SI_BUTTON_R | SI_BUTTON_Z)) {
						delta *= 60;
					}
					if(delta != 0) {
						int32_t pv=0, v=0;
						if(player->gametype == GAMETYPE_MINO) {
							switch(player->confitem) {
							case 1: pv=player->gametype;v=player->gametype = GAMETYPE_TURU; player_init(player, 1); break;
							case 2: pv=player->das;v=player->das = CLAMP(4, player->das + delta, 30); break;
							case 3: pv=player->repeatwait;v=player->repeatwait = CLAMP(1, player->repeatwait + delta, 10); break;
							case 4: pv=player->mino.fallspeed;v=player->mino.fallspeed = CLAMP(1, player->mino.fallspeed + delta, 60 * 20); break;
							case 5: pv=player->mino.groundwait;v=player->mino.groundwait = CLAMP(1, player->mino.groundwait + delta, 60); break;
							case 6: pv=player->mino.erasewait;v=player->mino.erasewait = CLAMP(1, player->mino.erasewait + delta, 60); break;
							case 7: pv=player->mino.rocksperturn;v=player->mino.rocksperturn = CLAMP(1, player->mino.rocksperturn + delta, 20); break;
							case 8: pv=player->mino.initrocks;v=player->rocks=player->mino.initrocks = CLAMP(0, player->mino.initrocks + delta, 10*20); break;
							case 9: pv=player->mino.are;v=player->mino.are = CLAMP(1, player->mino.are + delta, 60); break;
							}
						} else { // GAMETYPE_TURU
							switch(player->confitem) {
							case 1: pv=player->gametype;v=player->gametype = GAMETYPE_MINO; player_init(player, 1); break;
							case 2: pv=player->das;v=player->das = CLAMP(4, player->das + delta, 30); break;
							case 3: pv=player->repeatwait;v=player->repeatwait = CLAMP(1, player->repeatwait + delta, 10); break;
							case 4: pv=player->turu.fallspeed;v=player->turu.fallspeed = CLAMP(1, player->turu.fallspeed + delta, 60); break;
							case 5: pv=player->turu.groundwait;v=player->turu.groundwait = CLAMP(1, player->turu.groundwait + delta, 60); break;
							case 6: pv=player->turu.erasewait;v=player->turu.erasewait = CLAMP(1, player->turu.erasewait + delta, 60); break;
							case 7: pv=player->turu.rocksperturn;v=player->turu.rocksperturn = CLAMP(1, player->turu.rocksperturn + delta, 72); break;
							case 8: pv=player->turu.initrocks;v=player->rocks=player->turu.initrocks = CLAMP(0, player->turu.initrocks + delta, 6*12*10); break;
							case 9: pv=player->turu.ncolors;v=player->turu.ncolors = CLAMP(3, player->turu.ncolors + delta, 6); break;
							}
						}
						if(v != pv) {
							aplay(tamaf);
						}
					}
					if(downbuttons & SI_BUTTON_L) {
						gstate.bgtex ^= 1;
					}
					if(downbuttons & SI_BUTTON_CU) {
						gstate.gamedraw ^= 1;
					}
#ifdef DEBUG
					if(downbuttons & SI_BUTTON_CL) {
						extern void stub_install(void); extern void stub(void); stub_install(); stub();
					}
#endif
					if(((downbuttons & SI_BUTTON_A) && player->confitem == 0) || (downbuttons & SI_BUTTON_START)) {
						player->confitem = 0;
						player->gamestate = PLAYER_READY;
						player->gamestatet = 0;
						aplay(shakin);
					}
				}
				break;
			case PLAYER_READY:
				if(downbuttons & SI_BUTTON_B) {
					player->gamestate = PLAYER_CONFIG;
					aplay(tamaido);
				}
				break;
			// TODO: move PLAYER_INGAME here
			case PLAYER_PAUSE:
				if(downbuttons & SI_BUTTON_START) {
					if(!(pressbuttons & SI_BUTTON_Z)) {
						player->gamestate = PLAYER_INGAME;
						aplay(pageexit2);
					} else {
						// surrender
						player_init(player, 1);
					}
				}
				break;
			case PLAYER_GAMEOVER:
				if(downbuttons) {
					player_init(player, 1);
				}
				break;
			}
		}

		switch(player->gamestate) {
		case PLAYER_CONFIG:
			/*nothing to do*/
			break;
		case PLAYER_READY:
			player->gamestatet++;
			if(player->gamestatet == 50) {
				struct player *p = player;
				for(p = player->next; p != player; p = p->next) {
					if(controller_inputs[p->id] != -1 && p->gamestate == PLAYER_CONFIG) { // XXX: controller_inputs should be avoided
						break;
					}
				}
				if(p == player) {
					// all player is not in config... I am last configurator.
					player->leader = 1;
					player_init(player, 0);
					for(p = player->next; p != player; p = p->next) {
						if(controller_inputs[p->id] == -1 || p->gamestate != PLAYER_READY) { // XXX: controller_inputs should be avoided
							continue;
						}
						p->leader = 0;
						player_init(p, 0);
					}
				}
			}
			break;
		case PLAYER_INGAME:
			if(player->gametype == GAMETYPE_MINO) {
				switch(mino->phase) {
				case MINO_READY:
					mino->phaset--;
					if(mino->phaset == 40 && player->leader) {
						aplay(go);
					} else if(mino->phaset == 1) {
						//mino_spawn(player, initrot, inithold);
						// rock first...
						mino->phase = MINO_ARE;
						mino->phaset = mino->are;
					}
					break;
				case MINO_PLACING:
					mino_fall(player, softdrop); // will reset fallt unless grounded
					break;
				case MINO_GROUNDING:
					mino->phaset--;
					if(mino->phaset == 0) {
						mino_freeze(player);
					}
					break;
				case MINO_FLASHING:
					mino->phaset--;
					if(mino->phaset == 0) {
						mino->phase = MINO_ARE;
						mino->phaset = 0;
					}
					break;
				case MINO_ERASING:
					mino->phaset--;
					if(mino->phaset == 0) {
						mino_falllines(mino);
						mino->phase = MINO_ARE;
						mino->phaset = 0;
					}
					break;
				case MINO_ARE:
					mino->phaset++;
					if(mino->are - 5 <= mino->phaset) {
						if(0 < player->rocks && player->rockingfrom == 0 && mino->combo == 0) {
							mino_rock(player);
						} else {
							mino_spawn(player, initrot, inithold);
						}
					}
					break;
				case MINO_AREROCKED:
					mino->phaset++;
					if(mino->are <= mino->phaset) {
						mino_spawn(player, initrot, inithold);
					}
					break;
				case MINO_GAMEOVER:
					mino->phaset++;
					for(int32_t i = 0; i < ARRAY_SIZE(mino->goys); i++) {
						mino->goys[i] += mino->phaset * 0x10000 / 6;
					}
					if(60 < mino->phaset) {
						player->gamestate = PLAYER_GAMEOVER;
						player->gamestatet = 0;
					}
					break;
				}
			} else { // GAMETYPE_TURU
				switch(turu->phase) {
				case TURU_READY:
					turu->phaset--;
					if(turu->phaset == 40 && player->leader) {
						aplay(go);
					} else if(turu->phaset == 1) {
						//turu_spawn(turu);
						// rock first...
						turu->phase = TURU_BOUNCING;
						turu->phaset = 1;
					}
					break;
				case TURU_PLACING:
					turu_fallplace(player, softdrop);
					break;
				case TURU_FALLING: //FALLTHROUGH
				case TURU_ROCKFALLING:
					if(turu->falldy < 60) { // almost true.
						turu->falldy++;
					}
					turu->phaset -= turu->falldy;
					if(turu->phaset <= 0) {
						turu_falltest(player);
					}
					break;
				case TURU_BOUNCING: //FALLTHROUGH
				case TURU_ROCKSETTLING:
					turu->phaset--;
					if(turu->phaset <= 0) {
						turu_bounced(player);
					}
					break;
				case TURU_ERASING:
					turu->phaset--;
					if(turu->phaset <= 0) {
						turu_commiterase(player);
					}
					break;
				case TURU_GAMEOVER:
					turu->phaset++;
					for(int32_t i = 0; i < ARRAY_SIZE(turu->goys); i++) {
						turu->goys[i] += turu->phaset * 0x10000 / 6;
					}
					if(60 < turu->phaset) {
						player->gamestate = PLAYER_GAMEOVER;
						player->gamestatet = 0;
					}
					break;
				}
				if(turu->csx != 0) { int32_t csx = turu->csx + turu->cdx; turu->csx = (SGN(turu->csx) == SGN(csx)) ? csx : 0; }
				if(turu->csy != 0) { int32_t csy = turu->csy + turu->cdy; turu->csy = (SGN(turu->csy) == SGN(csy)) ? csy : 0; }
				if(turu->psx != 0) { int32_t psx = turu->psx + turu->pdx; turu->psx = (SGN(turu->psx) == SGN(psx)) ? psx : 0; }
				if(turu->psy != 0) { int32_t psy = turu->psy + turu->pdy; turu->psy = (SGN(turu->psy) == SGN(psy)) ? psy : 0; }
			}
			break;
		case PLAYER_PAUSE:
			/*nothing to do*/
			break;
		case PLAYER_GAMEOVER:
			player->gamestatet++;
			break;
		}
	}

	rock_step();
}

static void _dpmode(enum DP_SET_OTHER_MODES_VALUES mode, int transp) {
	dp_set_other_modes(
			DP_SOM_NPRIMITIVE,
			mode,
			DP_SOM_NOPERSP,
			DP_SOM_NOLOD,
			DP_SOM_NOTLUT,
			DP_SOM_BILERP,
			DP_SOM_NOCONV,
			DP_SOM_NOKEY,
			DP_SOM_RGBDITHER_NOISE,
			DP_SOM_ALPHADITHER_NONE,
			DP_SOM_BLEND_MA_PIXEL,
			DP_SOM_BLEND_MA_PIXEL,
			DP_SOM_BLEND_M1B_CC,
			DP_SOM_BLEND_M1B_CC,
			DP_SOM_BLEND_MA_MEMORY,
			DP_SOM_BLEND_MA_MEMORY,
			DP_SOM_BLEND_M2B_1MINUS,
			DP_SOM_BLEND_M2B_1MINUS,
			transp ? DP_SOM_FORCEBLEND : DP_SOM_CVGBLEND,
			DP_SOM_CVG,
			DP_SOM_AC_ALPHA,
			DP_SOM_ZMODE_OPAQUE,
			DP_SOM_CVGDEST_ZAP,
			DP_SOM_COLORALWAYS,
			transp ? DP_SOM_IMRD : DP_SOM_NOIMRD,
			DP_SOM_NOZUPDATE,
			DP_SOM_NOZCOMPARE,
			DP_SOM_NOAA,
			DP_SOM_ZSRC_PRIMITIVE,
			DP_SOM_ALPHACOMP_NONE);
}

// mode must be FILL or COPY.
static void dpmode_bulk(enum DP_SET_OTHER_MODES_VALUES mode) {
	_dpmode(mode, 0);
}

static void dpmode_opaque() {
	_dpmode(DP_SOM_1CYC, 0);
}

static void dpmode_transp() {
	_dpmode(DP_SOM_1CYC, 1);
}

// this is preferred way to load texture (faster than dploadtextile)
// this can be used only if texture_image_width == tile_width && tile_width*DP_BIPP(size) % 64 == 0.
static void dploadtexblock(uint32_t tile, enum DP_COLOR_FORMAT fmt, enum DP_PIXEL_SIZE size, uint32_t texpaddr, uint32_t left, uint32_t top, uint32_t width, uint32_t height) {
	assert(width * DP_BIPP(size) % 64 == 0);

	if((size == DP_SIZE_4B || size == DP_SIZE_8B) && 2048 < width * height) {
		// special: RDP2 can load at most 2048texel at once... load as 16bpp to avoid that!!
		width = width * DP_BIPP(size) / DP_BIPP(DP_SIZE_16B);
		size = DP_SIZE_16B;
	}

	assert(width * height <= 2048);

	dp_pipe_sync();
	dp_set_texture_image(fmt, size, width, texpaddr);
	dp_set_tile(tile, fmt, size, 0/*must be 0 for block*/, 0, 0, DP_TILE_CLAMP, 0, 0, DP_TILE_CLAMP, 0, 0);
	dp_load_sync();
	dp_load_block(tile, left, top, width, height, size);
	dp_pipe_sync();
}

static void dploadtexblock1(enum DP_COLOR_FORMAT fmt, enum DP_PIXEL_SIZE size, uint32_t texpaddr, uint32_t left, uint32_t top, uint32_t width, uint32_t height) {
	// using other tile than rendering makes tilesync unnecessary (but requires set_tile and set_tile_size)
	dploadtexblock(7, fmt, size, texpaddr, left, top, width, height);
	dp_set_tile(0, fmt, size, width, 0, 0, DP_TILE_CLAMP, 0, 0, DP_TILE_CLAMP, 0, 0);
	dp_set_tile_size_10(0, 0, 0, width, height);
}

static void draw_clean(void) {
	dp_set_combine_mode2(DP_SCM_COLOR_PRIMITIVE, DP_SCM_ALPHA_PRIMITIVE);
	if(gstate.bgtex) {
		dpmode_bulk(DP_SOM_COPY);

		dp_set_texture_image(DP_FORMAT_RGBA, DP_SIZE_16B, 640, (uint32_t)back2);
		dp_tile_sync();
		// using other tile than rendering makes tilesync unnecessary (but requires set_tile and set_tile_size)
		dp_set_tile(7, DP_FORMAT_RGBA, DP_SIZE_16B, 0, 0, 0, DP_TILE_CLAMP, 0, 0, DP_TILE_CLAMP, 0, 0);
		dp_set_tile(0, DP_FORMAT_RGBA, DP_SIZE_16B, 640, 0, 0, DP_TILE_CLAMP, 0, 0, DP_TILE_CLAMP, 0, 0);
		dp_set_tile_size_10(0, 0, 0, 640, 3);
		int32_t offy = (curfield ^ 1) * 240;
		for(int32_t i = curfield^1; i < 480; i += 6) {
			dp_load_sync();
			dp_load_block(7, 0, offy + i / 2, 640, 3, DP_SWAPPED);
			// +5 not +6 for avoid curfield==1 overrun (this does not effect display thanks to interlaced)
			// I dunno why ds must be 4.0 (not 1.0??)
			dp_texture_rectangle_10_raw(0, i, 640, i + 5, 0, 0, 0, 0x400*4, 0x400/2);
		}
	} else {
		dpmode_bulk(DP_SOM_FILL);
		dp_set_fill_color_16b(0, 0, 0, 1);
		dp_fill_rectangle_10_raw(0, 0, 640, 480);
	}
}

// must be dpmode_transp
static void draw_number(int32_t v, int32_t x, int32_t y, int32_t maxcols, int32_t scale/*s15.16*/) {
	int32_t minus = v < 0;
	if(minus) {
		v = -v;
	}

	dploadtexblock1(NUMTEX_FMT, NUMTEX_SIZE, (uint32_t)numtex, 0, 0, 16, 16*11);

	for(int32_t j = 0; j < maxcols; j++) {
		dp_texture_rectangle_10_raw(x, y, x + 12 * scale / 0x10000, y + 16 * scale / 0x10000, 0, 0, 0x20*16*(v%10), 0x400*0x10000/scale, 0x400*0x10000/scale);

		v /= 10;
		x -= 12 * scale / 0x10000;
		if(v == 0) {
			if(minus) {
				dp_texture_rectangle_10_raw(x, y, x + 12 * scale / 0x10000, y + 16 * scale / 0x10000, 0, 0, 0x20*16*10, 0x400*0x10000/scale, 0x400*0x10000/scale);
			}
			return;
		}
	}
}

// draw at most 6 rocks. overflowed rocks are not drawn (but counted internally).
static void draw_rocks(struct player *player, int32_t offx, int32_t offy) {
	// fastpath
	if(player->rocks <= 0) {
		return;
	}

	// blinking
	if((player->gamestate == PLAYER_INGAME) && (player->rockingfrom == 0) && (gstate.t % 10 < 5)) {
		return;
	}

	dploadtexblock1(TURUTEX_FMT, TURUTEX_SIZE, (uint32_t)turutex + (16 * 16*16)*DP_BYPP(TURUTEX_SIZE), 0, 0, 16, 16*3);
	dpmode_transp();
	dp_set_combine_mode2(DP_SCM_COLOR_TEX0xPRIM, DP_SCM_ALPHA_TEX0);

	int32_t wr = player->rockstate == ROCK_SOLID ? 0x10000 : player->rockt;
	int32_t n = player->rocks;
	for(int32_t i = 0; i < 6 && n; i++) {
		int ty, sz, dt;
		if(6*6*6 <= n) {
			n -= 6*6*6;
			ty = 2*16*0x20;
			sz = 16;
			dt = 0x400;
			dp_set_prim_color_rgba(0xC0C020FF, 0, 0);
		} else if(6*6 <= n) {
			n -= 6*6;
			ty = 1*16*0x20;
			sz = 16;
			dt = 0x400;
			dp_set_prim_color_rgba(0xC04020FF, 0, 0);
		} else if(6 <= n) {
			n -= 6;
			ty = 0*16*0x20;
			sz = 16;
			dt = 0x400;
			dp_set_prim_color_rgba(0xFFFFFFFF, 0, 0);
		} else {
			n--;
			ty = 0*16*0x20;
			sz = 10;
			dt = 0x400*16/10;
			dp_set_prim_color_rgba(0xFFFFFFFF, 0, 0);
		}

		int32_t dx = i * 14*10 * wr / 0x10000 / 6 + 14*10 * (0x10000 - wr) * 5 / 0x10000 / 12;
		dp_texture_rectangle_10_raw(offx + dx, offy + (16 - sz), offx + dx + sz, offy + 16, 0, 0, ty, dt, dt);
	}
}

static void draw_config(const struct player *player, int32_t xh, int32_t yh, int32_t bgw, int32_t cy, int32_t v) {
	dp_pipe_sync();
	dp_set_combine_mode8(
			DP_SCM_COLORA_PRIMITIVE,DP_SCM_COLORB_ZERO,DP_SCM_COLORC_TEXEL0,DP_SCM_COLORD_ZERO,
			DP_SCM_ALPHAA_TEX0,DP_SCM_ALPHAB_ZERO,DP_SCM_ALPHAC_PRIMITIVE,DP_SCM_ALPHAD_ZERO);
	int ready_flashy = player->gamestate == PLAYER_READY && (10 <= player->gamestatet || ((player->gamestatet % 4) < 2));

	if(0 < bgw) {
		dploadtexblock1(GUITEX_FMT, GUITEX_SIZE, (uint32_t)guitex, 0, 0, 16, 32);

		dp_set_prim_color_rgba(ready_flashy ? 0xFFFFFFFF: 0x404040C0 , 0, 0);
		dp_texture_rectangle_10_raw(xh, yh, xh + bgw, yh + 32, 0, 0, 0, 0x400, 0x400);
	}

	dp_set_prim_color_rgba(ready_flashy ? 0x000000FF : 0 < bgw ? 0xFFFFFFFF : 0xC0C0C080, 0, 0);

	if(cy == 0) {
		// Ready?/READY!
		int32_t t = player->gamestate == PLAYER_CONFIG ? 0 : 16;
		dploadtexblock1(OCHIMTEX_FMT, OCHIMTEX_SIZE, (uint32_t)ochimtex, 0, t, 80, 16);
		int32_t w = player->gamestate == PLAYER_CONFIG ? 49 : 37;
		int32_t x = xh + (14*10 - w) / 2;
		dp_texture_rectangle_10_raw(x, yh + 8, x + 80, yh + 8 + 16, 0, 0, 0, 0x400, 0x400);
	} else {
		// label
		dploadtexblock1(OCHIMTEX_FMT, OCHIMTEX_SIZE, (uint32_t)ochimtex, 0, 16*5 + 16*cy, 80, 16);
		dp_texture_rectangle_10_raw(xh + 4, yh + 8, xh + 4 + 80, yh + 8 + 16, 0, 0, 0, 0x400, 0x400);

		if(cy == 1) {
			// logo
			dploadtexblock1(GTYPETEX_FMT, GTYPETEX_SIZE, (uint32_t)gtypetex, 0, player->gametype == GAMETYPE_MINO ? 0 : 32, 48, 32);
			int32_t x = xh + 48 + (14*10-4-4-48)/2, y = yh + (player->gametype==GAMETYPE_MINO ? 2 : 0);
			dp_texture_rectangle_10_raw(x, y, x + 48, y + 32, 0, 0, 0, 0x400, 0x400);
		} else {
			draw_number(v, xh + 14*10 - 12 - 4, yh + 8, 4, 0x10000);
		}
	}
}

static void draw_gameover(struct player *player, int32_t foffx, int32_t foffy, int32_t fw) {
	dploadtexblock1(OCHIMTEX_FMT, OCHIMTEX_SIZE, (uint32_t)ochimtex, 0, 16*3, 80, 16);
	dp_set_combine_mode2(DP_SCM_COLOR_TEXEL0, DP_SCM_ALPHA_TEX0);
	int32_t w = 80 * 1.3, h = 16 * 1.3;
	int32_t x = foffx + (fw - w) / 2, y = foffy + 50 + sin(2 * 3.14159 * player->gamestatet / 60) * 10;
	dp_texture_rectangle_10_raw(x, y, x + w, y + h, 0, 0, 0, (int32_t)(0x400 / 1.3), (int32_t)(0x400 / 1.3));
}

static void draw_readygo(int32_t foffx, int32_t foffy, int32_t fw, int ready) {
	dploadtexblock1(OCHIMTEX_FMT, OCHIMTEX_SIZE, (uint32_t)ochimtex, 0, 16*(ready ? 4 : 5), 80, 16);
	dp_set_combine_mode2(DP_SCM_COLOR_TEXEL0, DP_SCM_ALPHA_TEX0);
	int32_t w = (ready ? 64 : 40) * 1.3, h = 16 * 1.3;
	int32_t x = foffx + (fw - w) / 2, y = foffy + 50;
	dp_texture_rectangle_10_raw(x, y, x + w, y + h, 0, 0, 0, (int32_t)(0x400 / 1.3), (int32_t)(0x400 / 1.3));
}

static void draw_mino(int32_t offx, int32_t offy, struct player *player) {
	struct mino * const mino = &player->mino;

	/* guideline color */
	static const uint32_t colors[10] = {
		0x101010FF, // notused
		0x00FFFFFF,
		0x3030FFFF,
		0xFF8000FF,
		0xFFFF00FF,
		0x00FF00FF,
		0xFF00FFFF,
		0xFF0000FF,
		0x404040FF,
		0x202020FF,
	};

	static const uint32_t bsize = 14, hbsize = 8;

	const uint32_t foffx = offx, foffy = offy + 3 * bsize; // field
	const uint32_t noffx = offx + 3 * bsize, noffy = offy; // next
	const uint32_t hoffx = offx, hoffy = offy; // hold

	// field
	dp_pipe_sync();
	dpmode_opaque();
	dp_set_combine_mode2(DP_SCM_COLOR_PRIMITIVE, DP_SCM_ALPHA_PRIMITIVE);
	dp_set_prim_color_rgba(0xFFFFFFFF, 0, 0);
	dp_fillrect(foffx - 4, foffy, foffx, foffy + 20 * bsize);
	dp_fillrect(foffx + 10 * bsize, foffy, foffx + 10 * bsize + 4, foffy + 20 * bsize);
	dp_fillrect(foffx - 4, foffy + 20 * bsize, foffx + 10 * bsize + 4, foffy + 20 * bsize + 4);

	// using other tile than rendering makes tilesync unnecessary (but requires set_tile and set_tile_size)
	dploadtexblock(7, MINOTEX_FMT, MINOTEX_SIZE, (uint32_t)minotex, 0, 0, 16, 64);
	dp_set_tile(0, MINOTEX_FMT, MINOTEX_SIZE, 16, 16*0*DP_BYPP(MINOTEX_SIZE), 0, DP_TILE_REPEAT, 4, 0, DP_TILE_REPEAT, 4, 0);
	dp_set_tile_size_10(0, 0, 0, 16, 16);
	dp_set_tile(1, MINOTEX_FMT, MINOTEX_SIZE, 16, 16*16*DP_BYPP(MINOTEX_SIZE), 0, DP_TILE_CLAMP, 0, 0, DP_TILE_CLAMP, 0, 0);
	dp_set_tile_size_10(1, 0, 0, 16, 16);
	dp_set_tile(2, MINOTEX_FMT, MINOTEX_SIZE, 16, 16*32*DP_BYPP(MINOTEX_SIZE), 0, DP_TILE_CLAMP, 0, 0, DP_TILE_CLAMP, 0, 0);
	dp_set_tile_size_10(2, 0, 0, 16, 16);
	dp_set_tile(3, MINOTEX_FMT, MINOTEX_SIZE, 16, 16*48*DP_BYPP(MINOTEX_SIZE), 0, DP_TILE_CLAMP, 0, 0, DP_TILE_CLAMP, 0, 0);
	dp_set_tile_size_10(3, 0, 0, 16, 16);

	// bg grid
	if(gstate.bgtex) {
		dpmode_transp();
		dp_set_combine_mode2(DP_SCM_COLOR_TEXEL0, DP_SCM_ALPHA_PRIMITIVE);
		dp_set_prim_color_rgba(0xFFFFFFC0, 0, 0);
	} else {
		dp_set_combine_mode2(DP_SCM_COLOR_TEXEL0, DP_SCM_ALPHA_TEX0);
	}
	dp_texture_rectangle_10_raw(foffx, foffy, foffx + 10 * bsize, foffy + 20 * bsize, 0, 0, 0, 0x400*16/14, 0x400*16/14);

	dp_pipe_sync();
	dpmode_opaque();
	dp_set_combine_mode8(
			DP_SCM_COLORA_TEXEL0,DP_SCM_COLORB_PRIMITIVE,DP_SCM_COLORC_TEX0A,DP_SCM_COLORD_PRIMITIVE,
			DP_SCM_ALPHAA_ZERO,DP_SCM_ALPHAB_ZERO,DP_SCM_ALPHAC_ZERO,DP_SCM_ALPHAD_PRIMITIVE);
	if(player->gamestate == PLAYER_INGAME) {
		// blocks
		dp_set_scissor_10(foffx, foffy, foffx + 10 * bsize, foffy + 20 * bsize, curfield + DP_SCISSOR_KEEPEVEN); // TODO frame-skip adaptive frame selection
		for(int32_t y = 0; y < 20; y++) {
			for(int32_t x = 0; x < 10; x++) {
				uint32_t xh = x * bsize + foffx, yh = (19 - y) * bsize + foffy;
				int8_t block = mino->field[y][x];
				if(block == 0) {
					continue;
				}
				if(mino->phase == MINO_GAMEOVER) {
					block = 9;
					yh += 0 < mino->goys[x] ? mino->goys[x] / 0x10000 : 0;
				}
				dp_set_prim_color_rgba(colors[block], 0, 0);
				dp_texture_rectangle_10_raw(xh, yh, xh + bsize, yh + bsize, 1, 0, 0, 0x400*16/14, 0x400*16/14);
			}
		}
		dp_pipe_sync();
		dp_set_scissor_10(0, 0, 640, 480, curfield + DP_SCISSOR_KEEPEVEN); // TODO frame-skip adaptive frame selection
	}

	// falling piece
	if(player->gamestate == PLAYER_INGAME && (mino->phase == MINO_PLACING || mino->phase == MINO_GROUNDING || mino->phase == MINO_FLASHING)) {
		int8_t block = pieces[mino->piece].block;
		uint32_t color;

		uint64_t mul = 0x100;
		if(mino->phase != MINO_FLASHING) {
			if(mino->phase == MINO_GROUNDING) {
				mul = 0x80 + 0x80 * mino->phaset / mino->groundwait;
			}
			uint32_t r = (((colors[block] >> 24) & 0xFF) * mul) >> 8;
			uint32_t g = (((colors[block] >> 16) & 0xFF) * mul) >> 8;
			uint32_t b = (((colors[block] >>  8) & 0xFF) * mul) >> 8;
			color = (r << 24) | (g << 16) | (b << 8) | 0xFF;
		} else {
			color = 0xFFFFFFFF; // flashing
		}

		for(int32_t dy = 0; dy < 4; dy++) {
			if(19 < mino->y + dy) {
				break;
			}
			for(int32_t dx = 0; dx < 4; dx++) {
				int8_t block = pieces[mino->piece].blocks[mino->rotate][dy][dx];
				if(block == 0) {
					continue;
				}
				uint32_t xh = (mino->x + dx) * bsize + foffx, yh = (19 - (mino->y + dy)) * bsize + foffy;
				dp_set_prim_color_rgba(color, 0, 0);
				dp_texture_rectangle_10_raw(xh, yh, xh + bsize, yh + bsize, 1, 0, 0, 0x400*16/14, 0x400*16/14);
			}
		}
	}

	// next piece
	if(0 <= mino->next){
		for(int32_t dy = 0; dy < 4; dy++) {
			for(int32_t dx = 0; dx < 4; dx++) {
				int8_t block = pieces[mino->next].blocks[0][dy][dx];
				if(block == 0) {
					continue;
				}
				uint32_t xh = dx * bsize + noffx, yh = (3 - dy) * bsize + noffy;
				dp_set_prim_color_rgba(colors[block], 0, 0);
				dp_texture_rectangle_10_raw(xh, yh, xh + bsize, yh + bsize, 1, 0, 0, 0x400*16/14, 0x400*16/14);
			}
		}
	}

	// hold piece
	if(0 <= mino->hold){
		for(int32_t dy = 0; dy < 4; dy++) {
			for(int32_t dx = 0; dx < 4; dx++) {
				int8_t block = pieces[mino->hold].blocks[0][dy][dx];
				if(block == 0) {
					continue;
				}
				uint32_t xh = dx * hbsize + hoffx, yh = (3 - dy) * hbsize + hoffy;
				dp_set_prim_color_rgba(colors[block], 0, 0);
				dp_texture_rectangle_10_raw(xh, yh, xh + hbsize, yh + hbsize, 1, 0, 0, 0x400*16/hbsize, 0x400*16/hbsize);
			}
		}
	}

	// ghost
	if(player->gamestate == PLAYER_INGAME && (mino->phase == MINO_PLACING /*|| mino->phase == MINO_GROUNDING*/)) {
		dp_pipe_sync();
		dpmode_transp();
		int32_t ly = mino_lowesty(mino);
		if(ly != mino->y) {
			for(int32_t dy = 0; dy < 4; dy++) {
				if(19 < ly + dy) {
					break;
				}
				for(int32_t dx = 0; dx < 4; dx++) {
					const int8_t (*form)[4] = &pieces[mino->piece].blocks[mino->rotate][0];
					int8_t block = form[dy][dx];
					if(block == 0) {
						continue;
					}
					// overlapping with falling block: skip drawing (opt)
					if(0 <= dy - mino->y + ly && form[dy - mino->y + ly][dx]) {
						continue;
					}
					uint32_t xh = (mino->x + dx) * bsize + foffx, yh = (19 - (ly + dy)) * bsize + foffy;
					dp_pipe_sync();
					dp_set_prim_color_rgba((colors[block] & 0xFFFFFF00) | /*0x30*/0xFF, 0, 0);
					dp_set_combine_mode2(DP_SCM_COLOR_TEX0xPRIM, DP_SCM_ALPHA_TEX0);
					dp_texture_rectangle_10_raw(xh, yh, xh + bsize, yh + bsize, 2, 0, 0, 0x400*16/16, 0x400*16/16);
					dp_pipe_sync();
					dp_set_combine_mode2(DP_SCM_COLOR_TEXEL0, DP_SCM_ALPHA_TEX0);
					dp_texture_rectangle_10_raw(xh, yh, xh + bsize, yh + bsize, 3, 0, 0, 0x400*16/13, 0x400*16/13);
				}
			}
		}
	}

	if(player->gamestate == PLAYER_PAUSE) {
		dploadtexblock1(OCHIMTEX_FMT, OCHIMTEX_SIZE, (uint32_t)ochimtex, 0, 16*2, 80, 16);
		dpmode_transp();
		dp_set_combine_mode2(DP_SCM_COLOR_TEXEL0, DP_SCM_ALPHA_TEX0);
		int32_t x = foffx + (14*10 - 72) / 2, yh = foffy + 20 * bsize / 2 - 8;
		dp_texture_rectangle_10_raw(x, yh, x + 80, yh + 16, 0, 0, 0, 0x400, 0x400);
	}

	draw_rocks(player, foffx, foffy - 16);

	// score
	dp_pipe_sync();
	dpmode_opaque();
	dp_set_combine_mode8(
			DP_SCM_COLORA_TEXEL0,DP_SCM_COLORB_PRIMITIVE,DP_SCM_COLORC_TEX0A,DP_SCM_COLORD_ZERO,
			DP_SCM_ALPHAA_ZERO,DP_SCM_ALPHAB_ZERO,DP_SCM_ALPHAC_ZERO,DP_SCM_ALPHAD_ONE);
	/* XXX: ouch! touching input var directly!! */
	if(controller_inputs[player - gstate.players] == -1) {
		dp_set_prim_color_rgba(0xC0C0C0FF, 0, 0); // minus-ing
	} else {
		dp_set_prim_color_rgba(0x000000FF, 0, 0);
	}
	// qdh: show zeros
	draw_number(100000 + player->score, foffx + (10 * bsize - 12 * 5) / 2 + 12 * 4, foffy + 20 * bsize + 10, 5, 0x10000);

	dp_pipe_sync();
	dpmode_transp();
	if(controller_inputs[player - gstate.players] != -1 && (player->gamestate == PLAYER_CONFIG || player->gamestate == PLAYER_READY)) {
		if(player->gamestate == PLAYER_CONFIG) {
			int32_t xh = foffx, yh = foffy + (20 * bsize - (10*20+16)) / 2;

			for(int32_t i = 0; i < 10; i++) {
				int32_t v;
				switch(i) {
				case 1: v = player->gametype; break;
				case 2: v = player->das; break;
				case 3: v = player->repeatwait; break;
				case 4: v = mino->fallspeed; break;
				case 5: v = mino->groundwait; break;
				case 6: v = mino->erasewait; break;
				case 7: v = mino->rocksperturn; break;
				case 8: v = mino->initrocks; break;
				case 9: v = mino->are; break;
				}

				draw_config(player, xh, yh, 10 * bsize * (player->confitem == i), i, v);
				yh += 20 + (i < 2 ? 8 : 0);
			}
		} else {
			int32_t xh = foffx, yh = foffy + (20 * bsize - 16) / 2;
			draw_config(player, xh, yh, 10 * bsize, 0, 0);
		}
	}

	if(player->gamestate == PLAYER_INGAME && mino->phase == MINO_READY) {
		draw_readygo(foffx, foffy, 10 * bsize, 40 < mino->phaset);
	} else if(player->gamestate == PLAYER_GAMEOVER) {
		draw_gameover(player, foffx, foffy, 10 * bsize);
	}
}

static void draw_turu(int32_t offx, int32_t offy, struct player *player) {
	struct turu * const turu = &player->turu;

	static const int32_t bsize = 23;

	const uint32_t foffx = offx + 1, foffy = offy + 4 + 3 * /*bsize*/14; // field
	const uint32_t noffx = offx + 2 * bsize, noffy = offy; // next

	// field bg
	dp_pipe_sync();
	dpmode_opaque();
	dp_set_combine_mode2(DP_SCM_COLOR_PRIMITIVE, DP_SCM_ALPHA_PRIMITIVE);
	dp_set_prim_color_rgba(0xFFFFFFFF, 0, 0);
	dp_fillrect(foffx - 4, foffy, foffx, foffy + 12 * bsize);
	dp_fillrect(foffx + 6 * bsize, foffy, foffx + 6 * bsize + 4, foffy + 12 * bsize);
	dp_fillrect(foffx - 4, foffy + 12 * bsize, foffx + 6 * bsize + 4, foffy + 12 * bsize + 4);

	if(gstate.bgtex){
		dpmode_transp();
	}
	dp_set_prim_color_rgba(0x101010C0, 0, 0);
	dp_fillrect(foffx, foffy, foffx + 6 * bsize, foffy + 12 * bsize);

	// commons
	dp_pipe_sync();
	dpmode_transp();
	dp_set_combine_mode2(DP_SCM_COLOR_TEX0xPRIM, DP_SCM_ALPHA_TEX0);

	// X mark
	dploadtexblock1(TURUTEX_FMT, TURUTEX_SIZE, (uint32_t)turutex + 19 * 16*16*DP_BYPP(TURUTEX_SIZE), 0, 0, 16, 16);
	dp_set_prim_color(255, 64, 64, 255, 0, 0);
	{
		uint32_t xh = foffx + 2 * bsize, yh = foffy;
		dp_texture_rectangle_10_raw(xh, yh, xh + bsize, yh + bsize, 0, 0, 0, 0x400*16/bsize, 0x400*16/bsize);
	}

	if(player->gamestate == PLAYER_INGAME) {
		// field
		dp_set_scissor_10(foffx, foffy, foffx + 6 * bsize, foffy + 12 * bsize, curfield + DP_SCISSOR_KEEPEVEN); // TODO frame-skip adaptive frame selection
		for(int32_t y = 0; y < 12; y++) {
			for(int32_t x = 0; x < 6; x++) {
				int8_t fd = turu->field[y][x];
				int8_t ft = turu->floating[y][x];
				if(!fd && !ft) {
					continue;
				}

				if(turu->phase == TURU_ERASING && fd == ft && (((40 < turu->phaset) && (turu->phaset % 6 < 3)) || (turu->phaset < 20))) {
					continue;
				}

				uint32_t code = 0;
				if(fd == 7 || ft == 7 || ft == -7/*settling rock*/) {
					code = 16;
				} else if(fd) {
					code = turu_concode(turu, x, y);
				}
				dploadtexblock1(TURUTEX_FMT, TURUTEX_SIZE, (uint32_t)turutex + code * 16*16*DP_BYPP(TURUTEX_SIZE), 0, 0, 16, 16);

				uint32_t xh = x * bsize + foffx, yh = (11 - y) * bsize + foffy;
				int32_t dw = 0, dh = 0;
				int32_t c;
				if(turu->phase == TURU_GAMEOVER) {
					c = turu->colors[7];
					yh += 0 < turu->goys[x] ? turu->goys[x] / 0x10000 : 0;
				} else if(fd) {
					c = turu->colors[fd - 1];
					if(ft) {
						if(turu->phase == TURU_PLACING) {
							// slow-flash
							// TODO: can't we optimize this???
							int32_t m = gstate.t % 30;
							if(15 <= m) {
								m = 30 - m;
							}
							int32_t r = 255 - ((255 - ((c >> 24) & 255)) * (15 - m) / 15);
							int32_t g = 255 - ((255 - ((c >> 16) & 255)) * (15 - m) / 15);
							int32_t b = 255 - ((255 - ((c >>  8) & 255)) * (15 - m) / 15);
							c = (r << 24) | (g << 16) | (b << 8) | 255;
						} else if(turu->phase == TURU_ERASING) {
							// whity
							int32_t m = 40 < turu->phaset ? 20 : CLAMP(0, 40 - turu->phaset, 20);
							int32_t r = 255 - ((255 - ((c >> 24) & 255)) * m / 20);
							int32_t g = 255 - ((255 - ((c >> 16) & 255)) * m / 20);
							int32_t b = 255 - ((255 - ((c >>  8) & 255)) * m / 20);
							c = (r << 24) | (g << 16) | (b << 8) | 255;
						}
					}
				} else { // !fd&&ft
					c = turu->colors[/*abs*/(0 < ft ? ft : -ft) - 1];
					if(ft < 0 && ft != -7) {
						dw = gstate.t / 4 % 4;
						dh = gstate.t / 4 % 4;
					}
				}

				dp_set_prim_color_rgba(c, 0, 0);
				if((turu->phase == TURU_FALLING || turu->phase == TURU_ROCKFALLING) && 0 < ft) {
					yh -= turu->phaset * bsize / 60;
				}
				dp_texture_rectangle_10_raw(xh - dw, yh + dh, xh + bsize + dw, yh + bsize, 0, 0, 0, 0x400*16/(bsize + dw * 2), 0x400*16/(bsize - dh));
			}
		}
	}

	dploadtexblock1(TURUTEX_FMT, TURUTEX_SIZE, (uint32_t)turutex, 0, 0, 16, 16);

	if(player->gamestate == PLAYER_INGAME) {
		// falling piece
		if(turu->phase == TURU_PLACING) {
			int32_t x = turu->x, y = turu->y;
			int32_t dy = /*clamplo*/(turu->phaset < -10 ? -10 : turu->phaset) * bsize / 60;

			{
				uint32_t xh = x * bsize + foffx + turu->csx * bsize / 60, yh = (11 - y) * bsize + foffy + turu->csy * bsize / 60 - dy;

				dp_set_prim_color_rgba(turu->colors[turu->piece[0] - 1], 0, 0);
				dp_texture_rectangle_10_raw(xh, yh, xh + bsize, yh + bsize, 0, 0, 0, 0x400*16/bsize, 0x400*16/bsize);
			}
			turu_pairxy(&x, &y, turu->rotate);
			{
				uint32_t xh = x * bsize + foffx + turu->psx * bsize / 60, yh = (11 - y) * bsize + foffy + turu->psy * bsize / 60 - dy;

				dp_set_prim_color_rgba(turu->colors[turu->piece[1] - 1], 0, 0);
				dp_texture_rectangle_10_raw(xh, yh, xh + bsize, yh + bsize, 0, 0, 0, 0x400*16/bsize, 0x400*16/bsize);
			}
		}

		dp_set_scissor_10(0, 0, 640, 480, curfield + DP_SCISSOR_KEEPEVEN); // TODO frame-skip adaptive frame selection
	}

	// next piece
	if(0 < turu->next[0]) {
		int32_t bsize = 18; // override smaller
		dp_set_prim_color_rgba(turu->colors[turu->next[1] - 1], 0, 0);
		dp_texture_rectangle_10_raw(noffx, noffy, noffx + bsize, noffy + bsize, 0, 0, 0, 0x400*16/bsize, 0x400*16/bsize);
		dp_set_prim_color_rgba(turu->colors[turu->next[0] - 1], 0, 0);
		dp_texture_rectangle_10_raw(noffx, noffy + bsize, noffx + bsize, noffy + bsize * 2, 0, 0, 0, 0x400*16/bsize, 0x400*16/bsize);
	}

	if(player->gamestate == PLAYER_PAUSE) {
		dploadtexblock1(OCHIMTEX_FMT, OCHIMTEX_SIZE, (uint32_t)ochimtex, 0, 16*2, 80, 16);
		dpmode_transp();
		dp_set_combine_mode2(DP_SCM_COLOR_TEXEL0, DP_SCM_ALPHA_TEX0);
		int32_t x = foffx + (14*10 - 72) / 2, yh = foffy + 12 * bsize / 2 - 8;
		dp_texture_rectangle_10_raw(x, yh, x + 80, yh + 16, 0, 0, 0, 0x400, 0x400);
	}
	draw_rocks(player, foffx, foffy - 16);

	// score
	dp_pipe_sync();
	dpmode_opaque();
	dp_set_combine_mode8(
			DP_SCM_COLORA_TEXEL0,DP_SCM_COLORB_PRIMITIVE,DP_SCM_COLORC_TEX0A,DP_SCM_COLORD_ZERO,
			DP_SCM_ALPHAA_ZERO,DP_SCM_ALPHAB_ZERO,DP_SCM_ALPHAC_ZERO,DP_SCM_ALPHAD_ONE);
	/* XXX: ouch! touching input var directly!! */
	if(controller_inputs[player - gstate.players] == -1) {
		dp_set_prim_color_rgba(0xC0C0C0FF, 0, 0); // minus-ing
	} else {
		dp_set_prim_color_rgba(0x000000FF, 0, 0);
	}
	// qdh: show zeros
	draw_number(100000 + player->score, foffx + (6 * bsize - 12 * 5) / 2 + 12 * 4, foffy + 12 * bsize + 10, 5, 0x10000);

	dp_pipe_sync();
	dpmode_transp();
	if(controller_inputs[player - gstate.players] != -1 && (player->gamestate == PLAYER_CONFIG || player->gamestate == PLAYER_READY)) {
		if(player->gamestate == PLAYER_CONFIG) {
			int32_t xh = foffx, yh = foffy + (12 * bsize - (10*20+16)) / 2;

			for(int32_t i = 0; i < 10; i++) {
				int32_t v;
				switch(i) {
				case 1: v = player->gametype; break;
				case 2: v = player->das; break;
				case 3: v = player->repeatwait; break;
				case 4: v = turu->fallspeed; break;
				case 5: v = turu->groundwait; break;
				case 6: v = turu->erasewait; break;
				case 7: v = turu->rocksperturn; break;
				case 8: v = turu->initrocks; break;
				case 9: v = turu->ncolors; break;
				}

				draw_config(player, xh, yh, 6 * bsize * (player->confitem == i), i + (8 < i ? 1 : 0), v);
				yh += 20 + (i < 2 ? 8 : 0);
			}
		} else {
			int32_t xh = foffx, yh = foffy + (12 * bsize - 16) / 2;
			draw_config(player, xh, yh, 6 * bsize, 0, 0);
		}
	}

	if(player->gamestate == PLAYER_INGAME && turu->phase == TURU_READY) {
		draw_readygo(foffx, foffy, 6 * bsize, 40 < turu->phaset);
	} else if(player->gamestate == PLAYER_GAMEOVER) {
		draw_gameover(player, foffx, foffy, 6 * bsize);
	}
}

static void draw_commit(void) {
#ifdef DEBUG
	int32_t dspt = prof_time();
#endif
	dp_run();
	while(DP->status & (DP_STATUS_CMD_BUSY | DP_STATUS_DMA_BUSY))
#ifdef DEBUG
	{
		int32_t pt = prof_time();
		if(dspt + 93750000/1 < pt) fatalhandler();
	}
#else
		/*nothing*/;
#endif
}

static void draw(void) {
#ifdef DEBUG
	int32_t dspt = prof_time();
#endif
	// Don't wait for CBUF_READY but !CMDBUSY/!DMABUSY just for angrylion
	while(DP->status & (DP_STATUS_CMD_BUSY | DP_STATUS_DMA_BUSY))
#ifdef DEBUG
	{
		int32_t pt = prof_time();
		if(dspt + 93750000/1 < pt) fatalhandler();
	}
#else
		/*nothing*/;
#endif

	dp_rewind();

	dp_set_color_image(DP_FORMAT_RGBA, DP_SIZE_16B, 640, (uint32_t)updatingcfb);

	dp_set_scissor_10(0, 0, 639, 480, curfield + DP_SCISSOR_KEEPEVEN); // TODO frame-skip adaptive frame selection

	draw_clean();

	if(gstate.gamedraw) {
		for(int32_t i = 0; i < 4; i++) {
#ifndef EMU
			dp_run(); // partial
#endif
			struct player * const player = &gstate.players[i];
			switch(player->gametype) {
			case GAMETYPE_MINO: draw_mino(12 + (14 * 10 + 15) * i, 30, player); break;
			case GAMETYPE_TURU: draw_turu(12 + (14 * 10 + 15) * i, 30, player); break;
			}
		}

#ifndef EMU
		dp_run(); // partial
#endif

		dploadtexblock1(TURUTEX_FMT, TURUTEX_SIZE, (uint32_t)turutex, 0, 16*16, 16, 16);
		dpmode_transp();
		dp_set_combine_mode2(DP_SCM_COLOR_TEXEL0, DP_SCM_ALPHA_TEX0);
		for(int32_t i = 0; i < ARRAY_SIZE(gstate.flyingrocks); i++) {
			struct flyrock *fr = &gstate.flyingrocks[i];
			if(!fr->dstplayer) {
				continue;
			}

			dp_texture_rectangle_10_raw(fr->x / 0x10000, fr->y / 0x10000, fr->x / 0x10000 + 16, fr->y / 0x10000 + 16, 0, 0, 0, 0x400, 0x400);
		}
	}

#ifdef PROFILE
	dp_pipe_sync();
	dpmode_opaque();
	dp_set_combine_mode2(DP_SCM_COLOR_PRIMITIVE, DP_SCM_ALPHA_PRIMITIVE);
	{
		const int32_t w = 600, h = 10;
		const int32_t x = (640 - w) / 2, y = 480 - h - 20;
		dp_set_prim_color(128, 128, 128, 255, 0, 0);
		dp_fillrect(x - 2, y - 2, x + w + 2, y + h + 2);
		dp_set_prim_color(40, 40, 40, 255, 0, 0);
		dp_fillrect(x, y, x + w, y + h);

		float max = oldprofile.end - oldprofile.start;

		dp_set_prim_color(64, 0, 0, 255, 0, 0);
		dp_fillrect(x, y, x + (int32_t)((oldprofile.input - oldprofile.start) / max * w), y + h);
		dp_set_prim_color(0, 64, 0, 255, 0, 0);
		dp_fillrect(x + (uint32_t)((oldprofile.input - oldprofile.start) / max * w), y, x + (uint32_t)((oldprofile.drawcmd - oldprofile.start) / max * w), y + h);
		dp_set_prim_color(0, 64, 64, 255, 0, 0);
		dp_fillrect(x + (uint32_t)((oldprofile.drawcmd - oldprofile.start) / max * w), y, x + (uint32_t)((oldprofile.drawwait - oldprofile.start) / max * w), y + h);
		dp_set_prim_color(32, 32, 64, 255, 0, 0);
		dp_fillrect(x + (uint32_t)((oldprofile.drawwait - oldprofile.start) / max * w), y, x + (uint32_t)((oldprofile.hotload - oldprofile.start) / max * w), y + h);
		dp_set_prim_color(255, 64, 0, 255, 0, 0);
		dp_fillrect(x + (uint32_t)((oldprofile.hotload - oldprofile.start) / max * w), y, x + (uint32_t)((oldprofile.amix - oldprofile.start) / max * w), y + h);
		dp_set_prim_color(0, 0, 64, 255, 0, 0);
		dp_fillrect(x + (uint32_t)((oldprofile.amix - oldprofile.start) / max * w), y, x + (uint32_t)((oldprofile.halt - oldprofile.start) / max * w), y + h);
		dp_set_prim_color(255, 0, 0, 255, 0, 0);
		dp_fillrect(x + (uint32_t)((93750000/2/60) / max * w), y - 4, x + (uint32_t)((93750000/2/60) / max * w) + 2, y + h + 4);
		dp_set_prim_color(255, 128, 128, 255, 0, 0);
		dp_fillrect(x + (uint32_t)((oldprofile.frame - oldprofile.start) / max * w), y, x + (uint32_t)((oldprofile.frame - oldprofile.start) / max * w) + 2, y + h);
	}
#endif

	dp_full_sync();

	PROF(drawcmd);

	draw_commit();

	PROF(drawwait);
}

int main(void) {
	init_video();
	init_audio();

	MI->intrmask =
		MI_INTRMASK_CLEAR_SP |
		MI_INTRMASK_CLEAR_SI |
		MI_INTRMASK_SET_AI |
		MI_INTRMASK_SET_VI |
		MI_INTRMASK_CLEAR_PI |
		MI_INTRMASK_CLEAR_DP ;

	// enable intr with IP2(MI), IP4(PreNMI), and FPU.
	setsr(getsr() | STATUS_CU1 | STATUS_IM4 | STATUS_IM2 | STATUS_IE);

	DP->status = DP_STATUS_CLEAR_XBUS_DMEM_DMA | DP_STATUS_CLEAR_FREEZE | DP_STATUS_CLEAR_FLUSH;

	// game initialize
	for(int32_t i = 0; i < ARRAY_SIZE(gstate.flyingrocks); i++) {
		gstate.flyingrocks[i].dstplayer = 0;
	}
	for(int32_t i = 0; i < 4; i++) {
		gstate.players[i].next = &gstate.players[(i + 1) % 4];
		gstate.players[i].id = i;
		player_default_config(&gstate.players[i]);
		player_init(&gstate.players[i], 1);
	}
	gstate.bgtex = 1;
	gstate.gamedraw = 1;

	// call twice to flush (invalid) empty oldprofile
	prof_newframe();
	prof_newframe();

	aplay(ready);

	for(;;) {
#ifdef DEBUG
#ifndef EMU
		hotload_rx();
#endif
#endif
		PROF(hotload);

		if(async) {
			amix(ab[nextabi]);
		}
		PROF(amix);

		vsync = 0;
		while(!vsync) {
			halt();
		}
		PROF(halt);

		prof_newframe();

		input();
		PROF(input);

		draw();

		flip();
	}
}
