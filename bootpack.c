#include "bootpack.h"

#define MEM_FREES 4090 /* 約32GB */
#define MEM_ADDR 0x003c0000

/* 空き情報 */
struct FREEINFO
{
    unsigned int addr, size;
};
/* メモリ管理 */
struct MEMMAN
{
    int frees, maxfrees, lostsize, losts;
    struct FREEINFO free[MEM_FREES];
};
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

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

    struct MOUSE_DEC mdec;
    enable_mouse(&mdec);

    init_palette();
    init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
    init_mouse_cursor8(mcursor, COL8_008400);
    int mx = (binfo->scrnx - 16) / 2;
    int my = (binfo->scrny - 28 - 16) / 2;
    putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
    //sprintf(s, "(%d, %d)", mx, my);
    //putfont8_asc(binfo->vram, binfo->scrnx, 16, 64, WHITE, s);
    // putfont8_asc(binfo->vram, binfo->scrnx, 8, 8, WHITE, "WDK");
    // putfont8_asc(binfo->vram, binfo->scrnx, 30, 30, WHITE, "oreore OS");

    struct MEMMAM *memman = (struct MEMMAN *)MEM_ADDR;
    unsigned int memtotal = memtest(0x00400000, 0xbfffffff);
    memman_init(memman);
    memman_free(memman, 0x00001000, 0x0009e000);
    memman_free(memman, 0x00400000, memtotal - 0x00400000);
    sprintf(s, "memory = %dMB , free = %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
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

void memman_init(struct MEMMAN *man)
{
    man->frees = 0;    /* 空き情報の個数 */
    man->maxfrees = 0; /* freesの最大値 */
    man->lostsize = 0; /* 解放に失敗した合計サイズ*/
    man->losts = 0;    /* 解放に失敗した回数　*/
    return;
}
/* 空きサイズの合計 */
unsigned int memman_total(struct MEMMAN *man)
{
    unsigned total = 0;
    for (unsigned int i = 0; i < man->frees; i++)
    {
        total += man->free[i].size;
    }
    return total;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
    for (unsigned int i = 0; i < man->frees; i++)
    {
        /* 十分な空きを発見 */
        if (man->free[i].size >= size)
        {
            unsigned int a = man->free[i].addr;
            man->free[i].addr += size;
            man->free[i].size -= size;
            if (man->free[i].size == 0)
            {
                /* free[i]がなくなれば、前へ詰める */
                man->frees--;
                for (; i < man->frees; i++)
                {
                    man->free[i] = man->free[i + 1];
                }
            }
            return a;
        }
    }
    return 0; /* 空きがない */
}
/* メモリの解放　*/
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i;
    for (i = 0; i < man->frees; i++)
    {
        if (man->free[i].addr > addr)
        {
            break;
        }
    }
    /* free[i-1].addr < addr < free[i].addrの状態 */
    if (i > 0)
    {
        /* 前が存在 */
        if (man->free[i - 1].addr + man->free[i - 1].size == addr)
        {
            /* 前の空き情報とまとめる */
            man->free[i - 1].size += size;
            /* 後ろも存在 */
            if (i < man->frees)
            {
                if (addr + size == man->free[i].addr)
                {
                    /*後ろもまとめる*/
                    man->free[i - 1].size += man->free[i].size;
                    man->frees--;
                    for (; i < man->frees; i++)
                    {
                        man->free[i] = man->free[i + 1];
                    }
                }
            }
            return 0; /* 成功終了 */
        }
    }
    if (i < man->frees)
    {
        /* 後ろの空き領域とまとめる */
        if (addr + size == man->free[i].addr)
        {
            man->free[i].addr = addr;
            man->free[i].size += size;
            return 0;
        }
    }
    if (man->frees < MEM_FREES)
    {
        /* free[i]より後ろのfreeを後ろへずらす */
        for (int j = man->frees; j > i; j--)
        {
            man->free[j] = man->free[j - 1];
        }
        man->frees++;
        if (man->frees > man->maxfrees)
        {
            man->maxfrees = man->frees;
        }
        man->free[i].addr = addr;
        man->free[i].size = size;
        return 0;
    }

    man->losts++;
    man->lostsize += size;
    return -1;
}
