/* fifo ライブラリ */

#include "bootpack.h"

#define FLAGS_OVERRUN 0x0001
/* FIFOの初期化 */
void fifo32_init(struct FIFO32 *fifo, int size, int *buf)
{
    fifo->size = size;
    fifo->buf = buf;
    fifo->free = size;
    fifo->flags = 0;
    fifo->p = 0; /* 書き込み位置 */
    fifo->q = 0; /* 読み込み位置 */
    return;
}
/* FIFOにデータを渡し，蓄える */
int fifo32_put(struct FIFO32 *fifo, int data)
{
    /* バッファの空きがない際には-1を返す */
    if (fifo->free == 0)
    {
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if (fifo->p == fifo->size)
    {
        fifo->p = 0;
    }
    fifo->free--;
    return 0;
}

/* FIFOからデータを1つ取り出す */
int fifo32_get(struct FIFO32 *fifo)
{
    /* バッファのデータが空っぽの際には-1を返す */
    if (fifo->free == fifo->size)
    {
        return -1;
    }
    int data = fifo->buf[fifo->q];
    fifo->q++;
    if (fifo->q == fifo->size)
    {
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}
/* 溜まっているデータの数 */
int fifo32_status(struct FIFO32 *fifo)
{
    return fifo->size - fifo->free;
}
