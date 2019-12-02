#include "bootpack.h"

void console_task(struct SHEET *sheet, int memtotal) {
    struct TASK *task     = task_now();
    struct MEMMAN *memman = (struct MEMMAN *)MEM_ADDR;

    int *fat = (int *)memman_alloc_4k(memman, 4 * 2880);
    file_readfat(fat, (unsigned char *)(ADR_DISKIMG + 0x000200));

    char cmdline[30];
    struct CONSOLE cons;
    cons.sht   = sheet;
    cons.cur_x = 8;
    cons.cur_y = 28;
    cons.cur_c = -1;
    // *((int *)0x0fec) = (int)&cons;
    task->cons = &cons;

    cons.timer = timer_alloc();
    timer_init(cons.timer, &task->fifo, 1);
    timer_settime(cons.timer, 50);

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
                    timer_init(cons.timer, &task->fifo, 0);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = WHITE;
                    }
                } else {
                    timer_init(cons.timer, &task->fifo, 1);
                    if (cons.cur_c >= 0) {
                        cons.cur_c = BLACK;
                    }
                }
                timer_settime(cons.timer, 50);
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

void cons_runcmd(char *cmdline, struct CONSOLE *cons, int *fat, int memtotal) {
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

void cmd_mem(struct CONSOLE *cons, int memtotal) {
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
    struct TASK *task              = task_now();

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
        char *p = (char *)memman_alloc_4k(memman, finfo->size);
        file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
        if (finfo->size >= 36 && strncmp(p + 4, "Hari", 4) == 0 && *p == 0x00) {
            int segsiz = *((int *)(p + 0x0000));
            int esp    = *((int *)(p + 0x000c));
            int datsiz = *((int *)(p + 0x0010));
            int datbin = *((int *)(p + 0x0014));
            char *q    = (char *)memman_alloc_4k(memman, segsiz);
            // *((int *)0xfe8) = (int)q;
            task->ds_base = (int)q;
            // set_segmdesc(gdt + 1003, finfo->size - 1, (int)p, AR_CODE32_ER + 0x60);
            // set_segmdesc(gdt + 1004, segsiz - 1, (int)q, AR_DATA32_RW + 0x60);
            set_segmdesc(gdt + task->sel / 8 + 1000, finfo->size - 1, (int)p, AR_CODE32_ER + 0x60);
            set_segmdesc(gdt + task->sel / 8 + 2000, segsiz - 1, (int)q, AR_DATA32_RW + 0x60);
            for (int i = 0; i < datsiz; i++) {
                q[esp + i] = p[datbin + i];
            }
            start_app(0x1b, task->sel + 1000 * 8, esp, task->sel + 2000 * 8, &(task->tss.esp0));
            struct SHTCTL *shtctl = (struct SHTCTL *)*((int *)0x0fe4);
            for (int i = 0; i < MAX_SHEETS; i++) {
                struct SHEET *sht = &(shtctl->sheets0[i]);
                if ((sht->flags & 0x11) == 0x11 && sht->task == task) {
                    /*アプリが開きっぱなしになった下敷きを発見*/
                    sheet_free(sht); /* 閉じる */
                }
            }
            timer_cancelall(&task->fifo);
            memman_free_4k(memman, (int)q, segsiz);
        } else {
            cons_putstr(cons, ".bin file format error");
        }
        memman_free_4k(memman, (int)p, finfo->size);
        cons_newline(cons);
        return 1;
    }
    /* ファイルが見つからない場合 */
    return 0;
}

int *bin_api(int edi, int esi, int ebp, int esp, int ebx, int edx, int ecx, int eax) {
    struct TASK *task    = task_now();
    int ds_base          = task->ds_base;
    struct CONSOLE *cons = task->cons;
    int *reg             = &eax + 1; /* eaxの次の番地 */

    if (edx == 1) {
        cons_putchar(cons, eax & 0xff, 1);
    } else if (edx == 2) {
        cons_putstr(cons, (char *)ebx + ds_base);
    } else if (edx == 3) {
        cons_putstr_len(cons, (char *)ebx + ds_base, ecx);
    } else if (edx == 4) {
        return &(task->tss.esp0);
    } else if (edx == 5) {
        struct SHTCTL *shtctl = (struct SHTCTL *)*((int *)0x0fe4);
        struct SHEET *sht     = sheet_alloc(shtctl);
        sht->task             = task;
        sht->flags |= 0x10;
        sheet_setbuf(sht, (char *)ebx + ds_base, esi, edi, eax);
        make_window8((char *)ebx + ds_base, esi, edi, (char *)ecx + ds_base, 0);
        sheet_slide(sht, ((shtctl->xsize - esi) / 2) & ~3, (shtctl->ysize - edi) / 2);
        sheet_updown(sht, shtctl->top);
        reg[7] = (int)sht;
    } else if (edx == 6) {
        struct SHEET *sht = (struct SHEET *)(ebx & 0xfffffffe);
        putfont8_asc(sht->buf, sht->bxsize, esi, edi, eax, (char *)ebp + ds_base);
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
        }
    } else if (edx == 7) {
        struct SHEET *sht = (struct SHEET *)(ebx & 0xfffffffe);
        boxfill8(sht->buf, sht->bxsize, ebp, eax, ecx, esi, edi);
        if ((ebx & 1) == 1) {
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
        }
    } else if (edx == 8) {
        memman_init((struct MEMMAN *)(ebx + ds_base));
        ecx &= 0xfffffff0;
        memman_free((struct MEMMAN *)(ebx + ds_base), eax, ecx);
    } else if (edx == 9) {
        ecx    = (ecx + 0x0f) & 0xfffffff0;
        reg[7] = memman_alloc((struct MEMMAN *)(ebx + ds_base), ecx);
    } else if (edx == 10) {
        ecx = (ecx * 0x0f) & 0xfffffff0;
        memman_free((struct MEMMAN *)(ebx + ds_base), eax, ecx);
    } else if (edx == 11) {
        struct SHEET *sht                 = (struct SHEET *)(ebx & 0xfffffffe);
        sht->buf[sht->bxsize * edi + esi] = eax;
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, esi, edi, esi + 1, edi + 1);
        }
    } else if (edx == 12) {
        struct SHEET *sht = (struct SHEET *)ebx;
        sheet_refresh(sht, eax, ecx, esi, edi);
    } else if (edx == 13) {
        struct SHEET *sht = (struct SHEET *)(ebx & 0xfffffffe);
        bin_api_linewin(sht, eax, ecx, esi, edi, ebp);
        if ((ebx & 1) == 0) {
            sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
        }
    } else if (edx == 14) {
        sheet_free((struct SHEET *)ebx);
    } else if (edx == 15) {
        while (1) {
            io_cli();
            if (fifo32_status(&task->fifo) == 0) {
                if (eax != 0) {
                    task_sleep(task);
                } else {
                    io_sti();
                    reg[7] = -1;
                    return 0;
                }
            }
            int data = fifo32_get(&task->fifo);
            io_sti();
            if (data <= 1) {
                timer_init(cons->timer, &task->fifo, 1);
                timer_settime(cons->timer, 50);
            }
            if (data == 2) {
                cons->cur_c = WHITE;
            }
            if (data == 3) {
                cons->cur_c = -1;
            }
            if (256 <= data && data <= 511) {
                reg[7] = data - 256;
                return 0;
            }
        }
    } else if (edx == 16) {
        reg[7]                           = (int)timer_alloc();
        ((struct TIMER *)reg[7])->flags2 = 1; /* 自動キャンセル有効 */
    } else if (edx == 17) {
        timer_init((struct TIMER *)ebx, &task->fifo, eax + 256);
    } else if (edx == 18) {
        timer_settime((struct TIMER *)ebx, eax);
    } else if (edx == 19) {
        timer_free((struct TIMER *)ebx);
    } else if (edx == 20) {
        int sound;
        if (eax == 0) {
            sound = io_in8(0x61);
            io_out8(0x61, sound & 0x0d);
        } else {
            sound = 1193180000 / eax;
            io_out8(0x43, 0xb6);
            io_out8(0x42, sound & 0xff);
            io_out8(0x42, sound >> 8);
            sound = io_in8(0x61);
            io_out8(0x61, (sound | 0x03) & 0x0f);
        }
    }

    return 0;
}

int *inthandler0c(int *esp) {
    struct TASK *task    = task_now();
    struct CONSOLE *cons = task->cons;
    cons_putstr(cons, "INT 0c :\n Stack Exception.");

    char s[30];
    sprintf(s, "eip = %x\n", esp[11]);
    cons_putstr(cons, s);

    return &(task->tss.esp0); /* 異常終了 */
}

int *inthandler0d(int *esp) {
    struct TASK *task    = task_now();
    struct CONSOLE *cons = task->cons;
    cons_putstr(cons, "INT 0d :\n General Prodtected Exception.");
    return &(task->tss.esp0); /* 異常終了させる */
}

void bin_api_linewin(struct SHEET *sht, int x0, int y0, int x1, int y1, int col) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int x  = x0 << 10;
    int y  = y0 << 10;

    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }

    int len;
    if (dx >= dy) {
        len = dx + 1;
        if (x0 > x1) {
            dx = -1024;
        } else {
            dx = 1024;
        }
        if (y0 <= y1) {
            dy = ((y1 - y0 + 1) << 10) / len;
        } else {
            dy = ((y1 - y0 - 1) << 10) / len;
        }
    } else {
        len = dy + 1;
        if (y0 > y1) {
            dy = -1024;
        } else {
            dy = 1024;
        }
        if (x0 <= x1) {
            dx = ((x1 - x0 + 1) << 10) / len;
        } else {
            dx = ((x1 - x0 - 1) << 10) / len;
        }
    }

    for (int i = 0; i < len; i++) {
        sht->buf[(y >> 10) * sht->bxsize + (x >> 10)] = col;
        x += dx;
        y += dy;
    }
}
