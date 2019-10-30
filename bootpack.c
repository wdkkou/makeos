#include "bootpack.h"

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfont8_asc_sht(struct SHEET *sht, int x, int y,
                      int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0,
                   int sx, int sy, int c);

void task_b_main(struct SHEET *sht_back);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40];
    static char keytable[0x54] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0, 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0, 0, ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.'};

    init_gdtidt();
    init_pic();
    io_sti(); /* IDTとPICの初期化が終わったのでCPUの割り込み禁止を解除 */

    struct FIFO32 fifo;
    int fifobuf[128];
    fifo32_init(&fifo, 128, fifobuf, 0);

    init_pit();

    struct MOUSE_DEC mdec;
    init_keyboard(&fifo, 256);
    enable_mouse(&fifo, 512, &mdec);

    io_out8(PIC0_IMR, 0xf8); /* PITとPIC1とキーボードを許可 */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可 */

    struct TIMER *timer;

    /*カーソル点滅*/
    int cursor_x = 8;
    int cursor_c = WHITE;
    timer = timer_alloc();
    timer_init(timer, &fifo, 1);
    timer_settime(timer, 50);

    struct MEMMAM *memman = (struct MEMMAN *)MEM_ADDR;
    unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free_4k(memman, 0x00001000, 0x0009e000);
    memman_free_4k(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();
    struct SHTCTL *shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    struct TASK *task_a = task_init(memman);
    fifo.task = task_a;

    /* sht_back */
    struct SHEET *sht_back = sheet_alloc(shtctl);
    unsigned char *buf_back = (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    init_screen(buf_back, binfo->scrnx, binfo->scrny);

    /* sht_window */
    struct SHEET *sht_window = sheet_alloc(shtctl);
    unsigned char *buf_win = (unsigned char *)memman_alloc_4k(memman, 160 * 52);
    sheet_setbuf(sht_window, buf_win, 160, 52, -1);
    make_window8(buf_win, 160, 52, "task_a", 1);
    make_textbox8(sht_window, 8, 28, 144, 16, WHITE);

    /* sht_win_b */
    struct SHEET *sht_win_b[3];
    unsigned char *buf_win_b;
    struct TASK *task_b[3];
    for (int i = 0; i < 3; i++)
    {
        sht_win_b[i] = sheet_alloc(shtctl);
        buf_win_b = (unsigned char *)memman_alloc_4k(memman, 144 * 52);
        sheet_setbuf(sht_win_b[i], buf_win_b, 144, 52, -1); /* 透明色なし */
        sprintf(s, "task_b%d", i);
        make_window8(buf_win_b, 144, 52, s, 0);
        task_b[i] = task_alloc();
        task_b[i]->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
        task_b[i]->tss.eip = (int)&task_b_main;
        task_b[i]->tss.es = 1 * 8;
        task_b[i]->tss.cs = 2 * 8;
        task_b[i]->tss.ss = 1 * 8;
        task_b[i]->tss.ds = 1 * 8;
        task_b[i]->tss.fs = 1 * 8;
        task_b[i]->tss.gs = 1 * 8;
        *((int *)(task_b[i]->tss.esp + 4)) = (int)sht_win_b[i];
        task_run(task_b[i]);
    }

    /* sht_mouse */
    struct SHEET *sht_mouse = sheet_alloc(shtctl);
    unsigned char buf_mouse[256];
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;

    sheet_slide(sht_back, 0, 0);
    sheet_slide(sht_win_b[0], 168, 56);
    sheet_slide(sht_win_b[1], 8, 116);
    sheet_slide(sht_win_b[2], 168, 116);
    sheet_slide(sht_window, 8, 56);
    sheet_slide(sht_mouse, mx, my);
    sheet_updown(sht_back, 0);
    sheet_updown(sht_win_b[0], 1);
    sheet_updown(sht_win_b[1], 2);
    sheet_updown(sht_win_b[2], 3);
    sheet_updown(sht_window, 4);
    sheet_updown(sht_mouse, 5);

    sprintf(s, "mouse (%d, %d)", mx, my);
    putfont8_asc_sht(sht_back, 0, 0, WHITE, COL8_008400, s, 15);

    sprintf(s, "memory = %dMB , free = %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
    putfont8_asc_sht(sht_back, 0, 50, WHITE, COL8_008400, s, 28);

    int i;
    for (;;)
    {
        io_cli();
        if (fifo32_status(&fifo) == 0)
        {
            task_sleep(task_a);
            io_stihlt();
        }
        else
        {
            i = fifo32_get(&fifo);
            io_sti();
            /* キーボードデータ */
            if (256 <= i && i < 512)
            {
                sprintf(s, "keycode %x", i - 256);
                putfont8_asc_sht(sht_back, 0, 16, WHITE, COL8_008400, s, 11);

                if (i < 256 + 0x54)
                {
                    /*　1文字表示してからカーソルを1つすすめる */
                    if (keytable[i - 256] != 0 && cursor_x < 144)
                    {
                        s[0] = keytable[i - 256];
                        s[1] = 0;
                        putfont8_asc_sht(sht_window, cursor_x, 28, BLACK, WHITE, s, 1);
                        cursor_x += 8;
                    }
                }
                /*バックスペース*/
                if (i == 256 + 0x0e && cursor_x > 8)
                {
                    putfont8_asc_sht(sht_window, cursor_x, 28, BLACK, WHITE, " ", 1);
                    cursor_x -= 8;
                }
                /*カーソルの再表示*/
                boxfill8(sht_window->buf, sht_window->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_window, cursor_x, 28, cursor_x + 8, 44);
            }
            /* マウスデータ */
            else if (512 <= i && i < 768)
            {
                if (mouse_decode(&mdec, i - 512) != 0)
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

                    if ((mdec.btn & 0x01) != 0)
                    {
                        sheet_slide(sht_window, mx - 80, my - 8);
                    }
                }
            }
            else if (i == 10)
            {
                putfont8_asc_sht(sht_back, 0, 80, WHITE, COL8_008400, "10 sec", 7);
            }
            else if (i == 3)
            {
                putfont8_asc_sht(sht_back, 0, 64, WHITE, COL8_008400, "3 sec", 6);
            }
            else if (i == 1)
            {
                timer_init(timer, &fifo, 0);
                cursor_c = BLACK;
                timer_settime(timer, 50);
                boxfill8(sht_window->buf, sht_window->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_window, cursor_x, 28, cursor_x + 8, 44);
            }
            else if (i == 0)
            {
                timer_init(timer, &fifo, 1);
                cursor_c = WHITE;
                timer_settime(timer, 50);
                boxfill8(sht_window->buf, sht_window->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                sheet_refresh(sht_window, cursor_x, 28, cursor_x + 8, 44);
            }
        }
    }
}
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act)
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

    char tc, tbc;
    if (act != 0)
    {
        tc = WHITE;
        tbc = COL8_000084;
    }
    else
    {
        tc = COL8_C6C6C6;
        tbc = COL8_848484;
    }
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0);
    boxfill8(buf, xsize, WHITE, 1, 1, xsize - 2, 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, WHITE, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, WHITE, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
    boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, WHITE, 0, ysize - 1, xsize - 1, ysize - 1);
    putfont8_asc(buf, xsize, 24, 4, tc, title);

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

void make_textbox8(struct SHEET *sht, int x0, int y0, int sx, int sy, int c)
{
    int x1 = x0 + sx, y1 = y0 + sy;
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
    boxfill8(sht->buf, sht->bxsize, COL8_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, WHITE, x0 - 3, y1 + 2, x1 + 1, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, WHITE, x1 + 2, y0 - 3, x1 + 2, y1 + 2);
    boxfill8(sht->buf, sht->bxsize, BLACK, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
    boxfill8(sht->buf, sht->bxsize, BLACK, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, COL8_C6C6C6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
    boxfill8(sht->buf, sht->bxsize, c, x0 - 1, y0 - 1, x1 + 0, y1 + 0);
    return;
}

void task_b_main(struct SHEET *sht_win_b)
{
    struct FIFO32 fifo;
    int fifobuf[128];
    fifo32_init(&fifo, 128, fifobuf, 0);

    struct TIMER *timer_1s;
    timer_1s = timer_alloc();
    timer_init(timer_1s, &fifo, 100);
    timer_settime(timer_1s, 100);

    char s[10];
    int count = 0, count0 = 0;
    int res = 0;
    for (;;)
    {
        count++;
        io_cli();
        if (fifo32_status(&fifo) == 0)
        {
            io_sti();
        }
        else
        {
            int i = fifo32_get(&fifo);
            io_sti();
            if (i == 100)
            {
                sprintf(s, "%d", count - count0);
                putfont8_asc_sht(sht_win_b, 24, 28, BLACK, COL8_C6C6C6, s, 20);
                count0 = count;
                timer_settime(timer_1s, 100);
            }
        }
    }
}
