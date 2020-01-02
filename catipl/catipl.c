#include "../api.h"

void HariMain(void) {
    int fh = api_fopen("ipl.asm");
    if (fh != 0) {
        char c;
        while (1) {
            if (api_fread(&c, 1, fh) == 0) {
                break;
            }
            api_putchar(c);
        }
    }
    api_end();
}
