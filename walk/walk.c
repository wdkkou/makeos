#include "../api.h"

void HariMain(void) {
    api_initmalloc();
    char *buf = api_malloc(160 * 100);
    int win   = api_openwin(buf, 160, 100, -1, "vim-walk");
    api_boxfillwin(win + 1, 4, 24, 155, 95, 0);

    int x = 76, y = 56;
    api_putstrwin(win, x, y, 3, 1, "*");
    while (1) {
        int data = api_getkey(1);
        api_putstrwin(win, x, y, 0, 1, "*");
        if (data == 'h' && x > 4) {
            x -= 8;
        }
        if (data == 'l' && x < 148) {
            x += 8;
        }
        if (data == 'k' && y > 24) {
            y -= 8;
        }
        if (data == 'j' && y < 80) {
            y += 8;
        }
        if (data == 0x0a) {
            break;
        }
        api_putstrwin(win, x, y, 3, 1, "*");
    }
    api_closewin(win);

    api_end();
}
