int api_openwin(char *buf, int xsize, int ysize, int col_inv, char *title);
void api_putstrwin(int win, int x, int y, int col, int len, char *str);
void api_boxfillwin(int win, int x0, int y0, int x1, int y1, int col);
void api_end(void);

char buf[150 * 50];

void HariMain(void) {
    int win = api_openwin(buf, 150, 50, -1, "hello");
    api_boxfillwin(win, 8, 36, 141, 42, 3);
    api_putstrwin(win, 28, 28, 0, 12, "hello,world");
    api_end();
}
