#include <stdint.h>

#include "regs.h"
#include "cache.h"
#include "rdp.h"

#ifdef DEBUG
extern void fatalhandler(void);
#define CHECKOVERRUN do{ if(&rdpcmds[0] + sizeof(rdpcmds)/sizeof(rdpcmds[0]) <= prdpcmd){ fatalhandler(); } }while(0)
#else
#define CHECKOVERRUN
#endif

static uint64_t rdpcmds[8192], *prdpcmd, *pprdpcmd;

void dp_rewind(void) {
	prdpcmd = pprdpcmd = &rdpcmds[0];
	DP->start = (uint32_t)&rdpcmds[0];
}

/* you must wait RDP while busy before call this. */
void dp_run(void) {
	dwbinval(pprdpcmd, (char*)prdpcmd - (char*)pprdpcmd);
	DP->end = (uint32_t)prdpcmd;
	pprdpcmd = prdpcmd;
}

void _dp_triangle(uint64_t y, uint64_t xl, uint64_t xh, uint64_t xm) {
	CHECKOVERRUN;
	*prdpcmd++ = y;
	*prdpcmd++ = xl;
	*prdpcmd++ = xh;
	*prdpcmd++ = xm;
}

/* valid fmt/size: rgba/16b, rgba/32b, index/8b */
void dp_set_color_image(uint32_t fmt, uint32_t size, uint32_t width, uint32_t dramadrs/*26b*/) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x3F << 56) | ((uint64_t)fmt << 53) | ((uint64_t)size << 51) | ((uint64_t)(width - 1) << 32) | ((uint64_t)dramadrs << 0);
}

/* valid fmt/size: rgba/16b, rgba/32b, yuv/16b, index/4b, index/8b, ia/4b, ia/8b, ia/16b, i/4b, i/8b */
void dp_set_texture_image(uint32_t fmt, uint32_t size, uint32_t width, uint32_t dramadrs) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x3D << 56) | ((uint64_t)fmt << 53) | ((uint64_t)size << 51) | ((uint64_t)(width - 1) << 32) | ((uint64_t)dramadrs << 0);
}

/* too many fields... just pass as a big dword */
void _dp_set_other_modes(uint64_t params) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x2F << 56) | ((uint64_t)params << 0);
}

/* too many fields... just pass as a big dword */
void _dp_set_combine_mode(uint64_t params) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x3C << 56) | ((uint64_t)params << 0);
}

void dp_set_fill_color(uint32_t fillword) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x37 << 56) | ((uint64_t)fillword << 0);
}

void dp_set_prim_color_rgba(uint32_t rgba, uint32_t minlevel/*u0.5*/, uint32_t lodfrac/*u0.8*/) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x3A << 56) | ((uint64_t)minlevel << 40) | ((uint64_t)lodfrac << 32) | ((uint64_t)rgba << 0);
}

void dp_set_prim_depth(uint16_t z, uint16_t dz) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x2E << 56) | ((uint64_t)z << 16) | ((uint64_t)dz << 0);
}

/* coordinates are u10.2, but here is u10.0. both are inclusive. */
void dp_fill_rectangle_10_raw(uint32_t xh, uint32_t yh, uint32_t xl, uint32_t yl) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x36 << 56) | ((uint64_t)(xl & 0x3FF) << (44+2)) | ((uint64_t)(yl & 0x3FF) << (32+2)) | ((uint64_t)(xh & 0x3FF) << (12+2)) | ((uint64_t)(yh & 0x3FF) << (0+2));
}

void dp_fillrect(int32_t xh, int32_t yh, int32_t xl, int32_t yl) {
	if(xl < xh) {
		xl ^= xh;
		xh ^= xl;
		xl ^= xh;
	}
	if(yl < yh) {
		yl ^= yh;
		yh ^= yl;
		yl ^= yh;
	}
	if(xl < 0) {
		return;
	}
	if(yl < 0) {
		return;
	}
	if(xh < 0) {
		xh = 0;
	}
	if(yh < 0) {
		yh = 0;
	}
#if 1
	dp_fill_rectangle_10_raw(xh, yh, xl, yl);
#else
	// primtri
	/*      xh,yh             xh,yh
	 *        .___.             .___.
	 *  Maj.  |  /              |   |
	 *  edge->| / <- DxLDy=0 => |   |
	 *        |/                |   |
	 *        '                 '    xl,yl
	 */
	CHECKOVERRUN;
	uint32_t lft = 1, miplevels = 1, tile = 0;
	uint32_t tyl = yl, tym = yh, tyh = yh, txl = xl << 16, dxldy = 0, txh = xh << 16, dxhdy = 0, txm = xl << 16, dxmdy = 0;
	*prdpcmd++ = ((uint64_t)0x08 << 56) | ((uint64_t)lft << 55) | ((uint64_t)(miplevels - 1) << 51) | ((uint64_t)tile << 48) | ((uint64_t)(tyl & 0x3FFF) << (32+2)) | ((uint64_t)(tym & 0x3FFF) << (16+2)) | ((uint64_t)(tyh & 0x3FFF) << (0+2));
	*prdpcmd++ = ((uint64_t)txl << 32) | ((uint64_t)dxldy << 0);
	*prdpcmd++ = ((uint64_t)txh << 32) | ((uint64_t)dxhdy << 0);
	*prdpcmd++ = ((uint64_t)txm << 32) | ((uint64_t)dxmdy << 0);
#endif
}

/* s,t,dsdx,dtdy are still s10.5 and s5.10 */
/* gbi.h swaps name h and l. */
void dp_texture_rectangle_10_raw(uint32_t xh, uint32_t yh, uint32_t xl, uint32_t yl, uint32_t tile, uint32_t s, uint32_t t, uint32_t dsdx, uint32_t dtdy) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x24 << 56) | ((uint64_t)xl << (44+2)) | ((uint64_t)yl << (32+2)) | ((uint64_t)tile << 24) | ((uint64_t)xh << (12+2)) | ((uint64_t)yh << (0+2));
	*prdpcmd++ = ((uint64_t)s << 48) | ((uint64_t)t << 32) | ((uint64_t)dsdx << 16) | ((uint64_t)dtdy << 0);
}
void dp_texture_rectangle_flip_10_raw(uint32_t xh, uint32_t yh, uint32_t xl, uint32_t yl, uint32_t tile, uint32_t s, uint32_t t, uint32_t dsdx, uint32_t dtdy) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x25 << 56) | ((uint64_t)xh << (44+2)) | ((uint64_t)yh << (32+2)) | ((uint64_t)tile << 24) | ((uint64_t)xl << (12+2)) | ((uint64_t)yl << (0+2));
	*prdpcmd++ = ((uint64_t)s << 48) | ((uint64_t)t << 32) | ((uint64_t)dsdx << 16) | ((uint64_t)dtdy << 0);
}

void dp_set_scissor_10(uint32_t xh, uint32_t yh, uint32_t xl, uint32_t yl, enum DP_SCISSOR_FIELD field) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x2D << 56) | ((uint64_t)xh << (44+2)) | ((uint64_t)yh << (32+2)) | ((uint64_t)field << 24) | ((uint64_t)xl << (12+2)) | ((uint64_t)yl << (0+2));
}

void _dp_set_tile(uint64_t params) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x35 << 56) | ((uint64_t)params << 0);
}

/* gbi says cmd,uls,ult,tile,lrs,lrt. l and h name are swapped in sgi doc?? */
/* bottom-right texel seems inclusive. -1 here makes exclusive. */
void dp_load_tile_10(uint32_t tile, uint32_t sl, uint32_t tl, uint32_t sh, uint32_t th) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x34 << 56) | ((uint64_t)sl << (44+2)) | ((uint64_t)tl << (32+2)) | ((uint64_t)tile << 24) | ((uint64_t)(sh - 1) << (12+2)) | ((uint64_t)(th - 1) << (0+2));
}

/* SH seems inclusive. NOTE that only load_block is u12.0 coord, not u10.2 like load_tile or set_tile_size. */
static void dp_load_block_raw(uint32_t tile, uint32_t sl, uint32_t tl, uint32_t sh, uint32_t dxt) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x33 << 56) | ((uint64_t)sl << 44) | ((uint64_t)tl << 32) | ((uint64_t)tile << 24) | ((uint64_t)sh << 12) | ((uint64_t)dxt << 0);
}

/* easier interface */
/* width*DP_BIPP(size) must be multiple of 64. so, words does not contain roundup. */
void dp_load_block(uint32_t tile, uint32_t sl, uint32_t tl, uint32_t width, uint32_t height, enum DP_PIXEL_SIZE size) {
	uint32_t dxt;
	if(size == DP_SWAPPED) {
		dxt = 0;
	} else {
		const uint32_t words = width * DP_BIPP(size) / 64; // width in 64bit-words
		dxt = ((1 << 11) + words - 1) / words; // round-up u1.11
	}
	const uint32_t sh = sl + width * height - 1;
	dp_load_block_raw(tile, sl, tl, sh, dxt);
}

/* bottom-right texel seems inclusive. -1 here makes exclusive. */
void dp_set_tile_size_10(uint32_t tile, uint32_t sl, uint32_t tl, uint32_t sh, uint32_t th) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x32 << 56) | ((uint64_t)sl << (44+2)) | ((uint64_t)tl << (32+2)) | ((uint64_t)tile << 24) | ((uint64_t)(sh - 1) << (12+2)) | ((uint64_t)(th - 1) << (0+2));
}

void dp_load_tlut_10(uint32_t tile, uint32_t sl, uint32_t tl, uint32_t sh, uint32_t th) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x30 << 56) | ((uint64_t)sl << (44+2)) | ((uint64_t)tl << (32+2)) | ((uint64_t)tile << 24) | ((uint64_t)sh << (12+2)) | ((uint64_t)th << (0+2));
}

void dp_load_sync(void) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x26 << 56);
}

void dp_pipe_sync(void) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x27 << 56);
}

void dp_tile_sync(void) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x28 << 56);
}

void dp_full_sync(void) {
	CHECKOVERRUN;
	*prdpcmd++ = ((uint64_t)0x29 << 56);
}
