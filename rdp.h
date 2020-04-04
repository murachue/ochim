
// DP_SIZE_*B -> bit/byte per pixel
#define DP_BIPP(size) (1 << (2 + (size)))
#define DP_BYPP(size) (DP_BIPP(size) / 8) // NOTE: 4B -> 0

enum DP_COLOR_FORMAT {
	DP_FORMAT_RGBA = 0,
	DP_FORMAT_YUV = 1,
	DP_FORMAT_INDEX = 2,
	DP_FORMAT_IA = 3,
	DP_FORMAT_I = 4,
};

enum DP_PIXEL_SIZE {
	DP_SIZE_4B = 0,
	DP_SIZE_8B = 1,
	DP_SIZE_16B = 2,
	DP_SIZE_32B = 3,

	DP_SWAPPED = 4, // special value only for dp_load_block
};

/* [25:24] (both f and o bits) */
enum DP_SCISSOR_FIELD {
	DP_SCISSOR_KEEPALL = 0,
	DP_SCISSOR_KEEPEVEN = 2,
	DP_SCISSOR_KEEPODD = 3,
};

enum DP_SET_OTHER_MODES_VALUES {
	DP_SOM_NPRIMITIVE = 0,
	DP_SOM_1PRIMITIVE = 1,

	DP_SOM_1CYC = 0,
	DP_SOM_2CYC = 1,
	DP_SOM_COPY = 2,
	DP_SOM_FILL = 3,

	DP_SOM_NOPERSP = 0,
	DP_SOM_PERSP = 1,

	// [50:48]
	DP_SOM_NOLOD = 0,
	DP_SOM_LOD = 1,
	DP_SOM_SHAPENLOD = 3,
	DP_SOM_DETAILLOD = 5,

	// [47:46]
	DP_SOM_NOTLUT = 0,
	DP_SOM_TLUTRGBA = 2, // rgba5551
	DP_SOM_TLUTIA = 3, // ia88

	// [45:44]
	DP_SOM_POINT = 0,
	DP_SOM_BILERP = 2,
	DP_SOM_MID = 3, // "average"/"box"?

	// [43:41]
	DP_SOM_CONVALL = 0, // convert on each cycle
	DP_SOM_CONVONE = 5, // convert cycle0 then bilerp?
	DP_SOM_NOCONV = 6, // don't convert at all

	DP_SOM_NOKEY = 0,
	DP_SOM_KEY = 1,

	DP_SOM_RGBDITHER_MSM = 0, /* magic square matrix */
	DP_SOM_RGBDITHER_BAYER = 1, /* "standard" bayer matrix */
	DP_SOM_RGBDITHER_NOISE = 2,
	DP_SOM_RGBDITHER_NONE = 3,

	DP_SOM_ALPHADITHER_PATTERN = 0,
	DP_SOM_ALPHADITHER_NOTPATTERN = 1,
	DP_SOM_ALPHADITHER_NOISE = 2,
	DP_SOM_ALPHADITHER_NONE = 3,

	DP_SOM_BLEND_MA_PIXEL = 0, /* 1st cycle (M1A) */
	DP_SOM_BLEND_MA_BLENDED = 0, /* 2nd cycle (M2A) */
	DP_SOM_BLEND_MA_MEMORY = 1,
	DP_SOM_BLEND_MA_BLEND = 2,
	DP_SOM_BLEND_MA_FOG = 3,

	DP_SOM_BLEND_M1B_CC = 0, /* color combiner output */
	DP_SOM_BLEND_M1B_FOG = 1,
	DP_SOM_BLEND_M1B_SHADE = 2,
	DP_SOM_BLEND_M1B_ZERO = 3,

	DP_SOM_BLEND_M2B_1MINUS = 0,
	DP_SOM_BLEND_M2B_MEMORY = 1,
	DP_SOM_BLEND_M2B_ONE = 2,
	DP_SOM_BLEND_M2B_ZERO = 3,

	DP_SOM_CVGBLEND = 0,
	DP_SOM_FORCEBLEND = 1,

	// key/(alpha*)cvg switch for alpha for blender (and for alpha compare)
	// AC->AlphaCvgSel
	DP_SOM_AC_KEY = 0,
	DP_SOM_AC_ALPHA = 1,

	// cvg for blender
	// ->CvgXAlpha
	DP_SOM_CVG = 0,
	DP_SOM_CVG_XALPHA = 1,

	DP_SOM_ZMODE_OPAQUE = 0,
	DP_SOM_ZMODE_INTERP = 1, /* inter-penetrating */
	DP_SOM_ZMODE_TRANSP = 2, /* transparent */
	DP_SOM_ZMODE_DECAL = 3,

	DP_SOM_CVGDEST_CLAMP = 0, /* normal */
	DP_SOM_CVGDEST_WRAP = 1, /* assume full coverage */
	DP_SOM_CVGDEST_ZAP = 2, /* force to full coverage */
	DP_SOM_CVGDEST_SAVE = 3, /* don't overwrite memory coverage */

	DP_SOM_COLORALWAYS = 0,
	DP_SOM_COLORONCVGOV = 1, // only update color on coverage overflow (transparent surfaces)

	DP_SOM_NOIMRD = 0,
	DP_SOM_IMRD = 1, // image read (color/cvg rmw memory access)

	DP_SOM_NOZUPDATE = 0,
	DP_SOM_ZUPDATE = 1,

	DP_SOM_NOZCOMPARE = 0,
	DP_SOM_ZCOMPARE = 1,

	DP_SOM_NOAA = 0,
	DP_SOM_AA = 1,

	DP_SOM_ZSRC_PIXEL = 0,
	DP_SOM_ZSRC_PRIMITIVE = 1,

	// [1:0]
	DP_SOM_ALPHACOMP_NONE = 0,
	DP_SOM_ALPHACOMP_THRESHOLD = 1,
	DP_SOM_ALPHACOMP_DITHER = 3,
};

/* (A - B) * C + D */
enum DP_SET_COMBINE_MODE_VALUES {
	DP_SCM_COLORA_COMBINED = 0,
	DP_SCM_COLORA_TEXEL0 = 1,
	DP_SCM_COLORA_TEXEL1 = 2,
	DP_SCM_COLORA_PRIMITIVE = 3,
	DP_SCM_COLORA_SHADE = 4,
	DP_SCM_COLORA_ENV = 5,
	DP_SCM_COLORA_ONE = 6,
	DP_SCM_COLORA_NOISE = 7,
	DP_SCM_COLORA_ZERO = 15, /* 8-15 */

	DP_SCM_COLORB_COMBINED = 0,
	DP_SCM_COLORB_TEXEL0 = 1,
	DP_SCM_COLORB_TEXEL1 = 2,
	DP_SCM_COLORB_PRIMITIVE = 3,
	DP_SCM_COLORB_SHADE = 4,
	DP_SCM_COLORB_ENV = 5,
	DP_SCM_COLORB_KEYCENTER = 6,
	DP_SCM_COLORB_K4 = 7,
	DP_SCM_COLORB_ZERO = 15, /* 8-15 */

	DP_SCM_COLORC_COMBINED = 0,
	DP_SCM_COLORC_TEXEL0 = 1,
	DP_SCM_COLORC_TEXEL1 = 2,
	DP_SCM_COLORC_PRIMITIVE = 3,
	DP_SCM_COLORC_SHADE = 4,
	DP_SCM_COLORC_ENV = 5,
	DP_SCM_COLORC_KEYSCALE = 6,
	DP_SCM_COLORC_COMBINEDA = 7,
	DP_SCM_COLORC_TEX0A = 8,
	DP_SCM_COLORC_TEX1A = 9,
	DP_SCM_COLORC_PRIMA = 10,
	DP_SCM_COLORC_SHADEA = 11,
	DP_SCM_COLORC_ENVA = 12,
	DP_SCM_COLORC_LODFRAC = 13,
	DP_SCM_COLORC_PRIMLODF = 14,
	DP_SCM_COLORC_K5 = 15,
	DP_SCM_COLORC_ZERO = 31, /* 16-31 */

	DP_SCM_COLORD_COMBINED = 0,
	DP_SCM_COLORD_TEXEL0 = 1,
	DP_SCM_COLORD_TEXEL1 = 2,
	DP_SCM_COLORD_PRIMITIVE = 3,
	DP_SCM_COLORD_SHADE = 4,
	DP_SCM_COLORD_ENV = 5,
	DP_SCM_COLORD_ONE = 6,
	DP_SCM_COLORD_ZERO = 7,

	DP_SCM_ALPHAA_COMBINED = 0,
	DP_SCM_ALPHAA_TEX0 = 1,
	DP_SCM_ALPHAA_TEX1 = 2,
	DP_SCM_ALPHAA_PRIMITIVE = 3,
	DP_SCM_ALPHAA_SHADE = 4,
	DP_SCM_ALPHAA_ENV = 5,
	DP_SCM_ALPHAA_ONE = 6,
	DP_SCM_ALPHAA_ZERO = 7,

	DP_SCM_ALPHAB_COMBINED = 0,
	DP_SCM_ALPHAB_TEX0 = 1,
	DP_SCM_ALPHAB_TEX1 = 2,
	DP_SCM_ALPHAB_PRIMITIVE = 3,
	DP_SCM_ALPHAB_SHADE = 4,
	DP_SCM_ALPHAB_ENV = 5,
	DP_SCM_ALPHAB_ONE = 6,
	DP_SCM_ALPHAB_ZERO = 7,

	DP_SCM_ALPHAC_LODFRAC = 0,
	DP_SCM_ALPHAC_TEX0 = 1,
	DP_SCM_ALPHAC_TEX1 = 2,
	DP_SCM_ALPHAC_PRIMITIVE = 3,
	DP_SCM_ALPHAC_SHADE = 4,
	DP_SCM_ALPHAC_ENV = 5,
	DP_SCM_ALPHAC_PRIMLODF = 6,
	DP_SCM_ALPHAC_ZERO = 7,

	DP_SCM_ALPHAD_COMBINED = 0,
	DP_SCM_ALPHAD_TEX0 = 1,
	DP_SCM_ALPHAD_TEX1 = 2,
	DP_SCM_ALPHAD_PRIMITIVE = 3,
	DP_SCM_ALPHAD_SHADE = 4,
	DP_SCM_ALPHAD_ENV = 5,
	DP_SCM_ALPHAD_ONE = 6,
	DP_SCM_ALPHAD_ZERO = 7,
};

// [9:8](s), [19:18](t)
enum DP_TILE_FLAGS {
	DP_TILE_REPEAT = 0,
	DP_TILE_MIRROR = 1,
	DP_TILE_CLAMP = 2,
};

#define DP_SCM_COLOR_PRIMITIVE DP_SCM_COLORA_ZERO,DP_SCM_COLORB_ZERO,DP_SCM_COLORC_ZERO,DP_SCM_COLORD_PRIMITIVE
#define DP_SCM_COLOR_TEXEL0    DP_SCM_COLORA_ZERO,DP_SCM_COLORB_ZERO,DP_SCM_COLORC_ZERO,DP_SCM_COLORD_TEXEL0
#define DP_SCM_COLOR_TEX0xPRIM DP_SCM_COLORA_PRIMITIVE,DP_SCM_COLORB_ZERO,DP_SCM_COLORC_TEXEL0,DP_SCM_COLORD_ZERO
#define DP_SCM_ALPHA_PRIMITIVE DP_SCM_ALPHAA_ZERO,DP_SCM_ALPHAB_ZERO,DP_SCM_ALPHAC_ZERO,DP_SCM_ALPHAD_PRIMITIVE
#define DP_SCM_ALPHA_TEX0      DP_SCM_ALPHAA_ZERO,DP_SCM_ALPHAB_ZERO,DP_SCM_ALPHAC_ZERO,DP_SCM_ALPHAD_TEX0
#define DP_SCM_ALPHA_ONE       DP_SCM_ALPHAA_ZERO,DP_SCM_ALPHAB_ZERO,DP_SCM_ALPHAC_ZERO,DP_SCM_ALPHAD_ONE

void dp_rewind(void);
void dp_run(void);

void _dp_triangle(uint64_t y, uint64_t xl, uint64_t xh, uint64_t xm);
#define dp_triangle_prim_11_16_0(leftmajor, miplevel, yh, ym, yl, xh, dxhdy, xm, dxmdy, xl, dxldy) _dp_triangle(\
		((uint64_t)0x08 << 56) | ((uint64_t)(leftmajor) << 55) | ((uint64_t)(miplevel) << 51) | ((uint64_t)(0/*no tile for primtri*/) << 48) | \
		((uint64_t)(yl) << (32+2)) | ((uint64_t)(ym) << (16+2)) | ((uint64_t)(yh) << (0+2)), \
		((uint64_t)(xl) << 48) | ((uint64_t)(dxldy) << 0), \
		((uint64_t)(xh) << 48) | ((uint64_t)(dxhdy) << 0), \
		((uint64_t)(xm) << 48) | ((uint64_t)(dxmdy) << 0))
/*
#define dp_triangle_texture_11_16_0(leftmajor, miplevel, tile, yh, ym, yl, xh, dxhdy, xm, dxmdy, xl, dxldy) _dp_triangle(\
		((uint64_t)0x08 << 56) | ((uint64_t)(leftmajor) << 55) | ((uint64_t)(miplevel) << 51) | ((uint64_t)(tile) << 48) | \
		((uint64_t)(yl) << (32+2)) | ((uint64_t)(ym) << (16+2)) | ((uint64_t)(yh) << (0+2)), \
		((uint64_t)(xl) << 48) | ((uint64_t)(dxldy) << 0), \
		((uint64_t)(xh) << 48) | ((uint64_t)(dxhdy) << 0), \
		((uint64_t)(xm) << 48) | ((uint64_t)(dxmdy) << 0))
*/

void dp_set_color_image(uint32_t fmt, uint32_t size, uint32_t width, uint32_t dramadrs);
void dp_set_texture_image(uint32_t fmt, uint32_t size, uint32_t width, uint32_t dramadrs);

void _dp_set_other_modes(uint64_t params);
#define dp_set_other_modes_raw( \
		atomic_prim, \
		cycle_type, \
		persp_tex_en, \
		detail_tex_en, \
		sharpen_tex_en, \
		tex_lod_en, \
		en_tlut, \
		tlut_type, \
		sample_type, \
		mid_texel, \
		bi_lerp_0, \
		bi_lerp_1, \
		convert_one, \
		key_en, \
		rgb_dither_sel, \
		alpha_dither_sel, \
		b_m1a_0, \
		b_m1a_1, \
		b_m1b_0, \
		b_m1b_1, \
		b_m2a_0, \
		b_m2a_1, \
		b_m2b_0, \
		b_m2b_1, \
		force_blend, \
		alpha_cvg_select, \
		cvg_times_alpha, \
		z_mode, \
		cvg_dest, \
		color_on_cvg, \
		image_read_en, \
		z_update_en, \
		z_compare_en, \
		antialias_en, \
		z_source_sel, \
		dither_alpha_en, \
		alpha_compare_en) \
		_dp_set_other_modes( \
				((uint64_t)(atomic_prim) << 55) | \
				((uint64_t)(cycle_type) << 52) | \
				((uint64_t)(persp_tex_en) << 51) | \
				((uint64_t)(detail_tex_en) << 50) | \
				((uint64_t)(sharpen_tex_en) << 49) | \
				((uint64_t)(tex_lod_en) << 48) | \
				((uint64_t)(en_tlut) << 47) | \
				((uint64_t)(tlut_type) << 46) | \
				((uint64_t)(sample_type) << 45) | \
				((uint64_t)(mid_texel) << 44) | \
				((uint64_t)(bi_lerp_0) << 43) | \
				((uint64_t)(bi_lerp_1) << 42) | \
				((uint64_t)(convert_one) << 41) | \
				((uint64_t)(key_en) << 40) | \
				((uint64_t)(rgb_dither_sel) << 38) | \
				((uint64_t)(alpha_dither_sel) << 36) | \
				((uint64_t)(b_m1a_0) << 30) | \
				((uint64_t)(b_m1a_1) << 28) | \
				((uint64_t)(b_m1b_0) << 26) | \
				((uint64_t)(b_m1b_1) << 24) | \
				((uint64_t)(b_m2a_0) << 22) | \
				((uint64_t)(b_m2a_1) << 20) | \
				((uint64_t)(b_m2b_0) << 18) | \
				((uint64_t)(b_m2b_1) << 16) | \
				((uint64_t)(force_blend) << 14) | \
				((uint64_t)(alpha_cvg_select) << 13) | \
				((uint64_t)(cvg_times_alpha) << 12) | \
				((uint64_t)(z_mode) << 10) | \
				((uint64_t)(cvg_dest) << 8) | \
				((uint64_t)(color_on_cvg) << 7) | \
				((uint64_t)(image_read_en) << 6) | \
				((uint64_t)(z_update_en) << 5) | \
				((uint64_t)(z_compare_en) << 4) | \
				((uint64_t)(antialias_en) << 3) | \
				((uint64_t)(z_source_sel) << 2) | \
				((uint64_t)(dither_alpha_en) << 1) | \
				((uint64_t)(alpha_compare_en) << 0) )
#define dp_set_other_modes( \
		atomic_prim, \
		cycle_type, \
		persp_tex_en, \
		lod, \
		tlut, \
		filter, \
		convert, \
		key_en, \
		rgb_dither_sel, \
		alpha_dither_sel, \
		b_m1a_0, \
		b_m1a_1, \
		b_m1b_0, \
		b_m1b_1, \
		b_m2a_0, \
		b_m2a_1, \
		b_m2b_0, \
		b_m2b_1, \
		force_blend, \
		alpha_cvg_select, \
		cvg_times_alpha, \
		z_mode, \
		cvg_dest, \
		color_on_cvg, \
		image_read_en, \
		z_update_en, \
		z_compare_en, \
		antialias_en, \
		z_source_sel, \
		alpha_compare) \
		_dp_set_other_modes( \
				((uint64_t)(atomic_prim) << 55) | \
				((uint64_t)(cycle_type) << 52) | \
				((uint64_t)(persp_tex_en) << 51) | \
				((uint64_t)(lod) << 48) | \
				((uint64_t)(tlut) << 46) | \
				((uint64_t)(filter) << 44) | \
				((uint64_t)(convert) << 41) | \
				((uint64_t)(key_en) << 40) | \
				((uint64_t)(rgb_dither_sel) << 38) | \
				((uint64_t)(alpha_dither_sel) << 36) | \
				((uint64_t)(b_m1a_0) << 30) | \
				((uint64_t)(b_m1a_1) << 28) | \
				((uint64_t)(b_m1b_0) << 26) | \
				((uint64_t)(b_m1b_1) << 24) | \
				((uint64_t)(b_m2a_0) << 22) | \
				((uint64_t)(b_m2a_1) << 20) | \
				((uint64_t)(b_m2b_0) << 18) | \
				((uint64_t)(b_m2b_1) << 16) | \
				((uint64_t)(force_blend) << 14) | \
				((uint64_t)(alpha_cvg_select) << 13) | \
				((uint64_t)(cvg_times_alpha) << 12) | \
				((uint64_t)(z_mode) << 10) | \
				((uint64_t)(cvg_dest) << 8) | \
				((uint64_t)(color_on_cvg) << 7) | \
				((uint64_t)(image_read_en) << 6) | \
				((uint64_t)(z_update_en) << 5) | \
				((uint64_t)(z_compare_en) << 4) | \
				((uint64_t)(antialias_en) << 3) | \
				((uint64_t)(z_source_sel) << 2) | \
				((uint64_t)(alpha_compare) << 0) )

void _dp_set_combine_mode(uint64_t params);
#define dp_set_combine_mode( \
		color_a_0, \
		color_b_0, \
		color_c_0, \
		color_d_0, \
		alpha_a_0, \
		alpha_b_0, \
		alpha_c_0, \
		alpha_d_0, \
		color_a_1, \
		color_b_1, \
		color_c_1, \
		color_d_1, \
		alpha_a_1, \
		alpha_b_1, \
		alpha_c_1, \
		alpha_d_1) \
		_dp_set_combine_mode( \
				((uint64_t)(color_a_0) << 52) | \
				((uint64_t)(color_c_0) << 47) | \
				((uint64_t)(alpha_a_0) << 44) | \
				((uint64_t)(alpha_c_0) << 41) | \
				((uint64_t)(color_a_1) << 37) | \
				((uint64_t)(color_c_1) << 32) | \
				((uint64_t)(color_b_0) << 28) | \
				((uint64_t)(color_b_1) << 24) | \
				((uint64_t)(alpha_a_1) << 21) | \
				((uint64_t)(alpha_c_1) << 18) | \
				((uint64_t)(color_d_0) << 15) | \
				((uint64_t)(alpha_b_0) << 12) | \
				((uint64_t)(alpha_d_0) << 9) | \
				((uint64_t)(color_d_1) << 6) | \
				((uint64_t)(alpha_b_1) << 3) | \
				((uint64_t)(alpha_d_1) << 0) )
#define dp_set_combine_mode8(CA,CB,CC,CD,AA,AB,AC,AD) dp_set_combine_mode(CA,CB,CC,CD,AA,AB,AC,AD,CA,CB,CC,CD,AA,AB,AC,AD)
#define dp_set_combine_mode4(C0,A0,C1,A1) dp_set_combine_mode(C0,A0,C1,A1)
#define dp_set_combine_mode2(C,A) dp_set_combine_mode(C,A,C,A)

void dp_set_fill_color(uint32_t fillword);
#define dp_set_fill_color_16b(r,g,b,cvg) dp_set_fill_color( \
		((uint32_t)(r) << 27) | \
		((uint32_t)(g) << 22) | \
		((uint32_t)(b) << 17) | \
		((uint32_t)(cvg) << 16) | \
		((uint32_t)(r) << 11) | \
		((uint32_t)(g) << 6) | \
		((uint32_t)(b) << 1) | \
		((uint32_t)(cvg) << 0) )

void dp_set_prim_color_rgba(uint32_t rgba, uint32_t minlevel/*u0.5*/, uint32_t lodfrac/*u0.8*/);
#define dp_set_prim_color(r,g,b,a,ml,lf) dp_set_prim_color_rgba( \
		((uint32_t)(r) << 24) | \
		((uint32_t)(g) << 16) | \
		((uint32_t)(b) << 8) | \
		((uint32_t)(a) << 0), \
		ml, \
		lf)

void dp_set_prim_depth(uint16_t z, uint16_t dz);

void dp_fill_rectangle_10_raw(uint32_t xh, uint32_t yh, uint32_t xl, uint32_t yl);
void dp_fillrect(int32_t xh, int32_t yh, int32_t xl, int32_t yl);
void dp_texture_rectangle_10_raw(uint32_t xh, uint32_t yh, uint32_t xl, uint32_t yl, uint32_t tile, uint32_t s, uint32_t t, uint32_t dsdx, uint32_t dtdy);
void dp_texture_rectangle_flip_10_raw(uint32_t xh, uint32_t yh, uint32_t xl, uint32_t yl, uint32_t tile, uint32_t s, uint32_t t, uint32_t dsdx, uint32_t dtdy);
void dp_set_scissor_10(uint32_t xh, uint32_t yh, uint32_t xl, uint32_t yl, enum DP_SCISSOR_FIELD field);
void _dp_set_tile(uint64_t params);
// with width+size->line conversion.
// tmemaddr is 64bit word for RDP, 8bit for here to make easier.
//   but in 32b, 1 texel is 16bit-offset, so if you load 16x32 and use second 16x16, specify 16*16*2, not 16*16*4. (because of physical TMEM layout?)
// 32b's line is also special. (because of physical TMEM layout?)
#define dp_set_tile( \
		tile, \
		format, \
		size, \
		width, \
		tmemaddr, \
		palette, \
		flagss, \
		masks, \
		shifts, \
		flagst, \
		maskt, \
		shiftt) \
		_dp_set_tile( \
				((uint64_t)(format) << 53) | \
				((uint64_t)(size) << 51) | \
				((uint64_t)(((width) * DP_BIPP(((size) == DP_SIZE_32B) ? DP_SIZE_16B : (size)) + 63) / 64) << 41) | \
				((uint64_t)((tmemaddr) / 8) << 32) | \
				((uint64_t)(tile) << 24) | \
				((uint64_t)(palette) << 20) | \
				((uint64_t)(flagst) << 18) | \
				((uint64_t)(maskt) << 14) | \
				((uint64_t)(shiftt) << 10) | \
				((uint64_t)(flagss) << 8) | \
				((uint64_t)(masks) << 4) | \
				((uint64_t)(shifts) << 0) )

void dp_load_tile_10(uint32_t tile, uint32_t sl, uint32_t tl, uint32_t sh, uint32_t th);
void dp_load_block(uint32_t tile, uint32_t sl, uint32_t tl, uint32_t width, uint32_t height, enum DP_PIXEL_SIZE size);
void dp_set_tile_size_10(uint32_t tile, uint32_t sl, uint32_t tl, uint32_t sh, uint32_t th);
void dp_load_tlut_10(uint32_t tile, uint32_t sl, uint32_t tl, uint32_t sh, uint32_t th);
void dp_load_sync(void);
void dp_pipe_sync(void);
void dp_tile_sync(void);
void dp_full_sync(void);
