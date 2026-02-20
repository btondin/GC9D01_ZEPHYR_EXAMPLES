/*
 * Tiny 5px bitmap font for LVGL
 * 3x5 pixel glyphs, 1bpp, monospace (4px advance)
 * Supports: space - . 0-9 : G X Y Z
 *
 * Glyph order MUST match unicode_list sort order:
 *   ' '(0x20) '-'(0x2D) '.'(0x2E) '0'-'9'(0x30-0x39) ':'(0x3A) 'G'(0x47) 'X'(0x58) 'Y'(0x59) 'Z'(0x5A)
 *
 * Bitmap: 3 cols x 5 rows, MSB-first, packed (stride=0).
 * 15 bits per glyph = 2 bytes. byte0=bits[14:7], byte1=bits[6:0]<<1
 */

#include <lvgl.h>

static const uint8_t glyph_bitmap[] = {
	/* [0] ' ' - empty placeholder */
	0x00,

	/* [1] '-'   row0-4: 000 000 111 000 000
	 * 00000011 10000000 = 0x03 0x80 */
	0x03, 0x80,

	/* [3] '.'   row0-4: 000 000 000 000 010
	 * 00000000 00000100 = 0x00 0x04 */
	0x00, 0x04,

	/* [5] '0'   111 101 101 101 111
	 * 11110110 11011110 = 0xF6 0xDE */
	0xF6, 0xDE,

	/* [7] '1'   010 110 010 010 111
	 * 01011001 00101110 = 0x59 0x2E */
	0x59, 0x2E,

	/* [9] '2'   111 001 111 100 111
	 * 11100111 11001110 = 0xE7 0xCE */
	0xE7, 0xCE,

	/* [11] '3'  111 001 111 001 111
	 * 11100111 10011110 = 0xE7 0x9E */
	0xE7, 0x9E,

	/* [13] '4'  101 101 111 001 001
	 * 10110111 10010010 = 0xB7 0x92 */
	0xB7, 0x92,

	/* [15] '5'  111 100 111 001 111
	 * 11110011 10011110 = 0xF3 0x9E */
	0xF3, 0x9E,

	/* [17] '6'  111 100 111 101 111
	 * 11110011 11011110 = 0xF3 0xDE */
	0xF3, 0xDE,

	/* [19] '7'  111 001 001 001 001
	 * 11100100 10010010 = 0xE4 0x92 */
	0xE4, 0x92,

	/* [21] '8'  111 101 111 101 111
	 * 11110111 11011110 = 0xF7 0xDE */
	0xF7, 0xDE,

	/* [23] '9'  111 101 111 001 111
	 * 11110111 10011110 = 0xF7 0x9E */
	0xF7, 0x9E,

	/* [25] ':'  000 010 000 010 000
	 * 00001000 00100000 = 0x08 0x20 */
	0x08, 0x20,

	/* [27] 'G'  111 100 101 101 111
	 * 11110010 11011110 = 0xF2 0xDE */
	0xF2, 0xDE,

	/* [29] 'X'  101 101 010 101 101
	 * 10110101 01011010 = 0xB5 0x5A */
	0xB5, 0x5A,

	/* [31] 'Y'  101 101 010 010 010
	 * 10110101 00100100 = 0xB5 0x24 */
	0xB5, 0x24,

	/* [33] 'Z'  111 001 010 100 111
	 * 11100101 01001110 = 0xE5 0x4E */
	0xE5, 0x4E,
};

/*
 * Glyph descriptors - order matches unicode_list (sorted by codepoint).
 * adv_w = 4px * 16 = 64 (monospace). ofs_y=1 (1px from bottom baseline).
 */
static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
	{.bitmap_index = 0,  .adv_w = 0,  .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0}, /* reserved */
	{.bitmap_index = 0,  .adv_w = 48, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 6}, /* ' '  */
	{.bitmap_index = 1,  .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '-'  */
	{.bitmap_index = 3,  .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '.'  */
	{.bitmap_index = 5,  .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '0'  */
	{.bitmap_index = 7,  .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '1'  */
	{.bitmap_index = 9,  .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '2'  */
	{.bitmap_index = 11, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '3'  */
	{.bitmap_index = 13, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '4'  */
	{.bitmap_index = 15, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '5'  */
	{.bitmap_index = 17, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '6'  */
	{.bitmap_index = 19, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '7'  */
	{.bitmap_index = 21, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '8'  */
	{.bitmap_index = 23, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* '9'  */
	{.bitmap_index = 25, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* ':'  */
	{.bitmap_index = 27, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* 'G'  */
	{.bitmap_index = 29, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* 'X'  */
	{.bitmap_index = 31, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* 'Y'  */
	{.bitmap_index = 33, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 1}, /* 'Z'  */
};

/* Sorted by codepoint: ' ' '-' '.' '0'-'9' ':' 'G' 'X' 'Y' 'Z' */
static const uint16_t unicode_list[] = {
	0x0000, /* ' '  U+0020 */
	0x000D, /* '-'  U+002D */
	0x000E, /* '.'  U+002E */
	0x0010, /* '0'  U+0030 */
	0x0011, /* '1'  U+0031 */
	0x0012, /* '2'  U+0032 */
	0x0013, /* '3'  U+0033 */
	0x0014, /* '4'  U+0034 */
	0x0015, /* '5'  U+0035 */
	0x0016, /* '6'  U+0036 */
	0x0017, /* '7'  U+0037 */
	0x0018, /* '8'  U+0038 */
	0x0019, /* '9'  U+0039 */
	0x001A, /* ':'  U+003A */
	0x0027, /* 'G'  U+0047 */
	0x0038, /* 'X'  U+0058 */
	0x0039, /* 'Y'  U+0059 */
	0x003A, /* 'Z'  U+005A */
};

static const lv_font_fmt_txt_cmap_t cmaps[] = {
	{
		.range_start       = 0x0020,
		.range_length      = 0x003B,
		.glyph_id_start    = 1,
		.unicode_list      = unicode_list,
		.glyph_id_ofs_list = NULL,
		.list_length       = 18,
		.type              = LV_FONT_FMT_TXT_CMAP_SPARSE_TINY,
	}
};

static const lv_font_fmt_txt_dsc_t font_dsc = {
	.glyph_bitmap  = glyph_bitmap,
	.glyph_dsc     = glyph_dsc,
	.cmaps         = cmaps,
	.kern_dsc      = NULL,
	.kern_scale    = 0,
	.cmap_num      = 1,
	.bpp           = 1,
	.kern_classes  = 0,
	.bitmap_format = 0,
};

const lv_font_t lv_font_tiny5 = {
	.get_glyph_dsc    = lv_font_get_glyph_dsc_fmt_txt,
	.get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,
	.line_height      = 6,
	.base_line        = 0,
	.subpx            = LV_FONT_SUBPX_NONE,
	.underline_position  = 0,
	.underline_thickness = 0,
	.dsc              = &font_dsc,
};
