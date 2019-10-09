#include "bootpack.h"

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], keybuf[32], mousebuf[128];

    init_gdtidt();
    init_pic();
    io_sti(); /* IDTとPICの初期化が終わったのでCPUの割り込み禁止を解除 */

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);

    io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可 */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可 */

    init_keyboard();

    struct MOUSE_DEC mdec;
    enable_mouse(&mdec);

    struct MEMMAM *memman = (struct MEMMAN *)MEM_ADDR;
    unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free_4k(memman, 0x00001000, 0x0009e000);
    memman_free_4k(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();

    struct SHTCTL *shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    struct SHEET *sht_back = sheet_alloc(shtctl);
    struct SHEET *sht_mouse = sheet_alloc(shtctl);
    unsigned char *buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);

    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    unsigned char buf_mouse[256];
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);

    init_screen(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);
    sheet_slide(shtctl, sht_back, 0, 0);
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(shtctl, sht_mouse, mx, my);
    sheet_updown(shtctl, sht_back, 0);
    sheet_updown(shtctl, sht_mouse, 1);

    sprintf(s, "mouse (%d, %d)", mx, my);
    putfont8_asc(buf_back, binfo->scrnx, 0, 0, WHITE, s);
    sprintf(s, "memory = %dMB , free = %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfont8_asc(buf_back, binfo->scrnx, 0, 50, WHITE, s);

    sheet_refresh(shtctl, sht_back, 0, 0, binfo->scrnx, 66);

    int i;
    for (;;)
    {
        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0)
        {
            io_stihlt();
        }
        else
        {
            if (fifo8_status(&keyfifo) != 0)
            {
                i = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "keycode %x", i);
                boxfill8(buf_back, binfo->scrnx, COL8_008400, 0, 16, 15 * 8 - 1, 31);
                putfont8_asc(buf_back, binfo->scrnx, 0, 16, WHITE, s);
                sheet_refresh(shtctl, sht_back, 0, 16, 15 * 8 - 1, 32);
            }
            else if (fifo8_status(&mousefifo) != 0)
            {
                i = fifo8_get(&mousefifo);
                io_sti();
                if (mouse_decode(&mdec, i) != 0)
                {
                    sprintf(s, "[lcr %d %d]", mdec.x, mdec.y);
                    if ((mdec.btn & 0x01) != 0)
                    {
                        s[1] = 'L';
                    }
                    if ((mdec.btn & 0x02) != 0)
                    {
                        s[3] = 'R';
                    }
                    if ((mdec.btn & 0x04) != 0)
                    {
                        s[2] = 'C';
                    }
                    boxfill8(buf_back, binfo->scrnx, COL8_008400, 32, 32, 32 + 15 * 8 - 1, 47);
                    putfont8_asc(buf_back, binfo->scrnx, 32, 32, WHITE, s);
                    sheet_refresh(shtctl, sht_back, 32, 32, 32 + 15 * 8, 48);
                    /* マウスカーソルの移動 */
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0)
                    {
                        mx = 0;
                    }
                    if (my < 0)
                    {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 16)
                    {
                        mx = binfo->scrnx - 16;
                    }
                    if (my > binfo->scrny - 16)
                    {
                        my = binfo->scrny - 16;
                    }
                    sprintf(s, "mouse (%d %d)", mx, my);
                    boxfill8(buf_back, binfo->scrnx, COL8_008400, 0, 0, 15 * 8 - 1, 15); /* 座標消す */
                    putfont8_asc(buf_back, binfo->scrnx, 0, 0, WHITE, s);                /* 座標書く */
                    // putblock8_8(buf_back, binfo->scrnx, 16, 16, mx, my, buf_mouse, 16);
                    sheet_refresh(shtctl, sht_back, 0, 0, 15 * 8 - 1, 16);
                    sheet_slide(shtctl, sht_mouse, mx, my); /* refresh 含む */
                }
            }
        }
    }
}
