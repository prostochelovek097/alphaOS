#include <fs/fat16.h>
#include <drivers/storage/ata.h>
#include <drivers/video/vesa.h>
#include <lib/string.h>

// ЖЕСТКИЕ КОНСТАНТЫ (Мы сами создаем диск, мы их знаем)
// Reserved=200, FATs=2, RootEnt=512
// RootDirSize = 32 sectors
static uint32_t root_lba = 0;
static uint32_t data_lba = 0;
static uint8_t buf[512];

typedef struct
{
    uint8_t name[8];
    uint8_t ext[3];
    uint8_t attr;
    uint8_t res[10];
    uint16_t time, date, cluster;
    uint32_t size;
} __attribute__((packed)) fat_entry_t;

// Преобразование "FILE    TXT" -> "FILE.TXT"
void fat_to_str(fat_entry_t *e, char *out)
{
    memset(out, 0, 13);
    int k = 0;
    for (int i = 0; i < 8 && e->name[i] != ' '; i++)
        out[k++] = e->name[i];
    if (e->ext[0] != ' ')
    {
        out[k++] = '.';
        for (int i = 0; i < 3 && e->ext[i] != ' '; i++)
            out[k++] = e->ext[i];
    }
}

// Преобразование "file.txt" -> "FILE    TXT"
void str_to_fat(const char *in, char *name, char *ext)
{
    memset(name, ' ', 8);
    memset(ext, ' ', 3);

    int dot = -1;
    int len = strlen(in);
    for (int i = 0; i < len; i++)
        if (in[i] == '.')
            dot = i;

    int name_len = (dot == -1) ? len : dot;
    if (name_len > 8)
        name_len = 8;

    for (int i = 0; i < name_len; i++)
        name[i] = toupper(in[i]);

    if (dot != -1)
    {
        for (int i = 0; i < 3; i++)
        {
            if (dot + 1 + i >= len)
                break;
            ext[i] = toupper(in[dot + 1 + i]);
        }
    }
}

void fat16_init()
{
    vesa_print("Mounting FS...\n");
    // Сканируем диск в поисках Root Directory
    // Ищем метку тома "ALPHAOS"
    for (int i = 100; i < 1000; i++)
    {
        ata_read_sector(i, buf);
        fat_entry_t *e = (fat_entry_t *)buf;

        for (int j = 0; j < 16; j++)
        {
            // ATTR 0x08 = Volume ID
            if (memcmp(e[j].name, "ALPHAOS ", 8) == 0 && e[j].attr == 0x08)
            {
                root_lba = i;
                data_lba = root_lba + 32; // 512 entries * 32 bytes = 32 sectors
                vesa_print_color("FS Mounted!\n", 0x00FF00);
                return;
            }
        }
    }
    vesa_print_color("FS Error: ALPHAOS label missing.\n", 0xFF0000);
}

void fat16_list_files()
{
    if (root_lba == 0)
        return;
    vesa_print("Files:\n");

    // Читаем первые 2 сектора каталога
    for (int s = 0; s < 2; s++)
    {
        ata_read_sector(root_lba + s, buf);
        fat_entry_t *e = (fat_entry_t *)buf;

        for (int i = 0; i < 16; i++)
        {
            if (e[i].name[0] == 0)
                return;
            if (e[i].name[0] == 0xE5)
                continue;

            // ФИЛЬТР: Пропускаем LFN (0x0F), VolumeID (0x08), Dir (0x10)
            if (e[i].attr == 0x0F || e[i].attr == 0x08 || e[i].attr & 0x10)
                continue;

            char name[13];
            fat_to_str(&e[i], name);
            vesa_print("  ");
            vesa_print(name);
            vesa_print("\n");
        }
    }
}

uint32_t fat16_read_file(const char *fname, uint8_t *out_buf)
{
    if (root_lba == 0)
        return 0;

    char target_n[8], target_e[3];
    str_to_fat(fname, target_n, target_e);

    // Поиск файла
    int cluster = 0;
    uint32_t size = 0;
    int found = 0;

    for (int s = 0; s < 4; s++)
    { // Ищем в первых 4 секторах каталога
        ata_read_sector(root_lba + s, buf);
        fat_entry_t *e = (fat_entry_t *)buf;

        for (int i = 0; i < 16; i++)
        {
            if (e[i].name[0] == 0)
                break;
            if (e[i].name[0] == 0xE5)
                continue;
            // Игнорируем не-файлы
            if (e[i].attr == 0x08 || e[i].attr & 0x10)
                continue;

            if (memcmp(e[i].name, target_n, 8) == 0 && memcmp(e[i].ext, target_e, 3) == 0)
            {
                cluster = e[i].cluster;
                size = e[i].size;
                found = 1;
                goto read;
            }
        }
    }
    return 0;

read:
    if (size > 2000000)
        size = 2000000; // Лимит 2МБ
    // LBA = DataStart + (Cluster - 2) * SectorsPerCluster (у нас 1)
    uint32_t start_lba = data_lba + (cluster - 2);
    uint32_t sectors = (size + 511) / 512;

    for (uint32_t i = 0; i < sectors; i++)
    {
        ata_read_sector(start_lba + i, out_buf + (i * 512));
    }
    return size;
}

void fat16_create_file(const char *fname, const char *content)
{
    if (root_lba == 0)
        return;
    ata_read_sector(root_lba, buf); // Читаем 1-й сектор рута
    fat_entry_t *e = (fat_entry_t *)buf;

    int idx = -1;
    // Ищем место
    for (int i = 0; i < 16; i++)
    {
        if (e[i].name[0] == 0 || e[i].name[0] == 0xE5)
        {
            idx = i;
            break;
        }
    }
    if (idx == -1)
        return;

    memset(&e[idx], 0, 32);
    str_to_fat(fname, (char *)e[idx].name, (char *)e[idx].ext);

    e[idx].attr = 0x20;         // Archive
    e[idx].cluster = 100 + idx; // Фейковый кластер для теста
    e[idx].size = strlen(content);

    ata_write_sector(root_lba, buf);

    // Пишем данные
    memset(buf, 0, 512);
    strcpy((char *)buf, content);
    ata_write_sector(data_lba + (e[idx].cluster - 2), buf);
    vesa_print("Saved.\n");
}

void fat16_write_file(const char *n, const char *c, uint32_t l)
{
    fat16_create_file(n, c); // Переиспользуем логику
}
