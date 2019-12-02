#include "bootpack.h"

#define SHEET_USE 1

struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize) {
    struct SHTCTL *ctl = (struct SHTCTL *)memman_alloc_4k(memman, sizeof(struct SHTCTL));
    if (ctl == 0) {
        return 0;
    }
    ctl->map = (unsigned char *)memman_alloc_4k(memman, xsize * ysize);
    if (ctl->map == 0) {
        memman_free_4k(memman, (int)ctl, sizeof(struct SHTCTL));
        return ctl;
    }
    ctl->vram  = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top   = -1;
    for (int i = 0; i < MAX_SHEETS; i++) {
        ctl->sheets0[i].flags = 0;   /* 未使用*/
        ctl->sheets0[i].ctl   = ctl; /* 所属*/
    }
    return ctl;
}

struct SHEET *sheet_alloc(struct SHTCTL *ctl) {
    for (int i = 0; i < MAX_SHEETS; i++) {
        if (ctl->sheets0[i].flags == 0) {
            struct SHEET *sht = &ctl->sheets0[i];
            sht->flags        = SHEET_USE;
            sht->height       = -1;
            sht->task         = 0;
            return sht;
        }
    }
    return 0;
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv) {
    sht->buf     = buf;
    sht->bxsize  = xsize;
    sht->bysize  = ysize;
    sht->col_inv = col_inv;
    return;
}

void sheet_updown(struct SHEET *sht, int height) {
    struct SHTCTL *ctl = sht->ctl;
    if (height > ctl->top + 1) {
        height = ctl->top + 1;
    }
    if (height < -1) {
        height = -1;
    }
    int old     = sht->height;
    sht->height = height;

    /* sheets[] の並び替え */
    if (old > height) /* 以前よりも低くなる場合 */
    {
        if (height >= 0) {
            for (int h = old; h > height; h--)
            /*old - height 間のsheetを引き上げる */
            {
                ctl->sheets[h]         = ctl->sheets[h - 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
            sheet_refreshmap(ctl, sht->vx0, sht->vy0,
                             sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
            sheet_refreshsub(ctl, sht->vx0, sht->vy0,
                             sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);
        } else /* 非表示になる場合 */
        {
            if (ctl->top > old)
            /* oldよりも上のsheetをおろす */
            {
                for (int h = old; h < ctl->top; h++) {
                    ctl->sheets[h]         = ctl->sheets[h + 1];
                    ctl->sheets[h]->height = h;
                }
            }
            ctl->top--;
        }
        sheet_refreshmap(ctl, sht->vx0, sht->vy0,
                         sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0,
                         sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
    } else if (old < height) /* 以前よりも高くなる場合*/
    {
        if (old >= 0) {
            /*old - height 間のsheetを押し下げる */
            for (int h = old; h < height; h++) {
                ctl->sheets[h]         = ctl->sheets[h + 1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        } else /* 非表示状態から表示状態へ */
        {
            for (int h = ctl->top; h >= height; h--) {
                ctl->sheets[h + 1]         = ctl->sheets[h];
                ctl->sheets[h + 1]->height = h + 1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;
        }
        sheet_refreshmap(ctl, sht->vx0, sht->vy0,
                         sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
        sheet_refreshsub(ctl, sht->vx0, sht->vy0,
                         sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
    }
    return;
}

void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1) {
    if (sht->height >= 0) {
        sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0,
                         sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
    }
    return;
}
void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0, int h1) {
    /* refresh範囲が画面外にはみ出していれば補正 */
    if (vx0 < 0) {
        vx0 = 0;
    }
    if (vy0 < 0) {
        vy0 = 0;
    }
    if (vx1 > ctl->xsize) {
        vx1 = ctl->xsize;
    }
    if (vy1 > ctl->ysize) {
        vy1 = ctl->ysize;
    }
    for (int h = h0; h <= h1; h++) {
        unsigned char *vram = ctl->vram, *map = ctl->map;
        struct SHEET *sht  = ctl->sheets[h];
        unsigned char *buf = sht->buf;
        unsigned sid       = sht - ctl->sheets0;

        int bx0 = vx0 - sht->vx0;
        int by0 = vy0 - sht->vy0;
        int bx1 = vx1 - sht->vx0;
        int by1 = vy1 - sht->vy0;

        if (bx0 < 0) {
            bx0 = 0;
        }
        if (by0 < 0) {
            by0 = 0;
        }

        if (bx1 > sht->bxsize) {
            bx1 = sht->bxsize;
        }
        if (by1 > sht->bysize) {
            by1 = sht->bysize;
        }

        for (int by = by0; by < by1; by++) {
            int vy = sht->vy0 + by;
            for (int bx = bx0; bx < bx1; bx++) {
                int vx = sht->vx0 + bx;
                if (map[vy * ctl->xsize + vx] == sid) {
                    vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
                }
            }
        }
    }
    return;
}

void sheet_refreshmap(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0) {
    /* refresh範囲が画面外にはみ出していれば補正 */
    if (vx0 < 0) {
        vx0 = 0;
    }
    if (vy0 < 0) {
        vy0 = 0;
    }
    if (vx1 > ctl->xsize) {
        vx1 = ctl->xsize;
    }
    if (vy1 > ctl->ysize) {
        vy1 = ctl->ysize;
    }
    for (int h = h0; h <= ctl->top; h++) {
        struct SHEET *sht  = ctl->sheets[h];
        unsigned char sid  = sht - ctl->sheets0;
        unsigned char *buf = sht->buf, *map = ctl->map;
        int bx0 = vx0 - sht->vx0;
        int by0 = vy0 - sht->vy0;
        int bx1 = vx1 - sht->vx0;
        int by1 = vy1 - sht->vy0;
        if (bx0 < 0) {
            bx0 = 0;
        }
        if (by0 < 0) {
            by0 = 0;
        }
        if (bx1 > sht->bxsize) {
            bx1 = sht->bxsize;
        }
        if (by1 > sht->bysize) {
            by1 = sht->bysize;
        }
        if (sht->col_inv == -1) {
            /* 透明色なし用 高速版*/
            for (int by = by0; by < by1; by++) {
                int vy = sht->vy0 + by;
                for (int bx = bx0; bx < bx1; bx++) {
                    int vx                    = sht->vx0 + bx;
                    map[vy * ctl->xsize + vx] = sid;
                }
            }
        } else {
            for (int by = by0; by < by1; by++) {
                int vy = sht->vy0 + by;
                for (int bx = bx0; bx < bx1; bx++) {
                    int vx = sht->vx0 + bx;
                    if (buf[by * sht->bxsize + bx] != sht->col_inv) {
                        map[vy * ctl->xsize + vx] = sid;
                    }
                }
            }
        }
    }
    return;
}
void sheet_slide(struct SHEET *sht, int vx0, int vy0) {
    struct SHTCTL *ctl = sht->ctl;
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
    sht->vx0 = vx0;
    sht->vy0 = vy0;
    if (sht->height >= 0) {
        sheet_refreshmap(ctl, old_vx0, old_vy0,
                         old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
        sheet_refreshmap(ctl, vx0, vy0,
                         vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);

        sheet_refreshsub(ctl, old_vx0, old_vy0,
                         old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
        sheet_refreshsub(ctl, vx0, vy0,
                         vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
    }
    return;
}

void sheet_free(struct SHEET *sht) {
    if (sht->height >= 0) {
        sheet_updown(sht, -1);
    }
    sht->flags = 0;
    return;
}
