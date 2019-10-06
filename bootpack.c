#include "bootpack.h"
unsigned int memtest(unsigned int start, unsigned int end);
unsigned int memtest_sub(unsigned int start, unsigned int end);

void HariMain(void)
{
    struct BOOTINFO *binfo = (struct BOOTINFO *)0xff0;
    char s[40], mcursor[256], keybuf[32], mousebuf[128];

    init_gdtidt();
    init_pic();
    io_sti(); /* IDTとPICの初期化が終わったのでCPUの割り込み禁止を解除 */

    fifo8_init(&keyfifo, 32, keybuf);
    fifo8_init(&mousefifo, 128, mousebuf);

    io_out8(PIC0_IMR, 0xf9); /* PIC1とキーボードを許可 */
    io_out8(PIC1_IMR, 0xef); /* マウスを許可 */

    init_keyboard();

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);

    // putfont8_asc(binfo->vram, binfo->scrnx, 8, 8, WHITE, "WDK");
    // putfont8_asc(binfo->vram, binfo->scrnx, 30, 30, WHITE, "oreore OS");

    init_mouse_cursor8(mcursor, COL8_008400);
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    //sprintf(s, "(%d, %d)", mx, my);
    //putfont8_asc(binfo->vram, binfo->scrnx, 16, 64, WHITE, s);

    struct MOUSE_DEC mdec;

    enable_mouse(&mdec);

    int x = memtest(0x00400000, 0xbfffffff) / (1024 * 1024);
    sprintf(s, "memory = %dMB", x);
    putfont8_asc(binfo->vram, binfo->scrnx, 0, 50, WHITE, s);
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
                sprintf(s, "%x", i);
                boxfill8(binfo->vram, binfo->scrnx, COL8_008400, 0, 16, 15, 31);
                putfont8_asc(binfo->vram, binfo->scrnx, 0, 16, WHITE, s);
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
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008400, 32, 32, 32 + 15 * 8 - 1, 47);
                    putfont8_asc(binfo->vram, binfo->scrnx, 32, 32, WHITE, s);
                    /* マウスカーソルの移動 */
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008400, mx, my, mx + 15, my + 15);
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
                    sprintf(s, "(%d %d)", mx, my);
                    boxfill8(binfo->vram, binfo->scrnx, COL8_008400, 0, 0, 79, 15);      /* 座標消す */
                    putfont8_asc(binfo->vram, binfo->scrnx, 0, 0, WHITE, s);             /* 座標書く */
                    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16); /* マウス描く */
                }
            }
        }
    }
}

#define EFLAGS_AC_BIT 0x00040000
#define CR0_CACHE_DISABLE 0x60000000
unsigned int memtest(unsigned int start, unsigned int end)
{
    int flg486 = 0;
    unsigned int eflg, cr0, i;

    eflg = io_load_eflags();
    eflg |= EFLAGS_AC_BIT;
    io_store_eflags(eflg);
    eflg = io_load_eflags();
    if ((eflg & EFLAGS_AC_BIT) != 0)
    {
        flg486 = 1;
    }
    eflg &= ~EFLAGS_AC_BIT;
    io_store_eflags(eflg);

    if (flg486 != 0)
    {
        cr0 = load_cr0();
        cr0 |= CR0_CACHE_DISABLE; /* キャッシュ禁止 */
        store_cr0(cr0);
    }

    i = memtest_sub(start, end);

    if (flg486 != 0)
    {
        cr0 = load_cr0();
        cr0 &= ~CR0_CACHE_DISABLE; /* キャッシュ許可*/
        store_cr0(cr0);
    }

    return i;
}

// unsigned int memtest_sub(unsigned int start, unsigned int end)
// {
//     unsigned int i, *p, old, pat0 = 0xaa55aa55, pat1 = 0x55aa55aa;
//     for (i = start; i <= end; i += 0x1000)
//     {
//         p = (unsigned int *)(i + 0xffc);
//         old = *p;
//         /* いじる前の値を覚えておく */
//         *p = pat0;
//         /* ためしに書いてみる */
//         *p ^= 0xffffffff;
//         /* そしてそれを反転してみる */
//         if (*p != pat1)
//         {
//             /* 反転結果になったか？ */
//         not_memory:
//             *p = old;
//             break;
//         }
//         *p ^= 0xffffffff;
//         /* もう一度反転してみる */
//         if (*p != pat0)
//         {
//             /* 元に戻ったか？ */
//             goto not_memory;
//         }
//         *p = old;
//         /* いじった値を元に戻す */
//     }
//     return i;
// }
