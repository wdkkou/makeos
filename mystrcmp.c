int strcmp(char *str1, char *str2) {
    for (; *str1 == *str2; str1++, str2++) {
        if (*str1 == '\0') {
            return 0;
        }
    }
    return *str1 - *str2;
}

int strncmp(char *str1, char *str2, int size) {
    for (; *str1 == *str2; str1++, str2++) {
        if (*str1 == '\0' || !--size) {
            return 0;
        }
    }
    return *str1 - *str2;
}
