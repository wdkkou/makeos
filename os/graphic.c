#include "bootpack.h"

void init_palette(void) {
    static unsigned char table_rgb[16 * 3] = {
        0x00, 0x00, 0x00,  // 000000 : 0 : 黒
        0xff, 0x00, 0x00,  // ff0000 : 1 : 明るい赤
        0x00, 0xff, 0x00,  // 00ff00 : 2 : 明るい緑
        0xff, 0xff, 0x00,  // ffff00 : 3 : 黄色
        0x00, 0x00, 0xff,  // 0000ff : 4 : 明るい青
        0xff, 0x00, 0xff,  // ff00ff : 5 : 明るい紫
        0x00, 0xff, 0xff,  // 00ffff : 6 : 明るい水色
        0xff, 0xff, 0xff,  // ffffff : 7 : 白
        0xc6, 0xc6, 0xc6,  // c6c6c6 : 8 : 明るい灰色
        0x84, 0x00, 0x00,  // 840000 : 9 : 暗い赤
        0x00, 0x84, 0x00,  // 008400 : 10: 暗い緑
        0x84, 0x84, 0x00,  // 848400 : 11: 暗い黄色
        0x00, 0x00, 0x84,  // 000084 : 12: 暗い青
        0x84, 0x00, 0x84,  // 840084 : 13: 暗い紫
        0x00, 0x84, 0x84,  // 008484 : 14: 暗い水色
        0x84, 0x84, 0x84,  // 848484 : 15: 暗い灰色
    };
    set_palette(0, 15, table_rgb);
    unsigned char table2[216 * 3];
    for (int b = 0; b < 6; b++) {
        for (int g = 0; g < 6; g++) {
            for (int r = 0; r < 6; r++) {
                table2[(r + g * 6 + b * 36) * 3 + 0] = r * 51;
                table2[(r + g * 6 + b * 36) * 3 + 1] = g * 51;
                table2[(r + g * 6 + b * 36) * 3 + 2] = b * 51;
            }
        }
    }
    set_palette(16, 231, table2);
    return;
}
void set_palette(int start, int end, unsigned char *rgb) {
    int i, eflags;
    eflags = io_load_eflags();  // 割り込み許可フラグの値を記録
    io_cli();                   // 許可フラグを0にして割り込みを禁止する
    io_out8(0x03c8, start);
    for (i = start; i <= end; i++) {
        io_out8(0x03c9, rgb[0] / 4);
        io_out8(0x03c9, rgb[1] / 4);
        io_out8(0x03c9, rgb[2] / 4);
        rgb += 3;
    }
    io_store_eflags(eflags);  // 割り込み許可フラグを元にもどす
    return;
}

void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1) {
    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            vram[y * xsize + x] = c;
        }
    }
}
void init_screen(char *vram, int x, int y) {
    boxfill8(vram, x, COL8_008400, 0, 0, x - 1, y - 29);
    boxfill8(vram, x, COL8_C6C6C6, 0, y - 28, x - 1, y - 28);
    boxfill8(vram, x, WHITE, 0, y - 27, x - 1, y - 27);
    boxfill8(vram, x, COL8_C6C6C6, 0, y - 26, x - 1, y - 1);

    boxfill8(vram, x, WHITE, 3, y - 24, 59, y - 24);
    boxfill8(vram, x, WHITE, 2, y - 24, 2, y - 4);
    boxfill8(vram, x, COL8_848484, 3, y - 4, 59, y - 4);
    boxfill8(vram, x, COL8_848484, 59, y - 23, 59, y - 5);
    boxfill8(vram, x, BLACK, 2, y - 3, 59, y - 3);
    boxfill8(vram, x, BLACK, 60, y - 24, 60, y - 3);

    boxfill8(vram, x, COL8_848484, x - 47, y - 24, x - 4, y - 24);
    boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y - 4);
    boxfill8(vram, x, WHITE, x - 47, y - 3, x - 4, y - 3);
    boxfill8(vram, x, WHITE, x - 3, y - 24, x - 3, y - 3);
    return;
}

void putfont8(char *vram, int xsize, int x, int y, char c, char *font) {
    char *p, d;
    for (int i = 0; i < 16; i++) {
        p = vram + (y + i) * xsize + x;
        d = font[i];
        if ((d & 0x80) != 0) {
            p[0] = c;
        }
        if ((d & 0x40) != 0) {
            p[1] = c;
        }
        if ((d & 0x20) != 0) {
            p[2] = c;
        }
        if ((d & 0x10) != 0) {
            p[3] = c;
        }
        if ((d & 0x08) != 0) {
            p[4] = c;
        }
        if ((d & 0x04) != 0) {
            p[5] = c;
        }
        if ((d & 0x02) != 0) {
            p[6] = c;
        }
        if ((d & 0x01) != 0) {
            p[7] = c;
        }
    }
    return;
}

void putfont8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s) {
    extern char hankaku[4096];
    struct TASK *task = task_now();
    char *nihongo     = (char *)*((int *)0x0fe8);
    if (task->langmode == 0) {
        for (; *s != 0x00; s++) {
            putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
            x += 8;
        }
    }
    int k, t;
    if (task->langmode == 1) {
        for (; *s != 0x00; s++) {
            if (task->langbyte1 == 0) {
                if ((0x81 <= *s && *s <= 0x9f) || (0xe0 <= *s && *s <= 0xfc)) {
                    task->langbyte1 = *s;
                } else {
                    putfont8(vram, xsize, x, y, c, nihongo + *s * 16);
                }
            } else {
                if (0x81 <= task->langbyte1 && task->langbyte1 <= 0x9f) {
                    k = (task->langbyte1 - 0x81) * 2;
                } else {
                    k = (task->langbyte1 - 0xe0) * 2 + 62;
                }
                if (0x40 <= *s && *s <= 0x7e) {
                    t = *s - 0x40;
                } else if (0x80 <= *s && *s <= 0x9e) {
                    t = *s - 0x80 + 63;
                } else {
                    t = *s - 0x9f;
                    k++;
                }
                task->langbyte1 = 0;
                char *font      = nihongo + 256 * 16 + (k * 94 + t) * 32;
                putfont8(vram, xsize, x - 8, y, c, font);
                putfont8(vram, xsize, x, y, c, font + 16);
            }
            x += 8;
        }
    }
    if (task->langmode == 2) {
        for (; *s != 0x00; s++) {
            if (task->langbyte1 == 0) {
                if (0x81 <= *s && *s <= 0xfe) {
                    task->langbyte1 = *s;
                } else {
                    putfont8(vram, xsize, x, y, c, nihongo + *s * 16);
                }
            } else {
                k               = task->langbyte1 - 0xa1;
                t               = *s - 0xa1;
                task->langbyte1 = 0;
                char *font      = nihongo + 256 * 16 + (k * 94 + t) * 32;
                putfont8(vram, xsize, x - 8, y, c, font);
                putfont8(vram, xsize, x, y, c, font + 16);
            }
            x += 8;
        }
    }
    return;
}
void init_mouse_cursor8(char *mouse, char bc) {
    static char cursor[16][16] = {
        "**************..",  // 1
        "*ooooooooooo*...",  // 2
        "*oooooooooo*....",  // 3
        "*ooooooooo*.....",  // 4
        "*oooooooo*......",  // 5
        "*ooooooo*.......",  // 6
        "*ooooooo*.......",  // 7
        "*oooooooo*......",  // 8
        "*oooo**ooo*.....",  // 9
        "*ooo*..*ooo*....",  // 10
        "*oo*....*ooo*...",  // 11
        "*o*......*ooo*..",  // 12
        "**........*ooo*.",  // 13
        "*..........*ooo*",  // 14
        "............*oo*",  // 15
        ".............***",  // 12
    };
    int x, y;

    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            if (cursor[y][x] == '*') {
                mouse[y * 16 + x] = BLACK;
            }
            if (cursor[y][x] == 'o') {
                mouse[y * 16 + x] = WHITE;
            }
            if (cursor[y][x] == '.') {
                mouse[y * 16 + x] = bc;
            }
        }
    }
    return;
}

void putblock8_8(char *vram, int vxsize, int pxsize, int pysize, int px0, int py0, char *buf, int bxsize) {
    for (int y = 0; y < pysize; y++) {
        for (int x = 0; x < pxsize; x++) {
            vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
        }
    }
    return;
}
