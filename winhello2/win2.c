#include "../api.h"

void HariMain(void) {
    char buf[150 * 50];
    int win = api_openwin(buf, 150, 50, -1, "hello");
    api_boxfillwin(win, 8, 36, 141, 42, 3);
    api_putstrwin(win, 28, 28, 0, 12, "hello,world");
    while (1) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_end();
}
