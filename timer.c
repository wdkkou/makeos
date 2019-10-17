#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define TIMER_FLAGS_ALLOC 1 /* 確保した状態 */
#define TIMER_FLAGS_USING 2 /* タイマ作動状態 */

struct TIMERCTL timerctl;

void init_pit(void)
{
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);

    timerctl.count = 0;
    timerctl.next = 0xffffffff; /* 最初は作動中のタイマが存在しない */
    timerctl.using = 0;

    for (int i = 0; i < MAX_TIMES; i++)
    {
        timerctl.timers0[i].flags = 0;
    }

    return;
}

struct TIMER *timer_alloc(void)
{
    for (int i = 0; i < MAX_TIMES; i++)
    {
        if (timerctl.timers0[i].flags == 0)
        {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return 0;
}

void timer_free(struct TIMER *timer)
{
    timer->flags = 0;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data)
{
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    int eflags = io_load_eflags();
    io_cli();
    /* どこに入れるべきか探す */
    int index = 0;
    for (int i = 0; i < timerctl.using; i++)
    {
        if (timerctl.timers[i]->timeout >= timer->timeout)
        {
            index = i;
            break;
        }
    }

    /* 後ろにずらしている */
    for (int j = timerctl.using; j > index; j--)
    {
        timerctl.timers[j] = timerctl.timers[j - 1];
    }
    timerctl.using ++;
    /* 空いた場所にtimerを入れる */
    timerctl.timers[index] = timer;
    timerctl.next = timerctl.timers[0]->timeout;

    io_store_eflags(eflags);

    return;
}

void inthandler20(int *esp)
{
    /* IRQ-00受付完了をPICに通知 */
    io_out8(PIC0_OCW2, 0x60);

    timerctl.count++;

    if (timerctl.next > timerctl.count)
    {
        /* まだ次の時刻になっていいないため、終了 */
        return;
    }

    int index = 0;
    for (int i = 0; i < timerctl.using; i++)
    {
        if (timerctl.timers[i]->timeout > timerctl.count)
        {
            index = i;
            break;
        }
        /* タイムアウト */
        timerctl.timers[i]->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timerctl.timers[i]->fifo, timerctl.timers[i]->data);
    }

    /* ちょうどindex個のタイマがタムアウトしたので、残りをずらしている */
    timerctl.using -= index;
    for (int j = 0; j < timerctl.using; j++)
    {
        timerctl.timers[j] = timerctl.timers[index + j];
    }

    if (timerctl.using > 0)
    {
        timerctl.next = timerctl.timers[0]->timeout;
    }
    else
    {
        timerctl.next = 0xffffffff;
    }

    return;
}
