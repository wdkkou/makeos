#include "bootpack.h"

void file_readfat(int *fat, unsigned char *img) {
    for (int i = 0, j = 0; i < 2800; i += 2, j += 3) {
        fat[i + 0] = (img[j + 0] | img[j + 1] << 8) & 0xfff;
        fat[i + 1] = (img[j + 1] >> 4 | img[j + 2] << 4) & 0xfff;
    }

    return;
}
void file_loadfile(int clustno, int size, char *buf, int *fat, char *img) {
    while (1) {
        if (size <= 512) {
            for (int i = 0; i < size; i++) {
                buf[i] = img[clustno * 512 + i];
            }
            break;
        }
        for (int i = 0; i < 512; i++) {
            buf[i] = img[clustno * 512 + i];
        }
        size -= 512;
        buf += 512;
        clustno = fat[clustno];
    }
    return;
}

struct FILEINFO *file_search(char *name, struct FILEINFO *finfo, int max) {
    char s[12];
    for (int y = 0; y < 11; y++) {
        s[y] = ' ';
    }
    for (int x = 0, y = 0; y < 11 && name[x] != 0; x++, y++) {
        if (name[x] == '.' && y <= 8) {
            y = 7;
        } else {
            s[y] = name[x];
            if ('a' <= s[y] && s[y] <= 'z') {
                /* 小文字を大文字に変換 */
                s[y] -= 0x20;
            }
        }
    }
    for (int x = 0; x < max; x++) {
        if (finfo[x].name[0] == 0x00) {
            break;
        }
        if ((finfo[x].type & 0x18) == 0) {
            int exist_file = 1;
            for (int y = 0; y < 11; y++) {
                if (finfo[x].name[y] != s[y]) {
                    exist_file = 0;
                    break;
                }
            }
            if (exist_file) {
                /* ファイルが見つかった */
                return finfo + x;
            }
        }
    }
    return 0;
}
