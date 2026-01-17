#include <lib/string.h>

void *memset(void *ptr, int val, size_t size)
{
    char *p = (char *)ptr;
    for (size_t i = 0; i < size; i++)
        p[i] = (char)val;
    return ptr;
}

void *memcpy(void *dest, const void *src, size_t size)
{
    char *d = (char *)dest;
    const char *s = (const char *)src;
    for (size_t i = 0; i < size; i++)
        d[i] = s[i];
    return dest;
}

// <--- НОВАЯ ФУНКЦИЯ --->
int memcmp(const void *ptr1, const void *ptr2, size_t num)
{
    const unsigned char *p1 = (const unsigned char *)ptr1;
    const unsigned char *p2 = (const unsigned char *)ptr2;
    for (size_t i = 0; i < num; i++)
    {
        if (p1[i] != p2[i])
        {
            return p1[i] - p2[i];
        }
    }
    return 0;
}

size_t strlen(const char *str)
{
    size_t l = 0;
    while (str[l])
        l++;
    return l;
}

void strcpy(char *dest, const char *src)
{
    while ((*dest++ = *src++))
        ;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char toupper(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - 32;
    return c;
}

int strcasecmp(const char *s1, const char *s2)
{
    while (*s1 && (toupper(*s1) == toupper(*s2)))
    {
        if (*s1 == 0)
            return 0;
        s1++;
        s2++;
    }
    return toupper(*(unsigned char *)s1) - toupper(*(unsigned char *)s2);
}
