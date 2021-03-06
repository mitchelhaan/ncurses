/****************************************************************************
 * Copyright (c) 1998 Free Software Foundation, Inc.                        *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/

/****************************************************************************
 *  Author: Zeyd M. Ben-Halim <zmbenhal@netcom.com> 1992,1995               *
 *     and: Eric S. Raymond <esr@snark.thyrsus.com>                         *
 ****************************************************************************/

/* lib_color.c
 *
 * Handles color emulation of SYS V curses
 *
 */

#include <curses.priv.h>

#include <term.h>

MODULE_ID("$Id: lib_color.c,v 1.24 1998/02/11 12:13:58 tom Exp $")

/*
 * Only 8 ANSI colors are defined; the ISO 6429 control sequences work only
 * for 8 values (0-7).
 */
#define MAX_ANSI_COLOR 8

/*
 * These should be screen structure members.  They need to be globals for
 * hystorical reasons.  So we assign them in start_color() and also in
 * set_term()'s screen-switching logic.
 */
int COLOR_PAIRS;
int COLORS;

/*
 * Given a RGB range of 0..1000, we'll normally set the individual values
 * to about 2/3 of the maximum, leaving full-range for bold/bright colors.
 */
#define RGB_ON  680
#define RGB_OFF 0

static const color_t cga_palette[] =
{
    /*  R		G		B */
	{RGB_OFF,	RGB_OFF,	RGB_OFF},	/* COLOR_BLACK */
	{RGB_ON,	RGB_OFF,	RGB_OFF},	/* COLOR_RED */
	{RGB_OFF,	RGB_ON,		RGB_OFF},	/* COLOR_GREEN */
	{RGB_ON,	RGB_ON,		RGB_OFF},	/* COLOR_YELLOW */
	{RGB_OFF,	RGB_OFF,	RGB_ON},	/* COLOR_BLUE */
	{RGB_ON,	RGB_OFF,	RGB_ON},	/* COLOR_MAGENTA */
	{RGB_OFF,	RGB_ON,		RGB_ON},	/* COLOR_CYAN */
	{RGB_ON,	RGB_ON,		RGB_ON},	/* COLOR_WHITE */
};
static const color_t hls_palette[] =
{
    /*  H	L	S */
	{0,	0,	0},	/* COLOR_BLACK */
	{120,	50,	100},	/* COLOR_RED */
	{240,	50,	100},	/* COLOR_GREEN */
	{180,	50,	100},	/* COLOR_YELLOW */
	{330,	50,	100},	/* COLOR_BLUE */
	{60,	50,	100},	/* COLOR_MAGENTA */
	{300,	50,	100},	/* COLOR_CYAN */
	{0,	50,	100},	/* COLOR_WHITE */
};

int start_color(void)
{
	T((T_CALLED("start_color()")));

#ifdef orig_pair
	if (orig_pair != NULL)
	{
		TPUTS_TRACE("orig_pair");
		putp(orig_pair);
	}
#endif /* orig_pair */
#ifdef orig_colors
	if (orig_colors != NULL)
	{
		TPUTS_TRACE("orig_colors");
		putp(orig_colors);
	}
#endif /* orig_colors */
#if defined(orig_pair) && defined(orig_colors)
	if (!orig_pair && !orig_colors)
		returnCode(ERR);
#endif /* defined(orig_pair) && defined(orig_colors) */
	if (max_pairs != -1)
		COLOR_PAIRS = SP->_pair_count = max_pairs;
	else
		returnCode(ERR);
	SP->_color_pairs = typeCalloc(unsigned short, max_pairs);
	SP->_color_pairs[0] = PAIR_OF(COLOR_WHITE, COLOR_BLACK);
	if (max_colors != -1)
		COLORS = SP->_color_count = max_colors;
	else
		returnCode(ERR);
	SP->_coloron = 1;

	SP->_color_table = malloc(sizeof(color_t) * COLORS);
#ifdef hue_lightness_saturation
	if (hue_lightness_saturation)
	    memcpy(SP->_color_table, hls_palette, sizeof(color_t) * COLORS);
	else
#endif /* hue_lightness_saturation */
	    memcpy(SP->_color_table, cga_palette, sizeof(color_t) * COLORS);

	if (orig_colors)
	{
	    TPUTS_TRACE("orig_colors");
	    putp(orig_colors);
	}

	T(("started color: COLORS = %d, COLOR_PAIRS = %d", COLORS, COLOR_PAIRS));

	returnCode(OK);
}

#ifdef hue_lightness_saturation
/* This function was originally written by Daniel Weaver <danw@znyx.com> */
static void rgb2hls(short r, short g, short b, short *h, short *l, short *s)
/* convert RGB to HLS system */
{
    short min, max, t;
    
    if ((min = g < r ? g : r) > b) min = b;
    if ((max = g > r ? g : r) < b) max = b;

    /* calculate lightness */
    *l = (min + max) / 20;

    if (min == max)		/* black, white and all shades of gray */
    {
	*h = 0;
	*s = 0;
	return;
    }

    /* calculate saturation */
    if (*l < 50)
	*s = ((max - min) * 100) / (max + min);
    else *s = ((max - min) * 100) / (2000 - max - min);

    /* calculate hue */
    if (r == max)
	t = 120 + ((g - b) * 60) / (max - min);
    else
	if (g == max)
	    t = 240 + ((b - r) * 60) / (max - min);
	else
	    t = 360 + ((r - g) * 60) / (max - min);

    *h = t % 360;
}
#endif /* hue_lightness_saturation */

/*
 * Extension (1997/1/18) - Allow negative f/b values to set default color
 * values.
 */
int init_pair(short pair, short f, short b)
{
	T((T_CALLED("init_pair(%d,%d,%d)"), pair, f, b));

	if ((pair < 1) || (pair >= COLOR_PAIRS))
		returnCode(ERR);
	if (SP->_default_color)
	{
		if (f < 0)
			f = C_MASK;
		if (b < 0)
			b = C_MASK;
		if (f >= COLORS && f != C_MASK)
			returnCode(ERR);
		if (b >= COLORS && b != C_MASK)
			returnCode(ERR);
	}
	else
	if ((f < 0) || (f >= COLORS)
	 || (b < 0) || (b >= COLORS))
		returnCode(ERR);

	/*
	 * FIXME: when a pair's content is changed, replace its colors
	 * (if pair was initialized before a screen update is performed
	 * replacing original pair colors with the new ones)
	 */

	SP->_color_pairs[pair] = PAIR_OF(f,b);

	if (initialize_pair)
	{
	    const color_t	*tp = hue_lightness_saturation ? hls_palette : cga_palette;

	    T(("initializing pair: pair = %d, fg=(%d,%d,%d), bg=(%d,%d,%d)",
	       pair,
	       tp[f].red, tp[f].green, tp[f].blue,
	       tp[b].red, tp[b].green, tp[b].blue));

	    if (initialize_pair)
	    {
		TPUTS_TRACE("initialize_pair");
		putp(tparm(initialize_pair,
			    pair,
			    tp[f].red, tp[f].green, tp[f].blue,
			    tp[b].red, tp[b].green, tp[b].blue));
	    }
	}

	returnCode(OK);
}

int init_color(short color, short r, short g, short b)
{
	T((T_CALLED("init_color(%d,%d,%d,%d)"), color, r, g, b));
#ifdef initialize_color
	if (initialize_color == NULL)
		returnCode(ERR);
#endif /* initialize_color */

	if (color < 0 || color >= COLORS)
		returnCode(ERR);
	if (r < 0 || r > 1000 || g < 0 ||  g > 1000 || b < 0 || b > 1000)
		returnCode(ERR);

#ifdef hue_lightness_saturation
	if (hue_lightness_saturation)
	    rgb2hls(r, g, b,
		      &SP->_color_table[color].red,
		      &SP->_color_table[color].green,
		      &SP->_color_table[color].blue);
	else
#endif /* hue_lightness_saturation */
	{
		SP->_color_table[color].red = r;
		SP->_color_table[color].green = g;
		SP->_color_table[color].blue = b;
	}

#ifdef initialize_color
	if (initialize_color)
	{
		TPUTS_TRACE("initialize_color");
		putp(tparm(initialize_color, color, r, g, b));
	}
#endif /* initialize_color */
	returnCode(OK);
}

bool can_change_color(void)
{
	T((T_CALLED("can_change_color()")));
	returnCode ((can_change != 0) ? TRUE : FALSE);
}

bool has_colors(void)
{
	T((T_CALLED("has_colors()")));
	returnCode (((orig_pair != NULL || orig_colors != NULL)
		     && (max_colors != -1) && (max_pairs != -1)
		     && (((set_foreground != NULL)
			  && (set_background != NULL))
			 || ((set_a_foreground != NULL)
			     && (set_a_background != NULL))
			 || set_color_pair)) ? TRUE : FALSE);
}

int color_content(short color, short *r, short *g, short *b)
{
    T((T_CALLED("color_content(%d,%p,%p,%p)"), color, r, g, b));
    if (color < 0 || color > COLORS)
	returnCode(ERR);

    if (r) *r = SP->_color_table[color].red;
    if (g) *g = SP->_color_table[color].green;
    if (b) *b = SP->_color_table[color].blue;
    returnCode(OK);
}

int pair_content(short pair, short *f, short *b)
{
	T((T_CALLED("pair_content(%d,%p,%p)"), pair, f, b));

	if ((pair < 0) || (pair > COLOR_PAIRS))
		returnCode(ERR);
	if (f) *f = ((SP->_color_pairs[pair] >> C_SHIFT) & C_MASK);
	if (b) *b =  (SP->_color_pairs[pair] & C_MASK);

	returnCode(OK);
}

/*
 * SVr4 curses is known to interchange color codes (1,4) and (3,6), possibly
 * to maintain compatibility with a pre-ANSI scheme.  The same scheme is
 * also used in the FreeBSD syscons.
 */
static int toggled_colors(int c)
{
    if (c < 16) {
	static const int table[] =
		{ 0,  4,  2,  6,  1,  5,  3,  7,
		  8, 12, 10, 14,  9, 13, 11, 15};
	c = table[c];
    }
    return c;
}

void _nc_do_color(int pair, bool reverse, int  (*outc)(int))
{
    short fg, bg;

    if (reverse)
    	pair = -pair;

    if (pair == 0)
    {
	if (orig_pair)
	{
	    TPUTS_TRACE("orig_pair");
	    tputs(orig_pair, 1, outc);
	}
    }
    else
    {
	if (set_color_pair)
	{
	    TPUTS_TRACE("set_color_pair");
	    tputs(tparm(set_color_pair, pair), 1, outc);
	}
	else
	{
	    pair_content(pair, &fg, &bg);
	    if (reverse) {
		short xx = fg;
		fg = bg;
		bg = xx;
	    }

	    T(("setting colors: pair = %d, fg = %d, bg = %d", pair, fg, bg));

	    if (fg == C_MASK || bg == C_MASK)
	    {
		if (orig_pair)
		{
		    TPUTS_TRACE("orig_pair");
		    tputs(orig_pair, 1, outc);
		}
		else
		{
		    TPUTS_TRACE("orig_colors");
		    tputs(orig_colors, 1, outc);
		}
	    }
	    if (fg != C_MASK)
	    {
		if (set_a_foreground && fg <= MAX_ANSI_COLOR)
		{
		    TPUTS_TRACE("set_a_foreground");
		    tputs(tparm(set_a_foreground, fg), 1, outc);
		}
		else
		{
		    TPUTS_TRACE("set_foreground");
		    tputs(tparm(set_foreground, toggled_colors(fg)), 1, outc);
		}
	    }
	    if (bg != C_MASK)
	    {
		if (set_a_background && bg <= MAX_ANSI_COLOR)
		{
		    TPUTS_TRACE("set_a_background");
		    tputs(tparm(set_a_background, bg), 1, outc);
		}
		else
		{
		    TPUTS_TRACE("set_background");
		    tputs(tparm(set_background, toggled_colors(bg)), 1, outc);
		}
	    }
	}
    }
}
