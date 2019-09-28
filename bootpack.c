#include "bootpack.h"

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)0xff0;
    extern char hankaku[4096];
    char s[40], mcursor[256];

    init_gdtidt();
    init_pic();
    io_sti();

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

    putfont8_asc(binfo->vram, binfo->scrnx, 8, 8, COL8_FFFFFF, "WDK");
    putfont8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_000000, "oreore OS");
    putfont8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, "oreore OS");

    init_mouse_cursor8(mcursor, COL8_008400);
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;
    sprintf(s, "(%d, %d)", mx, my);
    putfont8_asc(binfo->vram, binfo->scrnx, 16, 64, COL8_FFFFFF, s);
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

    io_out8(PIC0_IMR, 0xf9);
    io_out8(PIC1_IMR, 0xef);

    for (;;)
    {
        io_hlt();
    }
}
