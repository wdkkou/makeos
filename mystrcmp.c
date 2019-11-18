int strcmp(char *str1, char *str2)
{
    for (; *str1 == *str2; str1++, str2++)
    {
        if (*str1 == '\0')
        {
            return 0;
        }
    }
    return *str1 - *str2;
}
