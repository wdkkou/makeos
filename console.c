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

    /* プロンプト表示 */
    putfont8_asc_sht(sheet, 8, 28, WHITE, BLACK, "$ ", 2);

    char s[30];
    char cmdline[30];
    int cursor_x = 24, cursor_y = 28, cursor_c = -1;
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
                    if (cursor_c >= 0) {
                        cursor_c = WHITE;
                    }
                } else {
                    timer_init(timer, &task->fifo, 1);
                    if (cursor_c >= 0) {
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
            if (256 <= data &&
                data <= 511) { /* キーボードデータ（タスクA経由） */
                if (data == 8 + 256) {
                    /* バックスペース */
                    if (cursor_x > 24) {
                        /* カーソルをスペースで消してから、カーソルを1つ戻す */
                        putfont8_asc_sht(sheet, cursor_x, cursor_y, WHITE, BLACK, " ", 1);
                        cursor_x -= 8;
                    }
                } else if (data == 10 + 256) {
                    putfont8_asc_sht(sheet, cursor_x, cursor_y, WHITE, BLACK, " ", 1);
                    cmdline[cursor_x / 8 - 3] = 0;
                    cursor_y                  = cons_newline(cursor_y, sheet);
                    /* コマンド実行 */
                    if (strcmp(cmdline, "mem") == 0) {
                        sprintf(s, "total %dMB", memtotal / (1024 * 1024));
                        putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                        sprintf(s, "free %dKB", memman_total(memman) / 1024);
                        putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, s, 30);
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (strcmp(cmdline, "clear") == 0) {
                        for (int y = 28; y < 28 + 128; y++) {
                            for (int x = 8; x < 8 + 240; x++) {
                                sheet->buf[x + y * sheet->bxsize] = BLACK;
                            }
                        }
                        sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
                        cursor_y = 28;
                    } else if (strcmp(cmdline, "ls") == 0) {
                        for (int x = 0; x < 224; x++) {
                            if (finfo[x].name[0] == 0x00) {
                                break;
                            }
                            if (finfo[x].name[0] != 0xe5) {
                                if ((finfo[x].type & 0x18) == 0) {
                                    sprintf(s, "filename.ext %d",
                                            finfo[x].size);
                                    for (int y = 0; y < 8; y++) {
                                        s[y] = finfo[x].name[y];
                                    }
                                    s[9]  = finfo[x].ext[0];
                                    s[10] = finfo[x].ext[1];
                                    s[11] = finfo[x].ext[2];
                                    putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, s, 30);
                                    cursor_y = cons_newline(cursor_y, sheet);
                                }
                            }
                        }
                    } else if (strncmp(cmdline, "cat ", 4) == 0) {
                        /* catコマンド */
                        for (int y = 0; y < 11; y++) {
                            s[y] = ' ';
                        }
                        for (int x = 4, y = 0; y < 11 && cmdline[x] != 0; x++, y++) {
                            if (cmdline[x] == '.' && y <= 8) {
                                y = 7;
                            } else {
                                s[y] = cmdline[x];
                                if ('a' <= s[y] && s[y] <= 'z') {
                                    /* 小文字を大文字に変換 */
                                    s[y] -= 0x20;
                                }
                            }
                        }

                        /* ファイルを探す */
                        int exist_file = 0;
                        int file_x     = 224;
                        for (int x = 0; x < 224; x++) {
                            if (finfo[x].name[0] == 0x00) {
                                exist_file = 0;
                                file_x     = x;
                                break;
                            }
                            if ((finfo[x].type & 0x18) == 0) {
                                int ok = 1;
                                for (int y = 0; y < 11; y++) {
                                    if (finfo[x].name[y] != s[y]) {
                                        ok = 0;
                                        break;
                                    }
                                }
                                if (ok) {
                                    /* ファイルが見つかった */
                                    exist_file = 1;
                                    file_x     = x;
                                    break;
                                }
                            }
                        }
                        if (exist_file) {
                            /* ファイルが見つかった場合 */
                            int y   = finfo[file_x].size;
                            char *p = (char *)memman_alloc_4k(memman, finfo[file_x].size);
                            file_loadfile(finfo[file_x].clustno, finfo[file_x].size, p,
                                          fat, (char *)(ADR_DISKIMG + 0x003e00));
                            cursor_x = 8;
                            for (int y = 0; y < finfo[file_x].size; y++) {
                                s[0] = p[y];
                                s[1] = 0;
                                if (s[0] == 0x09) {
                                    while (1) {
                                        putfont8_asc_sht(sheet, cursor_x, cursor_y, WHITE, BLACK, " ", 1);
                                        cursor_x += 8;
                                        if (cursor_x == 8 + 240) {
                                            cursor_x = 8;
                                        }
                                        if (((cursor_x - 8) & 0x1f) == 0) {
                                            break; /* 32で割り切れたらbreak */
                                        }
                                    }
                                } else if (s[0] == 0x0a) {
                                    cursor_x = 8;
                                } else if (s[0] == 0x0d) {
                                    /* とりあえず何もしない */
                                } else {
                                    putfont8_asc_sht(sheet, cursor_x, cursor_y, WHITE, BLACK, s, 1);
                                    cursor_x += 8;
                                    if (cursor_x == 8 + 240) {
                                        cursor_x = 8;
                                        cursor_y =
                                            cons_newline(cursor_y, sheet);
                                    }
                                }
                            }
                            memman_free_4k(memman, (int)p, finfo[file_x].size);
                        } else {
                            /* ファイルが見つからなかった場合 */
                            putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, "fIle not found.", 15);
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (strcmp(cmdline, "hlt") == 0) {
                        /* catコマンド */
                        for (int y = 0; y < 11; y++) {
                            s[y] = ' ';
                        }
                        s[0]  = 'H';
                        s[1]  = 'L';
                        s[2]  = 'T';
                        s[8]  = 'B';
                        s[9]  = 'I';
                        s[10] = 'N';
                        /* ファイルを探す */
                        int exist_file = 0;
                        int file_x     = 224;
                        for (int x = 0; x < 224; x++) {
                            if (finfo[x].name[0] == 0x00) {
                                exist_file = 0;
                                file_x     = x;
                                break;
                            }
                            if ((finfo[x].type & 0x18) == 0) {
                                int ok = 1;
                                for (int y = 0; y < 11; y++) {
                                    if (finfo[x].name[y] != s[y]) {
                                        ok = 0;
                                        break;
                                    }
                                }
                                if (ok) {
                                    /* ファイルが見つかった */
                                    exist_file = 1;
                                    file_x     = x;
                                    break;
                                }
                            }
                        }
                        if (exist_file) {
                            /* ファイルが見つかった場合 */
                            char *p = (char *)memman_alloc_4k(memman, finfo[file_x].size);
                            file_loadfile(finfo[file_x].clustno, finfo[file_x].size, p,
                                          fat, (char *)(ADR_DISKIMG + 0x003e00));
                            set_segmdesc(gdt + 1003, finfo[file_x].size - 1, (int)p, AR_CODE32_ER);
                            farjmp(0, 1003 * 8);
                            memman_free_4k(memman, (int)p, finfo[file_x].size);
                        } else {
                            /* ファイルが見つからなかった場合 */
                            putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, "file not found.", 15);
                        }
                        cursor_y = cons_newline(cursor_y, sheet);
                    } else if (cmdline[0] != 0) {
                        putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, "command error", 15);
                        cursor_y = cons_newline(cursor_y, sheet);
                    }
                    /* プロンプト表示 */
                    putfont8_asc_sht(sheet, 8, cursor_y, WHITE, BLACK, "$ ", 2);
                    cursor_x = 24;
                } else {
                    /* 一般文字 */
                    if (cursor_x < 240) {
                        /* 一文字表示してから、カーソルを1つ進める */
                        s[0]                      = data - 256;
                        s[1]                      = 0;
                        cmdline[cursor_x / 8 - 3] = data - 256;
                        putfont8_asc_sht(sheet, cursor_x, cursor_y, WHITE, BLACK, s, 1);
                        cursor_x += 8;
                    }
                }
            }
            /* カーソル再表示 */
            if (cursor_c >= 0) {
                boxfill8(sheet->buf, sheet->bxsize, cursor_c, cursor_x,
                         cursor_y, cursor_x + 7, cursor_y + 15);
            }
            sheet_refresh(sheet, cursor_x, cursor_y, cursor_x + 8, cursor_y + 16);
        }
    }
}
int cons_newline(int cursor_y, struct SHEET *sheet) {
    if (cursor_y < 28 + 112) {
        cursor_y += 16;
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
    return cursor_y;
}
