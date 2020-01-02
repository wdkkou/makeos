#include "../api.h"

void HariMain(void) {
    char cmdline[30];
    api_cmdline(cmdline, 30);
    char *p;
    for (p = cmdline; *p > ' '; p++) {
    }
    for (; *p == ' '; p++) {
    }
    int fh = api_fopen(p);
    if (fh != 0) {
        char c;
        while (1) {
            if (api_fread(&c, 1, fh) == 0) {
                break;
            }
            api_putchar(c);
        }
    } else {
        api_putstr("file not found.\n");
    }
    api_end();
}
