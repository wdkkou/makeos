#include "bootpack.h"

void make_window8(unsigned char *buf, int xsize, int ysize, char *title);
void putfont8_asc_sht(struct SHEET *sht, int x, int y,
                      int c, int b, char *s, int l);
void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40], keybuf[32], mousebuf[128];

    init_gdtidt();
    init_pic();
    io_sti(); /* IDTとPICの初期化が終わったのでCPUの割り込み禁止を解除 */

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);

    init_pit();

    io_out8(PIC0_IMR, 0xf8); /* PITとPIC1とキーボードを許可 */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可 */

    struct TIMER *timer1, *timer2, *timer3;
    struct FIFO8 timerfifo;
    char timerbuf[8];

    fifo8_init(&timerfifo, 8, timerbuf);
    timer1 = timer_alloc();
    timer_init(timer1, &timerfifo, 10);
    timer_settime(timer1, 1000);

    timer2 = timer_alloc();
    timer_init(timer2, &timerfifo, 3);
    timer_settime(timer2, 300);

    timer3 = timer_alloc();
    timer_init(timer3, &timerfifo, 1);
    timer_settime(timer3, 50);

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
    struct SHEET *sht_window = sheet_alloc(shtctl);

    unsigned char *buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    unsigned char *buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);

    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    unsigned char buf_mouse[256];
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    sheet_setbuf(sht_window, buf_win, 160, 52, -1);

    init_screen(buf_back, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(buf_mouse, 99);
    make_window8(buf_win, 160, 52, "counter");
    // putfont8_asc(buf_win, 160, 8, 32, BLACK, "This is the window");
    sheet_slide(sht_back, 0, 0);
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;
    sheet_slide(sht_mouse, mx, my);
    sheet_slide(sht_window, 80, 90);
    sheet_updown(sht_back, 0);
    sheet_updown(sht_mouse, 2);
    sheet_updown(sht_window, 1);

    sprintf(s, "mouse (%d, %d)", mx, my);
    putfont8_asc_sht(sht_back, 0, 0, WHITE, COL8_008400, s, 15);

    sprintf(s, "memory = %dMB , free = %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfont8_asc_sht(sht_back, 0, 50, WHITE, COL8_008400, s, 28);

    int i;
    for (;;)
    {
        sprintf(s, "cnt : %d", timerctl.count);
        putfont8_asc_sht(sht_window, 40, 28, BLACK, COL8_C6C6C6, s, 10);

        io_cli();
        if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) + fifo8_status(&timerfifo) == 0)
        {
            io_sti();
        }
        else
        {
            if (fifo8_status(&keyfifo) != 0)
            {
                i = fifo8_get(&keyfifo);
                io_sti();
                sprintf(s, "keycode %x", i);
                putfont8_asc_sht(sht_back, 0, 16, WHITE, COL8_008400, s, 11);
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
                    putfont8_asc_sht(sht_back, 32, 32, WHITE, COL8_008400, s, 15);
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
                    if (mx > binfo->scrnx - 1)
                    {
                        mx = binfo->scrnx - 1;
                    }
                    if (my > binfo->scrny - 1)
                    {
                        my = binfo->scrny - 1;
                    }
                    sprintf(s, "mouse (%d, %d)", mx, my);
                    putfont8_asc_sht(sht_back, 0, 0, WHITE, COL8_008400, s, 17);
                    sheet_slide(sht_mouse, mx, my); /* refresh 含む */
                }
            }
            else if (fifo8_status(&timerfifo) != 0)
            {
                i = fifo8_get(&timerfifo);
                io_sti();
                if (i == 10)
                {
                    putfont8_asc_sht(sht_back, 0, 80, WHITE, COL8_008400, "10 sec", 7);
                }
                else if (i == 3)
                {
                    putfont8_asc_sht(sht_back, 0, 64, WHITE, COL8_008400, "3 sec", 6);
                }
                else
                {
                    if (i != 0)
                    {
                        timer_init(timer3, &timerfifo, 0);
                        boxfill8(buf_back, binfo->scrnx, WHITE, 8, 96, 15, 111);
                    }
                    else
                    {
                        timer_init(timer3, &timerfifo, 1);
                        boxfill8(buf_back, binfo->scrnx, COL8_008400, 8, 96, 15, 111);
                    }
                    timer_settime(timer3, 50);
                    sheet_refresh(sht_back, 8, 96, 16, 112);
                }
            }
        }
    }
}
void make_window8(unsigned char *buf, int xsize, int ysize, char *title)
{
    static char closebtn[14][16] = {
        "OOOOOOOOOOOOOOO@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQQQ@@QQQQQ$@",
        "OQQQQQ@@@@QQQQ$@",
        "OQQQQ@@QQ@@QQQ$@",
        "OQQQ@@QQQQ@@QQ$@",
        "OQQQQQQQQQQQQQ$@",
        "OQQQQQQQQQQQQQ$@",
        "O$$$$$$$$$$$$$$@",
        "@@@@@@@@@@@@@@@@"};
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0);
    boxfill8(buf, xsize, WHITE, 1, 1, xsize - 2, 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, WHITE, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, WHITE, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_000084, 3, 3, xsize - 4, 20);
    boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, WHITE, 0, ysize - 1, xsize - 1, ysize - 1);
    putfont8_asc(buf, xsize, 24, 4, WHITE, title);

    for (int y = 0; y < 14; y++)
    {
        for (int x = 0; x < 16; x++)
        {
            char c = closebtn[y][x];
            if (c == '@')
            {
                c = BLACK;
            }
            else if (c == '$')
            {
                c = COL8_848484;
            }
            else if (c == 'Q')
            {
                c = COL8_C6C6C6;
            }
            else
            {
                c = WHITE;
            }
            buf[(5 + y) * xsize + (xsize - 21 + x)] = c;
        }
    }
    return;
}

void putfont8_asc_sht(struct SHEET *sht, int x, int y,
                      int c, int b, char *s, int l)
{
    boxfill8(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
    putfont8_asc(sht->buf, sht->bxsize, x, y, c, s);
    sheet_refresh(sht, x, y, x + l * 8, y + 16);

    return;
}
