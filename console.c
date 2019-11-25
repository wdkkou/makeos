#include "bootpack.h"

void console_task(struct SHEET *sheet, unsigned int memtotal) {
    struct FIFO32 fifo;
    int fifobuf[128];
    struct TASK *task              = task_now();
    struct MEMMAN *memman          = (struct MEMMAN *)MEM_ADDR;
    struct FILEINFO *finfo         = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;

    fifo32_init(&task->fifo, 128, fifobuf, task);

    struct TIMER *timer = timer_alloc();
    timer_init(timer, &task->fifo, 1);
    timer_settime(timer, 50);

    int *fat = (int *)memman_alloc_4k(memman, 4 * 2880);
    file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));

    char cmdline[30];
    struct CONSOLE cons;
    cons.sht         = sheet;
    cons.cur_x       = 8;
    cons.cur_y       = 28;
    cons.cur_c       = -1;
    *((int *)0x0fec) = (int)&cons;

    /* プロンプト表示 */
    cons_putstr(&cons, "$ ");

    while (1) {
        io_cli();
        if (fifo32_status(&task->fifo) == 0) {
            task_sleep(task);
            io_sti();
        } else {
            int data = fifo32_get(&task->fifo);
            io_sti();
            if (data <= 1) {
                if (data != 0) {
                    timer_init(timer, &task->fifo, 0);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = WHITE;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = BLACK;
                    }
                }
                timer_settime(timer, 50);
            }
            if (data == 2) /* カーソルon */
            {
                cons.cur_c = WHITE;
            }
            if (data == 3) /* カーソルOFF */
            {
                boxfill8(sheet->buf, sheet->bxsize, BLACK, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
                cons.cur_c = -1;
            }
            if (256 <= data && data <= 511) { /* キーボードデータ（タスクA経由） */
                if (data == 8 + 256) {
                    /* バックスペース */
                    if (cons.cur_x > 24) {
                        /* カーソルをスペースで消してから、カーソルを1つ戻す */
                        cons_putchar(&cons, ' ', 0);
                        cons.cur_x -= 8;
                    }
                } else if (data == 10 + 256) {
                    cons_putchar(&cons, ' ', 0);
                    cmdline[cons.cur_x / 8 - 3] = 0;
                    cons_newline(&cons);
                    cons_runcmd(cmdline, &cons, fat, memtotal);
                    cons_putstr(&cons, "$ ");
                } else {
                    /* 一般文字 */
                    if (cons.cur_x < 240) {
                        /* 一文字表示してから、カーソルを1つ進める */
                        cmdline[cons.cur_x / 8 - 3] = data - 256;
                        cons_putchar(&cons, data - 256, 1);
                    }
                }
            }
            /* カーソル再表示 */
            if (cons.cur_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cons.cur_c, cons.cur_x,
                         cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
            }
            sheet_refresh(sheet, cons.cur_x, cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);
        }
    }
}
void cons_putchar(struct CONSOLE *cons, int chr, char move) {
    char s[2];
    s[0] = chr;
    s[1] = 0;
    if (s[0] == 0x09) {
        while (1) {
            putfont8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, WHITE, BLACK, " ", 1);
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240) {
                cons_newline(cons);
            }
            if (((cons->cur_x - 8) & 0x1f) == 0) {
                break; /* 32で割り切れたらbreak */
            }
        }
    } else if (s[0] == 0x0a) {
        cons_newline(cons);
    } else if (s[0] == 0x0d) {
        /* とりあえず何もしない */
    } else {
        putfont8_asc_sht(cons->sht, cons->cur_x, cons->cur_y, WHITE, BLACK, s, 1);
        if (move != 0) {
            cons->cur_x += 8;
            if (cons->cur_x == 8 + 240) {
                cons_newline(cons);
            }
        }
    }
    return;
}
void cons_newline(struct CONSOLE *cons) {
    struct SHEET *sheet = cons->sht;
    if (cons->cur_y < 28 + 112) {
        cons->cur_y += 16;
    } else {
        for (int y = 28; y < 28 + 112; y++) {
            for (int x = 8; x < 8 + 240; x++) {
                sheet->buf[x + y * sheet->bxsize] =
                    sheet->buf[x + (y + 16) * sheet->bxsize];
            }
        }
        for (int y = 28 + 112; y < 28 + 128; y++) {
            for (int x = 8; x < 8 + 240; x++) {
                sheet->buf[x + y * sheet->bxsize] = BLACK;
            }
        }
        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    }
    cons->cur_x = 8;
    return;
}
void cons_putstr(struct CONSOLE *cons, char *s) {
    for (; *s != 0; s++) {
        cons_putchar(cons, *s, 1);
    }
    return;
}
void cons_putstr_len(struct CONSOLE *cons, char *s, int l) {
    for (int i = 0; i < l; i++) {
        cons_putchar(cons, s[i], 1);
    }
    return;
}

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, unsigned int memtotal) {
    if (strcmp(cmdline, "mem") == 0) {
        cmd_mem(cons, memtotal);
    } else if (strcmp(cmdline, "clear") == 0) {
        cmd_clear(cons);
    } else if (strcmp(cmdline, "ls") == 0) {
        cmd_ls(cons);
    } else if (strncmp(cmdline, "cat ", 4) == 0) {
        cmd_cat(cons, fat, cmdline);
    } else if (cmdline[0] != 0) {
        if (cmd_app(cons, fat, cmdline) == 0) {
            /* コマンドエラー */
            cons_putstr(cons, "command error.\n");
        }
    }
    return;
}

void cmd_mem(struct CONSOLE *cons, unsigned int memtotal) {
    struct MEMMAN *memman = (struct MEMMAN *)MEM_ADDR;
    char s[60];
    sprintf(s, "total %dMB\n", memtotal / (1024 * 1024));
    cons_putstr(cons, s);
    sprintf(s, "free %dKB\n", memman_total(memman) / 1024);
    cons_putstr(cons, s);
    return;
}

void cmd_clear(struct CONSOLE *cons) {
    struct SHEET *sheet = cons->sht;
    for (int y = 28; y < 28 + 128; y++) {
        for (int x = 8; x < 8 + 240; x++) {
            sheet->buf[x + y * sheet->bxsize] = BLACK;
        }
    }
    sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
    cons->cur_y = 28;
    return;
}

void cmd_ls(struct CONSOLE *cons) {
    struct FILEINFO *finfo = (struct FILEINFO *)(ADR_DISKIMG + 0x002600);
    char s[30];
    for (int x = 0; x < 224; x++) {
        if (finfo[x].name[0] == 0x00) {
            break;
        }
        if (finfo[x].name[0] != 0xe5) {
            if ((finfo[x].type & 0x18) == 0) {
                sprintf(s, "filename.ext %d\n", finfo[x].size);
                for (int y = 0; y < 8; y++) {
                    s[y] = finfo[x].name[y];
                }
                s[9]  = finfo[x].ext[0];
                s[10] = finfo[x].ext[1];
                s[11] = finfo[x].ext[2];
                cons_putstr(cons, s);
            }
        }
    }
    return;
}

void cmd_cat(struct CONSOLE *cons, int *fat, char *cmdline) {
    /* catコマンド */
    struct MEMMAN *memman  = (struct MEMMAN *)MEM_ADDR;
    struct FILEINFO *finfo = file_search(cmdline + 4, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    if (finfo != 0) {
        /* ファイルが見つかった場合 */
        char *p = (char *)memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
        cons_putstr_len(cons, p, finfo->size);
        memman_free_4k(memman, (int)p, finfo->size);
    } else {
        /* ファイルが見つからなかった場合 */
        cons_putstr(cons, "file not found\n");
    }
    return;
}

int cmd_app(struct CONSOLE *cons, int *fat, char *cmdline) {
    struct MEMMAN *memman          = (struct MEMMAN *)MEM_ADDR;
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;

    char name[18];
    int index = 13;
    for (int i = 0; i < 13; i++) {
        if (cmdline[i] <= ' ') {
            index = i;
            break;
        }
        name[i] = cmdline[i];
    }
    name[index] = 0;

    struct FILEINFO *finfo = file_search(name, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    if (!finfo && name[index - 1] != '.') {
        /* hltコマンドでは見つからないので，hlt.binでは見つかるか探す*/
        name[index]     = '.';
        name[index + 1] = 'B';
        name[index + 2] = 'I';
        name[index + 3] = 'N';
        name[index + 4] = '0';
        finfo           = file_search(name, (struct FILEINFO *)(ADR_DISKIMG + 0x002600), 224);
    }

    if (finfo) {
        char *p         = (char *)memman_alloc_4k(memman, finfo->size);
        char *q         = (char *)memman_alloc_4k(memman, 64 * 1024);
        *((int *)0xfe8) = (int)p;
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
        set_segmdesc(gdt + 1003, finfo->size - 1, (int)p, AR_CODE32_ER);
        set_segmdesc(gdt + 1004, 64 * 1024 - 1, (int)q, AR_DATA32_RW);
        if (finfo->size >= 8 && strncmp(p + 4, "Hari", 4) == 0) {
            /* call 0x1b のアセンブル */
            p[0] = 0xe8;
            p[1] = 0x16;
            p[2] = 0x00;
            p[3] = 0x00;
            p[4] = 0x00;
            p[5] = 0xcb;
        }
        // farcall(0, 1003 * 8);
        start_app(0, 1003 * 8, 64 * 1024, 1004 * 8);
        memman_free_4k(memman, (int)p, finfo->size);
        memman_free_4k(memman, (int)q, 64 * 1024);
        cons_newline(cons);
        return 1;
    }
    /* ファイルが見つからない場合 */
    return 0;
}

void bin_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) {
    int cs_base          = *((int *)0xfe8);
    struct CONSOLE *cons = (struct CONSOLE *)*((int *)0x0fec);

    if (edx == 1) {
        cons_putchar(cons, eax & 0xff, 1);
    } else if (edx == 2) {
        cons_putstr(cons, (char *)ebx + cs_base);
    } else if (edx == 3) {
        cons_putstr_len(cons, (char *)ebx + cs_base, ecx);
    }
    return;
}

int inthandler0d(int *esp) {
    struct CONSOLE *cons = (struct CONSOLE *)*((int *)0x0fec);
    cons_putstr(cons, "\nINT 0d :\n General Prodtected Exception.\n");
    return 1;
}
