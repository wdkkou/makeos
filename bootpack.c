#include "bootpack.h"

struct FILEINFO
{
    unsigned char name[8], ext[3], type;
    char reserve[10];
    unsigned short time, date, clustno;
    unsigned int size;
};

void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act);
void putfont8_asc_sht(struct SHEET *sht, int x, int y,
                      int c, int b, char *s, int l);
void make_textbox8(struct SHEET *sht, int x0, int y0,
                   int sx, int sy, int c);
void console_task(struct SHEET *sheet, unsigned int memtotal);
int cons_newline(int cursor_y, struct SHEET *sheet);

#define KEYCMD_LED 0xed

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40];

    static char keytable0[0x80] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0, 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0, 0, ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0x5c, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5c, 0, 0};
    static char keytable1[0x80] = {
        0, 0, '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0, 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0, 0, '}', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, '_', 0, 0, 0, 0, 0, 0, 0, 0, 0, '|', 0, 0};

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
    task_run(task_a, 1, 0);

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

    /* sht_cons */
    struct SHEET *sht_cons = sheet_alloc(shtctl);
    unsigned *buf_cons = (unsigned char *)memman_alloc_4k(memman, 256 * 165);
    sheet_setbuf(sht_cons, buf_cons, 256, 165, -1);
    make_window8(buf_cons, 256, 165, "console", 0);
    make_textbox8(sht_cons, 8, 28, 240, 128, BLACK);
    struct TASK *task_cons = task_alloc();
    task_cons->tss.esp = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 8;
    task_cons->tss.eip = (int)&console_task;
    task_cons->tss.es = 1 * 8;
    task_cons->tss.cs = 2 * 8;
    task_cons->tss.ss = 1 * 8;
    task_cons->tss.ds = 1 * 8;
    task_cons->tss.fs = 1 * 8;
    task_cons->tss.gs = 1 * 8;
    *((int *)(task_cons->tss.esp + 4)) = (int)sht_cons;
    *((int *)(task_cons->tss.esp + 8)) = memtotal;
    task_run(task_cons, 2, 2); /*level=2 ,priority=2*/

    /* sht_mouse */
    struct SHEET *sht_mouse = sheet_alloc(shtctl);
    unsigned char buf_mouse[256];
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;

    sheet_slide(sht_back, 0, 0);
    sheet_slide(sht_cons, 32, 4);
    sheet_slide(sht_window, 64, 56);
    sheet_slide(sht_mouse, mx, my);
    sheet_updown(sht_back, 0);
    sheet_updown(sht_cons, 1);
    sheet_updown(sht_window, 2);
    sheet_updown(sht_mouse, 3);

    int i;
    int key_to = 0, key_shift = 0;
    int key_leds = (binfo->leds >> 4) & 7;
    int keycmd_wait = -1;

    /* 最初にキーボード状態との食い違いがないように、設定しておくことにする */
    struct FIFO32 keycmd;
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

    for (;;)
    {
        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0)
        {
            /* キーボードコントローラに送るデータがあれば、送る */
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }
        io_cli();
        if (fifo32_status(&fifo) == 0)
        {
            task_sleep(task_a);
            io_sti();
        }
        else
        {
            i = fifo32_get(&fifo);
            io_sti();
            /* キーボードデータ */
            if (256 <= i && i < 512)
            {
                if (i < 0x80 + 256) /* キーコードを文字コードに変換 */
                {
                    if (key_shift == 0)
                    {
                        s[0] = keytable0[i - 256];
                    }
                    else
                    {
                        s[0] = keytable1[i - 256];
                    }
                }
                else
                {
                    s[0] = 0;
                }
                if ('A' <= s[0] && s[0] <= 'Z')
                {
                    if (((key_leds & 4) == 0 && key_shift == 0) ||
                        ((key_leds & 4) != 0 && key_shift != 0))
                    {
                        s[0] += 0x20; /* 大文字を小文字に変換 */
                    }
                }
                if (s[0] != 0)
                {
                    if (key_to == 0) /* タスクA */
                    {
                        /*　1文字表示してからカーソルを1つすすめる */
                        if (cursor_x < 128)
                        {
                            s[1] = 0;
                            putfont8_asc_sht(sht_window, cursor_x, 28, BLACK, WHITE, s, 1);
                            cursor_x += 8;
                        }
                    }
                    else
                    {
                        fifo32_put(&task_cons->fifo, s[0] + 256);
                    }
                }
                /*バックスペース*/
                if (i == 256 + 0x0e) /* バックスペース */
                {
                    if (key_to == 0)
                    {
                        if (cursor_x > 8)
                        {
                            /* カーソルをスペースで消して，カーソル位置を1つ戻す */
                            putfont8_asc_sht(sht_window, cursor_x, 28, BLACK, WHITE, " ", 1);
                            cursor_x -= 8;
                        }
                    }
                    else
                    {
                        fifo32_put(&task_cons->fifo, 8 + 256);
                    }
                }
                if (i == 256 + 0x1c) /* Enter */
                {
                    if (key_to != 0)
                    {
                        fifo32_put(&task_cons->fifo, 10 + 256);
                    }
                }
                /* tab */
                if (i == 256 + 0x0f)
                {
                    if (key_to == 0)
                    {
                        key_to = 1;
                        make_wtitle8(buf_win, sht_window->bxsize, "task_a", 0);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 1);
                        cursor_c = -1; /* カーソルを消す*/
                        boxfill8(sht_window->buf, sht_window->bxsize, WHITE, cursor_x, 28, cursor_x + 7, 43);
                        fifo32_put(&task_cons->fifo, 2); /*コンソールのカーソルON*/
                    }
                    else
                    {
                        key_to = 0;
                        make_wtitle8(buf_win, sht_window->bxsize, "task_a", 1);
                        make_wtitle8(buf_cons, sht_cons->bxsize, "console", 0);
                        cursor_c = BLACK;                /* カーソルを出す*/
                        fifo32_put(&task_cons->fifo, 3); /*コンソールのカーソルOFF*/
                    }
                    sheet_refresh(sht_window, 0, 0, sht_window->bxsize, 21);
                    sheet_refresh(sht_cons, 0, 0, sht_cons->bxsize, 21);
                }
                if (i == 256 + 0x2a)
                { /* 左シフト　on */
                    key_shift |= 1;
                }
                if (i == 256 + 0x36)
                { /* 右シフト　on */
                    key_shift |= 2;
                }
                if (i == 256 + 0xaa)
                { /* 左シフト　off */
                    key_shift &= ~1;
                }
                if (i == 256 + 0xb6)
                { /* 右シフト　off */
                    key_shift &= ~2;
                }
                if (i == 256 + 0x3a)
                { /* CapsLock */
                    key_leds ^= 4;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x45)
                { /* NumLock */
                    key_leds ^= 2;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x46)
                { /* ScrollLock */
                    key_leds ^= 1;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0xfa)
                { /* キーボードがデータを無事に受け取った */
                    keycmd_wait = -1;
                }
                if (i == 256 + 0xfe)
                { /* キーボードがデータを無事に受け取れなかった */
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }
                /*カーソルの再表示*/
                if (cursor_c >= 0)
                {
                    boxfill8(sht_window->buf, sht_window->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                }
                sheet_refresh(sht_window, cursor_x, 28, cursor_x + 8, 44);
            }
            /* マウスデータ */
            else if (512 <= i && i < 768)
            {
                if (mouse_decode(&mdec, i - 512) != 0)
                {
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
                    putfont8_asc_sht(sht_back, 0, 0, WHITE, COL8_008400, s, 17);
                    sheet_slide(sht_mouse, mx, my); /* refresh 含む */
                    if ((mdec.btn & 0x01) != 0)
                    {
                        sheet_slide(sht_window, mx - 80, my - 8);
                    }
                }
            }
            else if (i == 1)
            {
                timer_init(timer, &fifo, 0);
                if (cursor_c >= 0)
                {
                    cursor_c = BLACK;
                }
                timer_settime(timer, 50);
                if (cursor_c >= 0)
                {
                    boxfill8(sht_window->buf, sht_window->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                    sheet_refresh(sht_window, cursor_x, 28, cursor_x + 8, 44);
                }
            }
            else if (i == 0)
            {
                timer_init(timer, &fifo, 1);
                if (cursor_c >= 0)
                {
                    cursor_c = WHITE;
                }
                timer_settime(timer, 50);
                if (cursor_c >= 0)
                {
                    boxfill8(sht_window->buf, sht_window->bxsize, cursor_c, cursor_x, 28, cursor_x + 7, 43);
                    sheet_refresh(sht_window, cursor_x, 28, cursor_x + 8, 44);
                }
            }
        }
    }
}
void make_window8(unsigned char *buf, int xsize, int ysize, char *title, char act)
{
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, xsize - 1, 0);
    boxfill8(buf, xsize, WHITE, 1, 1, xsize - 2, 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 0, 0, 0, ysize - 1);
    boxfill8(buf, xsize, WHITE, 1, 1, 1, ysize - 2);
    boxfill8(buf, xsize, COL8_848484, xsize - 2, 1, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, WHITE, xsize - 1, 0, xsize - 1, ysize - 1);
    boxfill8(buf, xsize, COL8_C6C6C6, 2, 2, xsize - 3, ysize - 3);
    boxfill8(buf, xsize, COL8_848484, 1, ysize - 2, xsize - 2, ysize - 2);
    boxfill8(buf, xsize, WHITE, 0, ysize - 1, xsize - 1, ysize - 1);
    make_wtitle8(buf, xsize, title, act);

    return;
}
void make_wtitle8(unsigned char *buf, int xsize, char *title, char act)
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
    boxfill8(buf, xsize, tbc, 3, 3, xsize - 4, 20);
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

void console_task(struct SHEET *sheet, unsigned int memtotal)
{
    struct FIFO32 fifo;
    int fifobuf[128];
    struct TASK *task = task_now();
    struct MEMMAN *memman = (struct MEMMAN *)MEM_ADDR;
    struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);

    fifo32_init(&task->fifo, 128, fifobuf, task);

    struct TIMER *timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    /* プロンプト表示 */
    putfont8_asc_sht(sheet, 8, 28, WHITE, BLACK, "$ ", 2);

    char s[30];
    char cmdline[30];
    int cursor_x = 24, cursor_y = 28, cursor_c = -1;
    while (1)
    {
        io_cli();
        if (fifo32_status(&task->fifo) == 0)
        {
            task_sleep(task);
            io_sti();
        }
        else
        {
            int data = fifo32_get(&task->fifo);
            io_sti();
            if (data <= 1)
            {
                if (data != 0)
                {
                    timer_init(timer, &task->fifo, 0);
                    if (cursor_c >= 0)
                    {
                        cursor_c = WHITE;
                    }
                }
                else
                {
                    timer_init(timer, &task->fifo, 1);
                    if (cursor_c >= 0)
                    {
                        cursor_c = BLACK;
                    }
                }
                timer_settime(timer, 50);
            }
            if (data == 2) /* カーソルon */
            {
                cursor_c = WHITE;
            }
            if (data == 3) /* カーソルOFF */
            {
                boxfill8(sheet->buf, sheet->bxsize, BLACK, cursor_x, 28, cursor_x + 7, cursor_y + 15);
                cursor_c = -1;
            }
            if (256 <= data && data <= 511)
            { /* キーボードデータ（タスクA経由） */
                if (data == 8 + 256)
                {
                    /* バックスペース */
                    if (cursor_x > 24)
                    {
                        /* カーソルをスペースで消してから、カーソルを1つ戻す */
                        putfont8_asc_sht(sheet, cursor_x, cursor_y, WHITE, BLACK, " ", 1);
                        cursor_x -= 8;
                    }
                }
                else if (data == 10 + 256)
                {
                    putfont8_asc_sht(sheet, cursor_x, cursor_y, WHITE, BLACK, " ", 1);
                    cmdline[cursor_x / 8 - 3] = 0;
                    cursor_y = cons_newline(cursor_y, sheet);
                    /* コマンド実行 */
                    if (strcmp(cmdline, "mem") == 0)
                    {
                        sprintf(s, "total %dMB", memtotal / (1024 * 1024));
                        putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                        sprintf(s, "free %dKB", memman_total(memman) / 1024);
                        putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    else if (strcmp(cmdline, "clear") == 0)
                    {
                        for (int y = 28; y < 28 + 128; y++)
                        {
                            for (int x = 8; x < 8 + 240; x++)
                            {
                                sheet->buf[x + y * sheet->bxsize] = BLACK;
                            }
                        }
                        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
                        cursor_y = 28;
                    }
                    else if (strcmp(cmdline, "ls") == 0)
                    {
                        for (int x = 0; x < 224; x++)
                        {
                            if (finfo[x].name[0] == 0x00)
                            {
                                break;
                            }
                            if (finfo[x].name[0] != 0xe5)
                            {
                                if ((finfo[x].type & 0x18) == 0)
                                {
                                    sprintf(s, "filename.ext %d", finfo[x].size);
                                    for (int y = 0; y < 8; y++)
                                    {
                                        s[y] = finfo[x].name[y];
                                    }
                                    s[9] = finfo[x].ext[0];
                                    s[10] = finfo[x].ext[1];
                                    s[11] = finfo[x].ext[2];
                                    putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, s, 30);
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                            }
                        }
                    }
                    else if (cmdline[0] != 0)
                    {
                        putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, "command error", 15);
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    /* プロンプト表示 */
                    putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, "$ ", 2);
                    cursor_x = 24;
                }
                else
                {
                    /* 一般文字 */
                    if (cursor_x < 240)
                    {
                        /* 一文字表示してから、カーソルを1つ進める */
                        s[0] = data - 256;
                        s[1] = 0;
                        cmdline[cursor_x / 8 - 3] = data - 256;
                        putfont8_asc_sht(sheet, cursor_x, cursor_y, WHITE, BLACK, s, 1);
                        cursor_x += 8;
                    }
                }
            }
            /* カーソル再表示 */
            if (cursor_c >= 0)
            {
                boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x, cursor_y, cursor_x + 7, cursor_y + 15);
            }
            sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
        }
    }
}
int cons_newline(int cursor_y, struct SHEET *sheet)
{
    if (cursor_y < 28 + 112)
    {
        cursor_y += 16;
    }
    else
    {
        for (int y = 28; y < 28 + 112; y++)
        {
            for (int x = 8; x < 8 + 240; x++)
            {
                sheet->buf[x + y * sheet->bxsize] = sheet->buf[x + (y + 16) * sheet->bxsize];
            }
        }
        for (int y = 28 + 112; y < 28 + 128; y++)
        {
            for (int x = 8; x < 8 + 240; x++)
            {
                sheet->buf[x + y * sheet->bxsize] = BLACK;
            }
        }
        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    }
    return cursor_y;
}
