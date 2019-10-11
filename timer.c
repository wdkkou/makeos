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
    for (int i = 0; i < MAX_TIMES; i++)
    {
        timerctl.timer[i].flags = 0;
    }

    return;
}

struct TIMER *timer_alloc(void)
{
    for (int i = 0; i < MAX_TIMES; i++)
    {
        if (timerctl.timer[i].flags == 0)
        {
            timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timer[i];
        }
    }
    return 0;
}

void timer_free(struct TIMER *timer)
{
    timer->flags = 0;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO8 *fifo, unsigned char data)
{
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout)
{
    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;
    return;
}

void inthandler20(int *esp)
{
    /* IRQ-00受付完了をPICに通知 */
    io_out8(PIC0_OCW2, 0x60);

    timerctl.count++;

    for (int i = 0; i < MAX_TIMES; i++)
    {
        if (timerctl.timer[i].flags == TIMER_FLAGS_USING)
        {
            if (timerctl.timer[i].timeout <= timerctl.count)
            {
                timerctl.timer[i].flags = TIMER_FLAGS_ALLOC;
                fifo8_put(timerctl.timer[i].fifo, timerctl.timer[i].data);
            }
        }
    }
    return;
}
