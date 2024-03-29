#include "tilda.h"
#include "tilda-palettes.h"

#include <glib/gi18n.h>

static GdkRGBA tilda_palettes_current_palette[TILDA_COLOR_PALETTE_SIZE];

static const GdkRGBA
        terminal_palette_tango[TILDA_COLOR_PALETTE_SIZE] = {
        { RGB(0x2e2e, 0x3434, 0x3636) }, //1. black
        { RGB(0xcccc, 0x0000, 0x0000) }, //2. darkred
        { RGB(0x4e4e, 0x9a9a, 0x0606) }, //3. darkgreen
        { RGB(0xc4c4, 0xa0a0, 0x0000) }, //4. brown
        { RGB(0x3434, 0x6565, 0xa4a4) }, //5. darkblue
        { RGB(0x7575, 0x5050, 0x7b7b) }, //6. darkmagenta
        { RGB(0x0606, 0x9898, 0x9a9a) }, //7. darkcyan
        { RGB(0xd3d3, 0xd7d7, 0xcfcf) }, //8. lightgrey
        { RGB(0x5555, 0x5757, 0x5353) }, //9. darkgrey
        { RGB(0xefef, 0x2929, 0x2929) }, //10. red
        { RGB(0x8a8a, 0xe2e2, 0x3434) }, //11. green
        { RGB(0xfcfc, 0xe9e9, 0x4f4f) }, //12. yellow
        { RGB(0x7272, 0x9f9f, 0xcfcf) }, //13. blue
        { RGB(0xadad, 0x7f7f, 0xa8a8) }, //14. magenta
        { RGB(0x3434, 0xe2e2, 0xe2e2) }, //15. cyan
        { RGB(0xeeee, 0xeeee, 0xecec) }  //16. white
};

static const GdkRGBA
        terminal_palette_zenburn[TILDA_COLOR_PALETTE_SIZE] = {
        { RGB(0x2222, 0x2222, 0x2222) }, //black
        { RGB(0x8080, 0x3232, 0x3232) }, //darkred
        { RGB(0x5b5b, 0x7676, 0x2f2f) }, //darkgreen
        { RGB(0xaaaa, 0x9999, 0x4343) }, //brown
        { RGB(0x3232, 0x4c4c, 0x8080) }, //darkblue
        { RGB(0x7070, 0x6c6c, 0x9a9a) }, //darkmagenta
        { RGB(0x9292, 0xb1b1, 0x9e9e) }, //darkcyan
        { RGB(0xffff, 0xffff, 0xffff) }, //lightgrey
        { RGB(0x2222, 0x2222, 0x2222) }, //darkgrey
        { RGB(0x9898, 0x2b2b, 0x2b2b) }, //red
        { RGB(0x8989, 0xb8b8, 0x3f3f) }, //green
        { RGB(0xefef, 0xefef, 0x6060) }, //yellow
        { RGB(0x2b2b, 0x4f4f, 0x9898) }, //blue
        { RGB(0x8282, 0x6a6a, 0xb1b1) }, //magenta
        { RGB(0xa1a1, 0xcdcd, 0xcdcd) }, //cyan
        { RGB(0xdede, 0xdede, 0xdede) }, //white}
};

static const GdkRGBA
        terminal_palette_linux[TILDA_COLOR_PALETTE_SIZE] = {
        { RGB(0x0000, 0x0000, 0x0000) },
        { RGB(0xaaaa, 0x0000, 0x0000) },
        { RGB(0x0000, 0xaaaa, 0x0000) },
        { RGB(0xaaaa, 0x5555, 0x0000) },
        { RGB(0x0000, 0x0000, 0xaaaa) },
        { RGB(0xaaaa, 0x0000, 0xaaaa) },
        { RGB(0x0000, 0xaaaa, 0xaaaa) },
        { RGB(0xaaaa, 0xaaaa, 0xaaaa) },
        { RGB(0x5555, 0x5555, 0x5555) },
        { RGB(0xffff, 0x5555, 0x5555) },
        { RGB(0x5555, 0xffff, 0x5555) },
        { RGB(0xffff, 0xffff, 0x5555) },
        { RGB(0x5555, 0x5555, 0xffff) },
        { RGB(0xffff, 0x5555, 0xffff) },
        { RGB(0x5555, 0xffff, 0xffff) },
        { RGB(0xffff, 0xffff, 0xffff) }
};

static const GdkRGBA
        terminal_palette_xterm[TILDA_COLOR_PALETTE_SIZE] = {
        {RGB(0x0000, 0x0000, 0x0000) },
        {RGB(0xcdcb, 0x0000, 0x0000) },
        {RGB(0x0000, 0xcdcb, 0x0000) },
        {RGB(0xcdcb, 0xcdcb, 0x0000) },
        {RGB(0x1e1a, 0x908f, 0xffff) },
        {RGB(0xcdcb, 0x0000, 0xcdcb) },
        {RGB(0x0000, 0xcdcb, 0xcdcb) },
        {RGB(0xe5e2, 0xe5e2, 0xe5e2) },
        {RGB(0x4ccc, 0x4ccc, 0x4ccc) },
        {RGB(0xffff, 0x0000, 0x0000) },
        {RGB(0x0000, 0xffff, 0x0000) },
        {RGB(0xffff, 0xffff, 0x0000) },
        {RGB(0x4645, 0x8281, 0xb4ae) },
        {RGB(0xffff, 0x0000, 0xffff) },
        {RGB(0x0000, 0xffff, 0xffff) },
        {RGB(0xffff, 0xffff, 0xffff) }
};

static const GdkRGBA
        terminal_palette_rxvt[TILDA_COLOR_PALETTE_SIZE] = {
        { RGB(0x0000, 0x0000, 0x0000) },
        { RGB(0xcdcd, 0x0000, 0x0000) },
        { RGB(0x0000, 0xcdcd, 0x0000) },
        { RGB(0xcdcd, 0xcdcd, 0x0000) },
        { RGB(0x0000, 0x0000, 0xcdcd) },
        { RGB(0xcdcd, 0x0000, 0xcdcd) },
        { RGB(0x0000, 0xcdcd, 0xcdcd) },
        { RGB(0xfafa, 0xebeb, 0xd7d7) },
        { RGB(0x4040, 0x4040, 0x4040) },
        { RGB(0xffff, 0x0000, 0x0000) },
        { RGB(0x0000, 0xffff, 0x0000) },
        { RGB(0xffff, 0xffff, 0x0000) },
        { RGB(0x0000, 0x0000, 0xffff) },
        { RGB(0xffff, 0x0000, 0xffff) },
        { RGB(0x0000, 0xffff, 0xffff) },
        { RGB(0xffff, 0xffff, 0xffff) }
};

static const GdkRGBA
        terminal_palette_solarizedL[TILDA_COLOR_PALETTE_SIZE] = {
        { RGB(0xeeee, 0xe8e8, 0xd5d5) },
        { RGB(0xdcdc, 0x3232, 0x2f2f) },
        { RGB(0x8585, 0x9999, 0x0000) },
        { RGB(0xb5b5, 0x8989, 0x0000) },
        { RGB(0x2626, 0x8b8b, 0xd2d2) },
        { RGB(0xd3d3, 0x3636, 0x8282) },
        { RGB(0x2a2a, 0xa1a1, 0x9898) },
        { RGB(0x0707, 0x3636, 0x4242) },
        { RGB(0xfdfd, 0xf6f6, 0xe3e3) },
        { RGB(0xcbcb, 0x4b4b, 0x1616) },
        { RGB(0x9393, 0xa1a1, 0xa1a1) },
        { RGB(0x8383, 0x9494, 0x9696) },
        { RGB(0x6565, 0x7b7b, 0x8383) },
        { RGB(0x6c6c, 0x7171, 0xc4c4) },
        { RGB(0x5858, 0x6e6e, 0x7575) },
        { RGB(0x0000, 0x2b2b, 0x3636) }
};

static const GdkRGBA
        terminal_palette_solarizedD[TILDA_COLOR_PALETTE_SIZE] = {
        { RGB(0x0707, 0x3636, 0x4242) },
        { RGB(0xdcdc, 0x3232, 0x2f2f) },
        { RGB(0x8585, 0x9999, 0x0000) },
        { RGB(0xb5b5, 0x8989, 0x0000) },
        { RGB(0x2626, 0x8b8b, 0xd2d2) },
        { RGB(0xd3d3, 0x3636, 0x8282) },
        { RGB(0x2a2a, 0xa1a1, 0x9898) },
        { RGB(0xeeee, 0xe8e8, 0xd5d5) },
        { RGB(0x0000, 0x2b2b, 0x3636) },
        { RGB(0xcbcb, 0x4b4b, 0x1616) },
        { RGB(0x5858, 0x6e6e, 0x7575) },
        { RGB(0x8383, 0x9494, 0x9696) },
        { RGB(0x6565, 0x7b7b, 0x8383) },
        { RGB(0x6c6c, 0x7171, 0xc4c4) },
        { RGB(0x9393, 0xa1a1, 0xa1a1) },
        { RGB(0xfdfd, 0xf6f6, 0xe3e3) }
};

static const GdkRGBA
        terminal_palette_snazzy[TILDA_COLOR_PALETTE_SIZE] = {
        { RGB(0x2828, 0x2a2a, 0x3636) },
        { RGB(0xffff, 0x5c5c, 0x5757) },
        { RGB(0x5a5a, 0xf7f7, 0x8e8e) },
        { RGB(0xf3f3, 0xf9f9, 0x9d9d) },
        { RGB(0x5757, 0xc7c7, 0xffff) },
        { RGB(0xffff, 0x6a6a, 0xc1c1) },
        { RGB(0x9a9a, 0xeded, 0xfefe) },
        { RGB(0xf1f1, 0xf1f1, 0xf0f0) },
        { RGB(0x6868, 0x6868, 0x6868) },
        { RGB(0xffff, 0x5c5c, 0x5757) },
        { RGB(0x5a5a, 0xf7f7, 0x8e8e) },
        { RGB(0xf3f3, 0xf9f9, 0x9d9d) },
        { RGB(0x5757, 0xc7c7, 0xffff) },
        { RGB(0xffff, 0x6a6a, 0xc1c1) },
        { RGB(0x9a9a, 0xeded, 0xfefe) },
        { RGB(0xf1f1, 0xf1f1, 0xf0f0) }
};

static TildaColorScheme palette_schemes[] = {
        { N_("Custom"), NULL },
        { N_("Tango"), terminal_palette_tango },
        { N_("Linux console"), terminal_palette_linux },
        { N_("XTerm"), terminal_palette_xterm },
        { N_("Rxvt"), terminal_palette_rxvt },
        { N_("Zenburn"), terminal_palette_zenburn },
        { N_("Solarized Light (deprecated)"), terminal_palette_solarizedL },
        { N_("Solarized"), terminal_palette_solarizedD },
        { N_("Snazzy"), terminal_palette_snazzy }
};

gint tilda_palettes_get_n_palette_schemes ()
{
    return G_N_ELEMENTS (palette_schemes);
}

TildaColorScheme * tilda_palettes_get_palette_schemes ()
{
    return palette_schemes;
}

void tilda_palettes_set_current_palette (const GdkRGBA *palette)
{
    memcpy (tilda_palettes_current_palette, palette,
            TILDA_COLOR_PALETTE_SIZE);
}

GdkRGBA *tilda_palettes_get_current_palette ()
{
    return tilda_palettes_current_palette;
}

const GdkRGBA *tilda_palettes_get_palette_color (const GdkRGBA *palette,
                                                 int color_num)
{
    g_return_val_if_fail (color_num >= 0 && color_num < TILDA_COLOR_PALETTE_SIZE,
                          NULL);

    return palette + color_num;
}
