#include "../api.h"

#define INVALID -0x7fffffff

/* mysprintf.c */
void sprintf(char *str, char *fmt, ...);
// int strtol(char *s, char **endp, int base); /* 標準関数 */

char *skipspace(char *p);
int getnum(char **pp, int priority);
int myatoi(char *str);

void HariMain(void) {
    int i;
    char s[30], *p;

    api_cmdline(s, 30);
    for (p = s; *p > ' '; p++) {
    } /* スペースが来るまで読み飛ばす */
    i = getnum(&p, 9);
    if (i == INVALID) {
        api_putstr("error!\n");
    } else {
        sprintf(s, "= %d = 0x%x\n", i, i);
        api_putstr(s);
    }
    api_end();
}

char *skipspace(char *p) {
    for (; *p == ' '; p++) {
    } /* スペースを読み飛ばす */
    return p;
}

int getnum(char **pp, int priority) {
    char *p = *pp;
    int i   = INVALID, j;
    p       = skipspace(p);

    /* 単項演算子 */
    if (*p == '+') {
        p = skipspace(p + 1);
        i = getnum(&p, 0);
    } else if (*p == '-') {
        p = skipspace(p + 1);
        i = getnum(&p, 0);
        if (i != INVALID) {
            i = -i;
        }
    } else if (*p == '~') {
        p = skipspace(p + 1);
        i = getnum(&p, 0);
        if (i != INVALID) {
            i = ~i;
        }
    } else if (*p == '(') { /* カッコ */
        p = skipspace(p + 1);
        i = getnum(&p, 9);
        if (*p == ')') {
            p = skipspace(p + 1);
        } else {
            i = INVALID;
        }
    } else if ('0' <= *p && *p <= '9') { /* 数値 */
        // i = strtol(p, &p, 0);
        // i = myatoi(p);
    } else { /* エラー */
        i = INVALID;
    }

    /* 2項演算子 */
    for (;;) {
        if (i == INVALID) {
            break;
        }
        p = skipspace(p);
        if (*p == '+' && priority > 2) {
            p = skipspace(p + 1);
            j = getnum(&p, 2);
            if (j != INVALID) {
                i += j;
            } else {
                i = INVALID;
            }
        } else if (*p == '-' && priority > 2) {
            p = skipspace(p + 1);
            j = getnum(&p, 2);
            if (j != INVALID) {
                i -= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '*' && priority > 1) {
            p = skipspace(p + 1);
            j = getnum(&p, 1);
            if (j != INVALID) {
                i *= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '/' && priority > 1) {
            p = skipspace(p + 1);
            j = getnum(&p, 1);
            if (j != INVALID && j != 0) {
                i /= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '%' && priority > 1) {
            p = skipspace(p + 1);
            j = getnum(&p, 1);
            if (j != INVALID && j != 0) {
                i %= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '<' && p[1] == '<' && priority > 3) {
            p = skipspace(p + 2);
            j = getnum(&p, 3);
            if (j != INVALID && j != 0) {
                i <<= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '>' && p[1] == '>' && priority > 3) {
            p = skipspace(p + 2);
            j = getnum(&p, 3);
            if (j != INVALID && j != 0) {
                i >>= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '&' && priority > 4) {
            p = skipspace(p + 1);
            j = getnum(&p, 4);
            if (j != INVALID) {
                i &= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '^' && priority > 5) {
            p = skipspace(p + 1);
            j = getnum(&p, 5);
            if (j != INVALID) {
                i ^= j;
            } else {
                i = INVALID;
            }
        } else if (*p == '|' && priority > 6) {
            p = skipspace(p + 1);
            j = getnum(&p, 6);
            if (j != INVALID) {
                i |= j;
            } else {
                i = INVALID;
            }
        } else {
            break;
        }
    }
    p   = skipspace(p);
    *pp = p;
    return i;
}
int myatoi(char *str) {
    int num = 0;

    while (*str != '\0') {
        if (*str < 48 || *str > 57) {
            break;
        }
        num += *str - 48;
        num *= 10;
        str++;
    }
    num /= 10;

    return num;
}
