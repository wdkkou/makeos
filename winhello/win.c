int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_end(void);
#include "../api.h"

char buf[150 * 50];

void HariMain(void) {
    int win = api_openwin(buf, 150, 50, -1, "hello");
    while (1) {
        if (api_getkey(1) == 0x0a) {
            break;
        }
    }
    api_end();
}
