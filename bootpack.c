#include "bootpack.h"

extern struct KEYBUF keybuf;
void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)0xff0;
    extern char hankaku[4096];
    char s[40], mcursor[256];

    init_gdtidt();
    init_pic();
    io_sti();

    io_out8(PIC0_IMR, 0xf9);
    io_out8(PIC1_IMR, 0xef);

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

    putfont8_asc(binfo->vram, binfo->scrnx, 8, 8, COL8_FFFFFF, "WDK");
    putfont8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, "oreore OS");

    init_mouse_cursor8(mcursor, COL8_008400);
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;
    sprintf(s, "(%d, %d)", mx, my);
    putfont8_asc(binfo->vram, binfo->scrnx, 16, 64, COL8_FFFFFF, s);

    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);

    int i;
    for (;;)
    {
        io_cli();
        if (keybuf.next == 0)
        {
            io_stihlt();
        }
        else
        {
            i = keybuf.data[0];
            keybuf.next--;
            for (int j = 0; j < keybuf.next; j++)
            {
                keybuf.data[j] = keybuf.data[j + 1];
            }
            io_sti();
            sprintf(s, "%x", i);
            boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
            putfont8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
        }
    }
}
