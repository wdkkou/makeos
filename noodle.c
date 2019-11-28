#include "api.h"
#include "bootpack.h"

void HariMain(void) {
    api_initmalloc();
    char *buf = api_malloc(150 * 50);
    int win   = api_openwin(buf, 150, 50, -1, "noodle");
    int timer = api_alloctimer();
    api_inittimer(timer, 128);
    int sec = 0, min = 0, hour = 0;
    char s[12];

    while (1) {
        sprintf(s, "    %d:%d:%d", hour, min, sec);
        api_boxfillwin(win, 28, 27, 115, 41, 7);
        api_putstrwin(win, 28, 27, 0, 11, s);
        api_settimer(timer, 100);
        if (api_getkey(1) != 128) {
            break;
        }
        sec++;
        if (sec == 60) {
            sec = 0;
            min++;
            if (min == 60) {
                min = 0;
                hour++;
            }
        }
    }
    api_end();
}
