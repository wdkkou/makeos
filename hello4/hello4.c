void api_putstr(char *s);
void api_end(void);

void HariMain(void) {
    api_putstr("hello world");
    api_end();
}
