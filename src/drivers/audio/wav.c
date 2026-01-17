#include <drivers/audio/wav.h>
#include <drivers/video/vesa.h>
#include <arch/x86/ports.h>
#include <lib/string.h>

// только структура для чанка "fmt " (параметры звука)
typedef struct
{
    uint16_t format;      // 1 = PCM
    uint16_t channels;    // 1
    uint32_t sample_rate; // 8000 etc
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample; // 8
} __attribute__((packed)) wav_fmt_t;

// управление spермой
static void speaker_on()
{
    uint8_t tmp = inb(0x61);
    if (tmp != (tmp | 3))
        outb(0x61, tmp | 3);
}

static void speaker_off()
{
    uint8_t tmp = inb(0x61) & 0xFC;
    outb(0x61, tmp);
}

static inline void wait(int n)
{
    for (volatile int i = 0; i < n; i++)
        ;
}

// вспомогательная: печать числа
void print_u32(uint32_t n)
{
    if (n == 0)
    {
        vesa_print("0");
        return;
    }
    char buf[12];
    int i = 0;
    while (n > 0)
    {
        buf[i++] = (n % 10) + '0';
        n /= 10;
    }
    for (int j = i - 1; j >= 0; j--)
    {
        char t[2] = {buf[j], 0};
        vesa_print(t);
    }
}

// --- ну вы тут сами разберетесь ---
void wav_play(uint8_t *buffer)
{
    // 1. проверка RIFF
    if (memcmp(buffer, "RIFF", 4) != 0)
    {
        vesa_print_color("Error: Not a RIFF file.\n", 0xFF0000);
        return;
    }
    // 2. проверка WAVE
    if (memcmp(buffer + 8, "WAVE", 4) != 0)
    {
        vesa_print_color("Error: Not a WAVE file.\n", 0xFF0000);
        return;
    }

    uint32_t filesize = *(uint32_t *)(buffer + 4);
    uint32_t offset = 12; // Начинаем после "RIFF...WAVE"

    // параметры, которые мы ищем
    uint8_t *audio_data = NULL;
    uint32_t audio_size = 0;
    uint32_t sample_rate = 8000;
    int found_fmt = 0;

    vesa_print("Parsing Chunks:\n");

    // цикл по чанкам майнкрафта
    // бежим пока не выйдем за пределы файла
    while (offset < filesize + 8)
    { // +8 заголовок
        char chunk_id[5] = {0};
        memcpy(chunk_id, buffer + offset, 4);
        uint32_t chunk_size = *(uint32_t *)(buffer + offset + 4);

        vesa_print(" Found: [");
        vesa_print(chunk_id);
        vesa_print("] Size: ");
        print_u32(chunk_size);
        vesa_print("\n");

        if (memcmp(chunk_id, "fmt ", 4) == 0)
        {
            wav_fmt_t *fmt = (wav_fmt_t *)(buffer + offset + 8);
            if (fmt->format != 1)
            {
                vesa_print_color(" Error: Compressed WAV not supported.\n", 0xFF0000);
                return;
            }
            if (fmt->bits_per_sample != 8)
            {
                vesa_print_color(" Error: Need 8-bit.\n", 0xFF0000);
                return;
            }
            sample_rate = fmt->sample_rate;
            found_fmt = 1;
        }
        else if (memcmp(chunk_id, "data", 4) == 0)
        {
            audio_data = buffer + offset + 8;
            audio_size = chunk_size;
            // если нашли data, то остальные чанки (в конце файла) нам не интересны для плеера
            break;
        }

        // переход к следующему чанку
        offset += 8 + chunk_size;
    }

    if (!found_fmt)
    {
        vesa_print_color("Error: 'fmt ' chunk not found.\n", 0xFF0000);
        return;
    }
    if (!audio_data)
    {
        vesa_print_color("Error: 'data' chunk not found.\n", 0xFF0000);
        return;
    }

    // кто читает - респект
    vesa_print_color("Playing...\n", 0x00FF00);

    // расчет задержки (калибровка под QEMU)
    // чем выше частота дискретизации, тем меньше задержка
    int delay = 200;
    if (sample_rate > 10000)
        delay = 100;
    if (sample_rate < 6000)
        delay = 400;

    for (uint32_t i = 0; i < audio_size; i++)
    {
        uint8_t sample = audio_data[i];

        // в 8-битном WAV: 0..255, тишина = 128.
        // эмуляция 1-битного ЦАП:
        // если значение выше середины -> подаем ток (динамик выдвигается)
        // если ниже -> убираем ток.
        if (sample > 128)
            speaker_on();
        else
            speaker_off();

        wait(delay);

        // выход по клавише
        if ((i % 2000 == 0) && (inb(0x64) & 1))
        {
            inb(0x60); // сброс
            vesa_print("\nStopped.\n");
            break;
        }
    }
    speaker_off();
    vesa_print("Done.\n");
}
