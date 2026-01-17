#include <fs/fat16.h>
#include <drivers/storage/ata.h>
#include <drivers/video/vesa.h>
#include <lib/string.h>

static uint32_t root_lba = 0;
static uint32_t data_lba = 0;
static uint8_t buf[512];
static uint8_t sec_per_clus = 1;

typedef struct
{
    uint8_t n[8], e[3], a, r[10];
    uint16_t t, d, c;
    uint32_t s;
} __attribute__((packed)) E;

// читаем BPB для уточнения параметров
void read_bpb()
{
    ata_read_sector(0, buf);
    // смещение 13 (0x0D) - Sectors Per Cluster (1 байт)
    uint8_t spc = buf[0x0D];
    if (spc > 0 && spc <= 128)
        sec_per_clus = spc;
    // смещение 14 (0x0E) - Reserved Sectors (2 байта)
    uint16_t reserved = *(uint16_t *)(buf + 0x0E);
    // смещение 16 (0x10) - Number of FATs (1 байт)
    uint8_t num_fats = buf[0x10];
    // смещение 22 (0x16) - Sectors Per FAT (2 байта)
    uint16_t spf = *(uint16_t *)(buf + 0x16);

    // пересчитываем адреса ТОЧНО по формуле
    if (reserved > 0)
    {
        uint32_t fat_start = reserved; // Обычно 200 у нас
        // Root = Reserved + (Fats * SectorsPerFat)
        root_lba = reserved + (num_fats * spf);
        // Data = Root + 32 (для 512 записей)
        data_lba = root_lba + 32;
        vesa_print("BPB Read: SPC=");
        // хак для вывода числа
        char t[2] = {'0' + sec_per_clus, 0};
        vesa_print(t);
        vesa_print("\n");
    }
}

void fat_to_str(E *e, char *b)
{
    memset(b, 0, 13);
    int k = 0;
    for (int i = 0; i < 8 && e->n[i] != ' '; i++)
        b[k++] = e->n[i];
    if (e->e[0] != ' ')
    {
        b[k++] = '.';
        for (int i = 0; i < 3 && e->e[i] != ' '; i++)
            b[k++] = e->e[i];
    }
}

void str_to_fat(const char *fn, char *n, char *e)
{
    memset(n, ' ', 8);
    memset(e, ' ', 3);
    int d = -1, l = 0;
    for (int i = 0; i < (int)strlen(fn); i++)
        if (fn[i] == '.')
            d = i;
    l = (d == -1) ? strlen(fn) : d;
    if (l > 8)
        l = 8;
    for (int i = 0; i < l; i++)
        n[i] = toupper(fn[i]);
    if (d != -1)
        for (int i = 0; i < 3; i++)
        {
            if (fn[d + 1 + i] == 0)
                break;
            e[i] = toupper(fn[d + 1 + i]);
        }
}

void fat16_init()
{
    vesa_print("Init FS...\n");
    // Сначала пробуем прочитать BPB из 0 сектора (если bootloader его не затер полностью)
    read_bpb();

    // Если BPB показал бред (root_lba=0), запускаем сканер
    if (root_lba < 100)
    {
        vesa_print("BPB invalid, scanning...\n");
        for (int i = 200; i < 1000; i++)
        {
            ata_read_sector(i, buf);
            E *e = (E *)buf;
            int found = 0;
            for (int j = 0; j < 16; j++)
            {
                if (memcmp(e[j].n, "LOGO    ", 8) == 0)
                    found = 1;
                if (memcmp(e[j].n, "TEST    ", 8) == 0)
                    found = 1;
            }
            if (found)
            {
                root_lba = i;
                data_lba = root_lba + 32;
                vesa_print("Scanner Found Root!\n");
                return;
            }
        }
    }
}

uint32_t fat16_read_file(const char *n, uint8_t *out)
{
    if (root_lba == 0)
        return 0;
    ata_read_sector(root_lba, buf);
    E *e = (E *)buf;
    int x = -1;
    for (int i = 0; i < 16; i++)
    {
        if (e[i].n[0] == 0)
            break;
        if (e[i].n[0] == 0xE5)
            continue;
        char fn[13];
        fat_to_str(&e[i], fn);
        if (strcasecmp(n, fn) == 0)
        {
            x = i;
            break;
        }
    }
    if (x == -1)
        return 0;

    // АДРЕСАЦИЯ С УЧЕТОМ КЛАСТЕРОВ
    uint32_t start = data_lba + (e[x].c - 2) * sec_per_clus;
    // Если кластер < 2, это баг или root файл
    if (e[x].c < 2)
        start = data_lba + x * sec_per_clus;

    uint32_t bytes_per_clus = sec_per_clus * 512;
    uint32_t clusters = (e[x].s + bytes_per_clus - 1) / bytes_per_clus;

    for (uint32_t c = 0; c < clusters; c++)
    {
        for (int s = 0; s < sec_per_clus; s++)
        {
            ata_read_sector(start + c * sec_per_clus + s, out + (c * bytes_per_clus + s * 512));
        }
    }
    return e[x].s;
}

void fat16_write_file(const char *fn, const char *c, uint32_t len)
{
    if (root_lba == 0)
    {
        root_lba = 400;
        data_lba = 432;
        memset(buf, 0, 512);
    }
    else
        ata_read_sector(root_lba, buf);

    E *e = (E *)buf;
    int x = -1;
    char tn[8], te[3];
    str_to_fat(fn, tn, te);

    for (int i = 0; i < 16; i++)
        if (memcmp(e[i].n, tn, 8) == 0 && memcmp(e[i].e, te, 3) == 0 && e[i].n[0] != 0xE5)
        {
            x = i;
            break;
        }
    if (x == -1)
        for (int i = 0; i < 16; i++)
            if (e[i].n[0] == 0 || e[i].n[0] == 0xE5)
            {
                x = i;
                break;
            }
    if (x == -1)
        return;

    memset(&e[x], 0, 32);
    memcpy(e[x].n, tn, 8);
    memcpy(e[x].e, te, 3);
    e[x].c = 2 + x;
    e[x].s = len; // Принудительно ставим кластер 2+x
    ata_write_sector(root_lba, buf);

    uint32_t start = data_lba + x * sec_per_clus;
    uint32_t sc = (len + 511) / 512;
    for (uint32_t s = 0; s < sc; s++)
    {
        memset(buf, 0, 512);
        int cl = (len - (s * 512) < 512) ? len - (s * 512) : 512;
        memcpy(buf, c + (s * 512), cl);
        ata_write_sector(start + s, buf);
    }
    vesa_print("Saved.\n");
}
void fat16_create_file(const char *n, const char *c) { fat16_write_file(n, c, strlen(c)); }
void fat16_list_files()
{
    if (root_lba == 0)
        return;
    ata_read_sector(root_lba, buf);
    E *e = (E *)buf;
    vesa_print("Files:\n");
    for (int i = 0; i < 16; i++)
    {
        if (e[i].n[0] == 0)
            break;
        if (e[i].n[0] == 0xE5 || e[i].a == 0xF)
            continue;
        char fn[13];
        fat_to_str(&e[i], fn);
        vesa_print(" ");
        vesa_print(fn);
        vesa_print("\n");
    }
}
