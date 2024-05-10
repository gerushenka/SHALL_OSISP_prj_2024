#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#define NUM_BLOCKS 100
#define BLOCK_LENGTH 512
#define BYTES_PER_LINE 16
#define TOTAL_BLOCKS 2048 // Примерное общее количество блоков в блочном устройстве
#define BYTES_PER_LINE 16
#define TOP_MARGIN 1   // Отступ сверху
#define LEFT_MARGIN 4  // Отступ слева
#define MAX_DEVICES 100 // Максимальное количество блочных устройств (задайте максимальное разумное значение)

// Структура для хранения информации о блочном устройстве
typedef struct {
    char name[256];
    char path[512];
    off_t size;
} BlockDevice;

// Функция для чтения блока
void read_block(const char *block_device_name, int block_num, char *buffer);
// Функция для записи блока
void write_block(const char *block_device_name, int block_num, const char* buffer, int length);

// Функция для проверки блока на содержание данных
int is_block_empty(const char *buffer);

// Функция для проверки конца устройства
int is_end_of_device(int block_num);

// Функция для форматирования блочного устройства
void format_block_device(const char *block_device_name);

// Функция для выбора блочного устройства
BlockDevice* choose_block_device();

// Функция для вывода данных блока с адреса start_address
void print_block_data(int start_address, char *block_buffer, WINDOW *win, int cursor_offset, bool edit_mode);

// Функция для просмотра содержимого блочного устройства
void view_block_device_content(const char* block_device_name);

#endif // BLOCK_DEVICE_H
