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

    timerctl.using ++;
    /*　動作中のタイマがこれ1つの場合 */
    if (timerctl.using == 1)
    {
        timerctl.t0 = timer;
        timer->next = 0;
        timerctl.next = timer->timeout;

        io_store_eflags(eflags);
        return;
    }

    struct TIMER *t = timerctl.t0;
    /*　先頭に入れる場合 */
    if (timer->timeout <= t->timeout)
    {
        timerctl.t0 = timer;
        timer->next = t;
        timerctl.next = timer->timeout;

        io_store_eflags(eflags);
        return;
    }

    struct TIMER *s;
    while (1)
    {
        s = t;
        t = t->next;
        if (t == 0)
        {
            break;
        }
        if (timer->timeout <= t->timeout)
        {
            s->next = timer;
            timer->next = t;

            io_store_eflags(eflags);
            return;
        }
    }

    s->next = timer;
    timer->next = 0;

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

    struct TIMER *timer = timerctl.t0;
    int index = 0;
    for (int i = 0; i < timerctl.using; i++)
    {
        if (timer->timeout > timerctl.count)
        {
            index = i;
            break;
        }
        /* タイムアウト */
        timer->flags = TIMER_FLAGS_ALLOC;
        fifo32_put(timer->fifo, timer->data);
        timer = timer->next;
    }

    /* ちょうどindex個のタイマがタムアウトしたので、残りをずらしている */
    timerctl.using -= index;

    timerctl.t0 = timer;

    if (timerctl.using > 0)
    {
        timerctl.next = timerctl.t0->timeout;
    }
    else
    {
        timerctl.next = 0xffffffff;
    }

    return;
}
