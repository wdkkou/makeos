int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *titile);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_initmalloc(void);
char *api_malloc(int size);
void api_point(int win, int x, int y, int col);
void api_end(void);

unsigned long rand(void);

void HariMain(void) {
    char *buf;
    int win;

    api_initmalloc();
    buf = api_malloc(150 * 100);
    win = api_openwin(buf, 150, 100, -1, "stars");
    api_boxfillwin(win, 6, 26, 143, 93, 0);
    for (int i = 0; i < 50; i++) {
        int x = (rand() % 137) + 6;
        int y = (rand() % 67) + 26;
        api_point(win, x, y, 3);
    }
    api_end();
}

unsigned long rand(void) {
    unsigned long rand;
    rand *= 1234567;
    rand += 1397;

    return rand;
}
