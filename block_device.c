#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h> 
#include <string.h>
#include "block_device.h"

#define BYTES_PER_LINE 16
#define BLOCK_SIZE 512 // Примерное значение, замените его на фактический размер блока
#define TOTAL_BLOCKS 2048 // Примерное общее количество блоков в блочном устройстве
#define TOP_MARGIN 1   // Отступ сверху
#define LEFT_MARGIN 4  // Отступ слева
#define MAX_DEVICES 10 // Максимальное количество блочных устройств

// Функция для чтения блока
void read_block(const char *block_device_name, int block_num, char *buffer) {
    // Открываем файл блока устройства SSD
    char path[512];
    snprintf(path, sizeof(path), "/dev/%s", block_device_name);
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Вычисляем смещение для чтения блока
    off_t offset = (off_t) block_num * BLOCK_SIZE;

    // Позиционируемся на нужном блоке
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Читаем содержимое блока
    ssize_t bytes_read = read(fd, buffer, BLOCK_SIZE);
    if (bytes_read == -1) {
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Закрываем файл
    close(fd);
}

// Функция для записи блока
void write_block(const char *block_device_name, int block_num, const char* buffer, int length) {
    // Открываем файл блока устройства SSD для записи
    char path[512];
    snprintf(path, sizeof(path), "/dev/%s", block_device_name);
    int fd = open(path, O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Вычисляем смещение для записи блока
    off_t offset = (off_t) block_num * BLOCK_SIZE;

    // Позиционируемся на нужном блоке
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Записываем содержимое блока
    ssize_t bytes_written = write(fd, buffer, length);
    if (bytes_written == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    // Закрываем файл
    close(fd);
}

// Функция для форматирования блочного устройства
void format_block_device(const char *block_device_name) {
    // Буфер для хранения данных блока
    char block_buffer[BLOCK_SIZE];

    // Заполнение блока нулями
    memset(block_buffer, 0, BLOCK_SIZE);

    // Запись нулевого блока во все блоки устройства
    for (int i = 0; i < TOTAL_BLOCKS; ++i) {
        write_block(block_device_name, i, block_buffer, BLOCK_SIZE);
    }
}

// Ваш остальной код здесь...

