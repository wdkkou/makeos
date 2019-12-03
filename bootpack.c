#include "bootpack.h"

#define KEYCMD_LED 0xed

void keywin_off(struct SHEET *key_win);
void keywin_on(struct SHEET *key_win);
struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal);

void HariMain(void) {
    struct BOOTINFO *binfo = (struct BOOTINFO *)ADR_BOOTINFO;
    char s[40];

    static char keytable0[0x80] = {
        0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '^', 0x08, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0x0a, 0, 'A', 'S',
        'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0, 0, ']', 'Z', 'X', 'C', 'V',
        'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
        '2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0x5c, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x5c, 0, 0};
    static char keytable1[0x80] = {
        0, 0, '!', 0x22, '#', '$', '%', '&', 0x27, '(', ')', '~', '=', '~', 0x08, 0,
        'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0x0a, 0, 'A', 'S',
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

    struct MEMMAM *memman = (struct MEMMAN *)MEM_ADDR;
    unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free_4k(memman, 0x00001000, 0x0009e000);
    memman_free_4k(memman, 0x00400000, memtotal - 0x00400000);

    init_palette();
    struct SHTCTL *shtctl =
        shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
    struct TASK *task_a = task_init(memman);
    fifo.task           = task_a;
    task_run(task_a, 1, 0);
    *((int *)0x0fe4) = (int)shtctl;

    /* sht_back */
    struct SHEET *sht_back = sheet_alloc(shtctl);
    unsigned char *buf_back =
        (unsigned char *)memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
    sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
    init_screen(buf_back, binfo->scrnx, binfo->scrny);

    /* sht_mouse */
    struct SHEET *sht_mouse = sheet_alloc(shtctl);
    unsigned char buf_mouse[256];
    sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
    init_mouse_cursor8(buf_mouse, 99);
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;

    struct SHEET *key_win = open_console(shtctl, memtotal);

    sheet_slide(sht_back, 0, 0);
    sheet_slide(key_win, 32, 4);
    sheet_slide(sht_mouse, mx, my);
    sheet_updown(sht_back, 0);
    sheet_updown(key_win, 1);
    sheet_updown(sht_mouse, 2);
    keywin_on(key_win);

    int i;
    int key_shift   = 0;
    int key_leds    = (binfo->leds >> 4) & 7;
    int keycmd_wait = -1;
    int mmx = -1, mmy = -1, mmx2 = 0;
    int new_mx = -1, new_my = 0;
    int new_wx = 0x7fffffff, new_wy = 0;

    struct SHEET *sht = 0;

    /* 最初にキーボード状態との食い違いがないように、設定しておくことにする */
    struct FIFO32 keycmd;
    int keycmd_buf[32];
    fifo32_init(&keycmd, 32, keycmd_buf, 0);
    fifo32_put(&keycmd, KEYCMD_LED);
    fifo32_put(&keycmd, key_leds);

    for (;;) {
        if (fifo32_status(&keycmd) > 0 && keycmd_wait < 0) {
            /* キーボードコントローラに送るデータがあれば、送る */
            keycmd_wait = fifo32_get(&keycmd);
            wait_KBC_sendready();
            io_out8(PORT_KEYDAT, keycmd_wait);
        }
        io_cli();
        if (fifo32_status(&fifo) == 0) {
            /* fifoが空っぽになったので, 保留中の描画があれば実行 */
            if (new_mx >= 0) {
                io_sti();
                sheet_slide(sht_mouse, new_mx, new_my);
                new_mx = -1;
            } else if (new_wx != 0x7fffffff) {
                io_sti();
                sheet_slide(sht, new_wx, new_wy);
                new_wx = 0x7fffffff;
            } else {
                task_sleep(task_a);
                io_sti();
            }
        } else {
            i = fifo32_get(&fifo);
            io_sti();
            if (key_win->flags == 0) {
                /* windowが閉じられた */
                key_win = shtctl->sheets[shtctl->top - 1];
                keywin_on(key_win);
            }
            /* キーボードデータ */
            if (256 <= i && i < 512) {
                if (i < 0x80 + 256) /* キーコードを文字コードに変換 */
                {
                    if (key_shift == 0) {
                        s[0] = keytable0[i - 256];
                    } else {
                        s[0] = keytable1[i - 256];
                    }
                } else {
                    s[0] = 0;
                }
                if ('A' <= s[0] && s[0] <= 'Z') {
                    if (((key_leds & 4) == 0 && key_shift == 0) ||
                        ((key_leds & 4) != 0 && key_shift != 0)) {
                        s[0] += 0x20; /* 大文字を小文字に変換 */
                    }
                }
                if (s[0] != 0) {
                    fifo32_put(&key_win->task->fifo, s[0] + 256);
                }
                /* tab */
                if (i == 256 + 0x0f) {
                    keywin_off(key_win);
                    int height = key_win->height - 1;
                    if (height == 0) {
                        height = shtctl->top - 1;
                    }
                    key_win = shtctl->sheets[height];
                    keywin_on(key_win);
                }
                if (i == 256 + 0x2a) { /* 左シフト　on */
                    key_shift |= 1;
                }
                if (i == 256 + 0x36) { /* 右シフト　on */
                    key_shift |= 2;
                }
                if (i == 256 + 0xaa) { /* 左シフト　off */
                    key_shift &= ~1;
                }
                if (i == 256 + 0xb6) { /* 右シフト　off */
                    key_shift &= ~2;
                }
                if (i == 256 + 0x3a) { /* CapsLock */
                    key_leds ^= 4;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x45) { /* NumLock */
                    key_leds ^= 2;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x46) { /* ScrollLock */
                    key_leds ^= 1;
                    fifo32_put(&keycmd, KEYCMD_LED);
                    fifo32_put(&keycmd, key_leds);
                }
                if (i == 256 + 0x2e && key_shift != 0) {
                    /* shift + c */
                    struct TASK *task = key_win->task;
                    cons_putstr(task->cons, "\nBreak\n");
                    io_cli();
                    task->tss.eax = (int)&(task->tss.esp0);
                    task->tss.eip = (int)asm_end_app;
                    io_sti();
                }
                if (i == 256 + 0x3c && key_shift != 0) {
                    /* shift + f2 */
                    keywin_off(key_win);
                    key_win = open_console(shtctl, memtotal);
                    sheet_slide(key_win, 32, 4);
                    sheet_updown(key_win, shtctl->top);
                    keywin_on(key_win);
                }
                if (i == 256 + 0x57 && shtctl->top > 2) {
                    sheet_updown(shtctl->sheets[1], shtctl->top - 1);
                }
                if (i == 256 + 0xfa) { /* キーボードがデータを無事に受け取った */
                    keycmd_wait = -1;
                }
                if (i == 256 + 0xfe) { /* キーボードがデータを無事に受け取れなかった */
                    wait_KBC_sendready();
                    io_out8(PORT_KEYDAT, keycmd_wait);
                }
            }
            /* マウスデータ */
            else if (512 <= i && i < 768) {
                if (mouse_decode(&mdec, i - 512) != 0) {
                    /* マウスカーソルの移動 */
                    mx += mdec.x;
                    my += mdec.y;
                    if (mx < 0) {
                        mx = 0;
                    }
                    if (my < 0) {
                        my = 0;
                    }
                    if (mx > binfo->scrnx - 1) {
                        mx = binfo->scrnx - 1;
                    }
                    if (my > binfo->scrny - 1) {
                        my = binfo->scrny - 1;
                    }
                    new_mx = mx;
                    new_my = my;
                    int x, y;
                    if ((mdec.btn & 0x01) != 0) {
                        if (mmx < 0) {
                            for (int j = shtctl->top - 1; j > 0; j--) {
                                sht = shtctl->sheets[j];
                                x   = mx - sht->vx0;
                                y   = my - sht->vy0;
                                if (0 <= x && x < sht->bxsize && 0 <= y && y < sht->bysize) {
                                    if (sht->buf[y * sht->bxsize + x] != sht->col_inv) {
                                        sheet_updown(sht, shtctl->top - 1);
                                        if (sht != key_win) {
                                            keywin_off(key_win);
                                            key_win = sht;
                                            keywin_on(key_win);
                                        }
                                        if (3 <= x && x < sht->bxsize - 3 && 3 <= y && y < 21) {
                                            mmx    = mx;
                                            mmy    = my;
                                            mmx2   = sht->vx0;
                                            new_wy = sht->vy0;
                                        }
                                        if (sht->bxsize - 21 <= x && x < sht->bxsize - 5 && 5 <= y && y < 19) {
                                            /* ✕をクリック*/
                                            if ((sht->flags & 0x10) != 0) {
                                                struct TASK *task = sht->task;
                                                cons_putstr(task->cons, "Break(mouse)\n");
                                                io_cli();
                                                task->tss.eax = (int)&(task->tss.esp0);
                                                task->tss.eip = (int)asm_end_app;
                                                io_sti();
                                            }
                                        }
                                        break;
                                    }
                                }
                            }
                        } else {
                            x      = mx - mmx;
                            y      = my - mmy;
                            new_wx = (mmx2 + x + 2) & ~3;
                            new_wy = new_wy + y;
                            mmy    = my;
                        }
                    } else {
                        /* 左ボタンを押していない */
                        mmx = -1;
                        if (new_wx != 0x7fffffff) {
                            sheet_slide(sht, new_wx, new_wy); /* 1度確定させる */
                            new_wx = 0x7fffffff;
                        }
                    }
                }
                /* なぜか，sht_backをrefreshしないと画面下の方でshtが残る */
                // sheet_refresh(sht_back, 0, 0, 1 * 8, 1);
            }
        }
    }
}

void keywin_off(struct SHEET *key_win) {
    change_wtitle8(key_win, 0);
    if ((key_win->flags & 0x20) != 0) {
        fifo32_put(&key_win->task->fifo, 3);
    }
    return;
}

void keywin_on(struct SHEET *key_win) {
    change_wtitle8(key_win, 1);

    if ((key_win->flags & 0x20) != 0) {
        fifo32_put(&key_win->task->fifo, 2);
    }
    return;
}

struct SHEET *open_console(struct SHTCTL *shtctl, unsigned int memtotal) {
    struct MEMMAN *memman = (struct MEMMAN *)MEM_ADDR;
    struct SHEET *sht     = sheet_alloc(shtctl);
    unsigned char *buf    = (unsigned char *)memman_alloc_4k(memman, 256 * 165);
    struct TASK *task     = task_alloc();
    int *cons_fifo        = (unsigned char *)memman_alloc_4k(memman, 128 * 4);
    sheet_setbuf(sht, buf, 256, 165, -1);
    make_window8(buf, 256, 165, "console", 0);
    make_textbox8(sht, 8, 28, 240, 128, BLACK);
    task->tss.esp                 = memman_alloc_4k(memman, 64 * 1024) + 64 * 1024 - 12;
    task->tss.eip                 = (int)&console_task;
    task->tss.es                  = 1 * 8;
    task->tss.cs                  = 2 * 8;
    task->tss.ss                  = 1 * 8;
    task->tss.ds                  = 1 * 8;
    task->tss.fs                  = 1 * 8;
    task->tss.gs                  = 1 * 8;
    *((int *)(task->tss.esp + 4)) = (int)sht;
    *((int *)(task->tss.esp + 8)) = memtotal;
    task_run(task, 2, 2); /*level=2 ,priority=2*/
    sht->task = task;
    sht->flags |= 0x20;
    fifo32_init(&task->fifo, 128, cons_fifo, task);

    return sht;
}
