#include "../api.h"

void HariMain(void) {
    char *buf;
    int win;

    api_initmalloc();
    buf = api_malloc(150 * 50);
    win = api_openwin(buf, 150, 100, -1, "stars");
    api_boxfillwin(win, 6, 26, 143, 93, 0);
    api_point(win, 75, 59, 3);

    while (1) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_end();
}
