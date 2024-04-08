#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#define NUM_BLOCKS 100
#define BLOCK_SIZE 512
#define BYTES_PER_LINE 16


// Функция для чтения блока
void read_block(int block_num, char *buffer);

// Функция для записи блока
void write_block(int block_num, const char* buffer, int length);

// Функция для проверки блока на содержание данных
int is_block_empty(const char *buffer);

// Функция для проверки конца устройства
int is_end_of_device(int block_num);

#endif // BLOCK_DEVICE_H
