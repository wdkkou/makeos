#include "bootpack.h"

#define SHEET_USE 1

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{
    struct SHTCTL *ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));
    if (ctl == 0)
    {
        return 0;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;
    for (int i = 0; i < MAX_SHEETS; i++)
    {
        ctl->sheets0[i].flags = 0;
    }
    return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl)
{
    for (int i = 0; i < MAX_SHEETS; i++)
    {
        if (ctl->sheets0[i].flags == 0)
        {
            struct SHEET *sht = &ctl->sheets0[i];
            sht->flags = SHEET_USE;
            sht->height = -1;
            return sht;
        }
    }
    return 0;
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
    sht->buf = buf;
    sht->bxsize = xsize;
    sht->bysize = ysize;
    sht->col_inv = col_inv;
    return;
}

void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height)
{
    if (height > ctl->top + 1)
    {
        height = ctl->top + 1;
    }
    if (height < -1)
    {
        height = -1;
    }
    int old = sht->height;
    sht->height = height;

    /* sheets[] の並び替え */
    if (old > height) /* 以前よりも低くなる場合 */
    {
        if (height >= 0)
        {
            for (int h = old; h > height; h--)
            /*old - height 間のsheetを引き上げる */
            {
                ctl->sheets[h] = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        }
        else /* 非表示になる場合 */
        {
            if (ctl->top > old)
            /* oldよりも上のsheetをおろす */
            {
                for (int h = old; h < ctl->top; h++)
                {
                    ctl->sheets[h] = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--;
        }
        sheet_refresh(ctl);
    }
    else if (old < height) /* 以前よりも高くなる場合*/
    {
        if (old >= 0)
        {
            /*old - height 間のsheetを押し下げる */
            for (int h = old; h < height; h++)
            {
                ctl->sheets[h] = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        }
        else /* 非表示状態から表示状態へ */
        {
            for (int h = ctl->top; h >= height; h--)
            {
                ctl->sheets[h + 1] = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;
        }
        sheet_refresh(ctl);
    }
    return;
}

void sheet_refresh(struct SHTCTL *ctl)
{
    for (int h = 0; h <= ctl->top; h++)
    {
        unsigned char *vram = ctl->vram;
        struct SHEET *sht = ctl->sheets[h];
        unsigned char *buf = sht->buf;
        for (int by = 0; by < sht->bysize; by++)
        {
            int vy = sht->vy0 + by;
            for (int bx = 0; bx < sht->bxsize; bx++)
            {
                int vx = sht->vx0 + bx;
                unsigned char c = buf[by * sht->bxsize + bx];
                if (c != sht->col_inv)
                {
                    vram[vy * sht->bxsize + vx] = c;
                }
            }
        }
    }
    return;
}

void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0)
{
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0)
    {
        sheet_refresh(ctl);
    }
    return;
}

void sheet_free(struct SHTCTL *ctl, struct SHEET *sht)
{
    if (sht->height >= 0)
    {
        sheet_updown(ctl, sht, -1);
    }
    sht->flags = 0;
    return;
}
