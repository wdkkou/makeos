/* マルチタスク関係 */
#include "bootpack.h"

struct TASKCTL *taskctl;
struct TIMER *task_timer;

struct TASK *task_init(struct MEMMAN *memman)
{
    struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *)ADR_GDT;
    taskctl = (struct TASKCTL *)memman_alloc_4k(memman, sizeof(struct TASKCTL));
    for (int i = 0; i < MAX_TASKS; i++)
    {
        taskctl->tasks0[i].flags = 0;
        taskctl->tasks0[i].sel = (TASK_GDT0 + i) * 8;
        set_segmdesc(gdt + TASK_GDT0 + i, 103, (int)&taskctl->tasks0[i].tss, AR_TSS32);
    }

    struct TASK *task = task_alloc();
    task->flags = 2;    /* 動作中*/
    task->priority = 2; /* 0.02sec*/
    taskctl->running = 1;
    taskctl->now = 0;
    taskctl->tasks[0] = task;
    load_tr(task->sel);
    task_timer = timer_alloc();
    timer_settime(task_timer, 2);

    return task;
}

struct TASK *task_alloc(void)
{
    for (int i = 0; i < MAX_TASKS; i++)
    {
        if (taskctl->tasks0[i].flags == 0)
        {
            struct TASK *task = &taskctl->tasks0[i];
            task->flags = 1;               /* 使用中マーク */
            task->tss.eflags = 0x00000202; /* IF = 1; */
            task->tss.eax = 0;             /* とりあえず0にしておくことにする */
            task->tss.ecx = 0;
            task->tss.edx = 0;
            task->tss.ebx = 0;
            task->tss.ebp = 0;
            task->tss.esi = 0;
            task->tss.edi = 0;
            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;
            task->tss.ldtr = 0;
            task->tss.iomap = 0x40000000;
            return task;
        }
    }
    return 0;
}

void task_run(struct TASK *task, int priority)
{
    if (priority > 0)
    {
        task->priority = priority;
    }
    if (task->flags != 2)
    {
        task->flags = 2; /* 動作中 */
        taskctl->tasks[taskctl->running] = task;
        taskctl->running++;
    }
    return;
}

void task_switch(void)
{
    taskctl->now++;
    if (taskctl->now == taskctl->running)
    {
        taskctl->now = 0;
    }
    struct TASK *task = taskctl->tasks[taskctl->now];
    timer_settime(task_timer, task->priority);

    /* 動作中のタスクが2以上の時だけタスクスイッチ */
    if (taskctl->running >= 2)
    {
        farjmp(0, task->sel);
    }

    return;
}

void task_sleep(struct TASK *task)
{
    if (task->flags == 2)
    {
        char ts = 0;
        if (task == taskctl->tasks[taskctl->now])
        {
            /*自分自身をsleepする際には, 後でタスクスウィッチ */
            ts = 1;
        }
        /* taskがどこにあるのかを探す */
        int pos = 0;
        for (int i = 0; i < taskctl->running; i++)
        {
            if (taskctl->tasks[i] == task)
            {
                pos = i;
                break;
            }
        }
        taskctl->running--;
        if (pos < taskctl->now)
        {
            taskctl->now--;
        }
        for (; pos < taskctl->running; pos++)
        {
            taskctl->tasks[pos] = taskctl->tasks[pos + 1];
        }
        task->flags = 1; /* 動作していない状態 */

        if (ts != 0)
        {
            /*タスクスイッチ*/
            if (taskctl->now >= taskctl->running)
            {
                taskctl->now = 0;
            }
            farjmp(0, taskctl->tasks[taskctl->now]->sel);
        }
    }
    return;
}
