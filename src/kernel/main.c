#include <drivers/video/vesa.h>
#include <drivers/input/keyboard.h>
#include <drivers/audio/pcspk.h>
#include <drivers/audio/wav.h>
#include <fs/fat16.h>
#include <lib/string.h>

#define FILE_BUFFER_ADDR 0x200000

static char input_buffer[128];
static int input_idx = 0;

// структура BMP
typedef struct
{
    uint16_t type;
    uint32_t size;
    uint16_t r1, r2;
    uint32_t offset;
    uint32_t header_size;
    int32_t width, height;
    uint16_t planes, bits;
} __attribute__((packed)) bmp_header_t;

void draw_bmp(uint8_t *buffer)
{
    bmp_header_t *header = (bmp_header_t *)buffer;
    if (header->type != 0x4D42 || header->bits != 24)
    {
        vesa_print_color("Error: Invalid BMP (Need 24-bit)\n", 0xFF0000);
        return;
    }
    vesa_clear(0x000000);
    uint8_t *image = buffer + header->offset;
    int w = header->width;
    int h = header->height;
    int start_x = (800 - w) / 2;
    int start_y = (600 - h) / 2;

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            int file_y = (h - 1) - y;
            int idx = (file_y * w + x) * 3;
            uint32_t color = (image[idx + 2] << 16) | (image[idx + 1] << 8) | image[idx];
            vesa_put_pixel(start_x + x, start_y + y, color);
        }
    }
    vesa_print("BMP Loaded. Press Enter...");
    while (keyboard_get_char() != '\n')
        ;
    vesa_clear(0x000088);
}

void start_editor(const char *filename)
{
    uint8_t *buf = (uint8_t *)FILE_BUFFER_ADDR;
    memset(buf, 0, 10000);
    int cursor = fat16_read_file(filename, buf);

    vesa_clear(0x000088);
    vesa_print_color("--- AlphaOS Editor ---\n", 0xFFFF00);
    vesa_print("TAB: Save | ESC: Exit\n----------------\n");
    vesa_print((char *)buf);

    while (1)
    {
        char c = keyboard_get_char();
        if (c == 27)
            break; // ESC
        if (c == '\t')
        { // TAB
            fat16_write_file(filename, (char *)buf, cursor);
            break;
        }
        if (c == '\b')
        {
            if (cursor > 0)
            {
                cursor--;
                buf[cursor] = 0;
                vesa_clear(0x000088);
                vesa_print_color("--- AlphaOS Editor ---\n", 0xFFFF00);
                vesa_print("TAB: Save | ESC: Exit\n----------------\n");
                vesa_print((char *)buf);
            }
        }
        else if (c)
        {
            buf[cursor++] = c;
            char tmp[2] = {c, 0};
            vesa_print(tmp);
        }
    }
    vesa_clear(0x000088);
}

void execute_command()
{
    vesa_print("\n");

    char cmd[16] = {0};
    char arg[16] = {0};

    // тут простой парсинг команд
    int space = -1;
    for (int i = 0; i < 128; i++)
    {
        if (input_buffer[i] == ' ')
        {
            space = i;
            break;
        }
        if (input_buffer[i] == 0)
            break;
    }

    if (space != -1)
    {
        memcpy(cmd, input_buffer, space);
        strcpy(arg, input_buffer + space + 1);
    }
    else
    {
        strcpy(cmd, input_buffer);
    }

    if (strcmp(cmd, "help") == 0)
    {
        vesa_print("Commands: ls, cat [file], edit [file], play [file], view [file], mkfile, clear\n");
    }
    else if (strcmp(cmd, "ls") == 0)
    {
        fat16_list_files();
    }
    else if (strcmp(cmd, "clear") == 0)
    {
        vesa_clear(0x000088);
    }
    else if (strcmp(cmd, "mkfile") == 0)
    {
        if (arg[0])
            fat16_create_file(arg, "New File");
        else
            fat16_create_file("NEW.TXT", "Welcome to AlphaOS!");
    }
    else if (strcmp(cmd, "cat") == 0)
    {
        uint8_t *buf = (uint8_t *)FILE_BUFFER_ADDR;
        memset(buf, 0, 10000);
        if (fat16_read_file(arg, buf))
        {
            vesa_print((char *)buf);
            vesa_print("\n");
        }
        else
        {
            vesa_print_color("Error: File not found.\n", 0xFF0000);
        }
    }
    else if (strcmp(cmd, "edit") == 0)
    {
        if (arg[0])
            start_editor(arg);
        else
            vesa_print("Usage: edit FILENAME\n");
    }
    else if (strcmp(cmd, "view") == 0)
    {
        uint8_t *buf = (uint8_t *)FILE_BUFFER_ADDR;
        if (fat16_read_file(arg, buf))
            draw_bmp(buf);
        else
            vesa_print_color("Error: File not found.\n", 0xFF0000);
    }
    else if (strcmp(cmd, "play") == 0)
    {
        if (!arg[0])
            play_imperial_march();
        else
        {
            uint8_t *buf = (uint8_t *)FILE_BUFFER_ADDR;
            if (fat16_read_file(arg, buf))
                wav_play(buf);
            else
                vesa_print_color("Error: File not found.\n", 0xFF0000);
        }
    }
    else
    {
        vesa_print("Unknown command.\n");
    }

    input_idx = 0;
    memset(input_buffer, 0, 128);
    vesa_print("\nAlphaOS > ");
}

__attribute__((section(".text.entry"))) void kernel_main(uint32_t vesa_info_addr)
{
    vesa_init(vesa_info_addr);
    fat16_init();

    vesa_print_color("AlphaOS Ultimate v1.0\n", 0x00FF00);
    vesa_print("Type 'help' for commands.\n\nAlphaOS > ");

    while (1)
    {
        char c = keyboard_get_char();
        if (c)
        {
            if (c == '\n')
            {
                input_buffer[input_idx] = 0;
                execute_command();
            }
            else if (c == '\b')
            {
                if (input_idx > 0)
                {
                    input_idx--;
                    char tmp[2] = {'\b', 0};
                    vesa_print(tmp);
                }
            }
            else
            {
                if (input_idx < 127)
                {
                    input_buffer[input_idx++] = c;
                    char tmp[2] = {c, 0};
                    vesa_print(tmp);
                }
            }
            for (volatile int i = 0; i < 100000; i++)
                ;
        }
    }
}
