#include "bootpack.h"

#define PIT_CTRL 0x0043
#define PIT_CNT0 0x0040
#define TIMER_FLAGS_ALLOC 1 /* 確保した状態 */
#define TIMER_FLAGS_USING 2 /* タイマ作動状態 */

struct TIMERCTL timerctl;

void init_pit(void) {
    io_out8(PIT_CTRL, 0x34);
    io_out8(PIT_CNT0, 0x9c);
    io_out8(PIT_CNT0, 0x2e);

    timerctl.count = 0;

    for (int i = 0; i < MAX_TIMES; i++) {
        timerctl.timers0[i].flags = 0;
    }

    struct TIMER *t = timer_alloc();
    t->timeout      = 0xffffffff;
    t->flags        = TIMER_FLAGS_USING;
    t->next         = 0;
    timerctl.t0     = t;
    timerctl.next   = 0xffffffff; /* 番兵の時刻 */

    return;
}

struct TIMER *timer_alloc(void) {
    for (int i = 0; i < MAX_TIMES; i++) {
        if (timerctl.timers0[i].flags == 0) {
            timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
            return &timerctl.timers0[i];
        }
    }
    return 0;
}

void timer_free(struct TIMER *timer) {
    timer->flags = 0;
    return;
}

void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data) {
    timer->fifo = fifo;
    timer->data = data;
    return;
}

void timer_settime(struct TIMER *timer, unsigned int timeout) {
    timer->timeout = timeout + timerctl.count;
    timer->flags   = TIMER_FLAGS_USING;
    int eflags     = io_load_eflags();
    io_cli();

    struct TIMER *t = timerctl.t0;
    /*　先頭に入れる場合 */
    if (timer->timeout <= t->timeout) {
        timerctl.t0   = timer;
        timer->next   = t;
        timerctl.next = timer->timeout;

        io_store_eflags(eflags);
        return;
    }

    struct TIMER *s;
    while (1) {
        s = t;
        t = t->next;
        /* sとtの間に入れる場合 */
        if (timer->timeout <= t->timeout) {
            s->next     = timer;
            timer->next = t;

            io_store_eflags(eflags);
            return;
        }
    }
}

void inthandler20(int *esp) {
    /* IRQ-00受付完了をPICに通知 */
    io_out8(PIC0_OCW2, 0x60);

    timerctl.count++;

    if (timerctl.next > timerctl.count) {
        /* まだ次の時刻になっていいないため、終了 */
        return;
    }

    char ts             = 0;
    struct TIMER *timer = timerctl.t0;
    while (1) {
        if (timer->timeout > timerctl.count) {
            break;
        }
        /* タイムアウト */
        timer->flags = TIMER_FLAGS_ALLOC;
        if (timer != task_timer) {
            fifo32_put(timer->fifo, timer->data);
        } else {
            ts = 1;
        }
        timer = timer->next;
    }

    timerctl.t0   = timer;
    timerctl.next = timer->timeout;
    if (ts != 0) {
        task_switch();
    }

    return;
}
