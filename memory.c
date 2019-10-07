#include "bootpack.h"

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

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
    size = (size + 0xfff) & 0xfffff000;
    unsigned a = memman_alloc(man, size);
    return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    size = (size + 0xfff) & 0xfffff000;
    int i = memman_free(man, addr, size);
    return i;
}
