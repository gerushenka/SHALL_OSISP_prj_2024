#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "block_device.h"

\
void read_block(int block_num, char *buffer) {
    int fd;
    off_t offset;
    ssize_t bytes_read;

    // Открываем файл блока устройства SSD
    fd = open("/dev/nvme0n1p7", O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Вычисляем смещение для чтения блока
    offset = (off_t) block_num * BLOCK_SIZE;

    // Позиционируемся на нужном блоке
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Читаем содержимое блока
    bytes_read = read(fd, buffer, BLOCK_SIZE);
    if (bytes_read == -1) {
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Закрываем файл
    close(fd);
}

// Функция для записи блока
void write_block(int block_num, const char* buffer, int length) {
    int fd;
    off_t offset;
    ssize_t bytes_written;
    printf("%s", buffer );
    fflush(stdout);
    // Открываем файл блока устройства SSD для записи
    fd = open("/dev/nvme0n1p7", O_WRONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    // Вычисляем смещение для записи блока
    offset = (off_t) block_num * BLOCK_SIZE;

    // Позиционируемся на нужном блоке
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Записываем содержимое блока
    bytes_written = write(fd, buffer, length);
    if (bytes_written == -1) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }
    
    // Закрываем файл
    close(fd);
}

int is_block_empty(const char *buffer) {
    for (int i = 0; i < BLOCK_SIZE; ++i) {
        if (buffer[i] != '\0') {
            return 0; // Блок не пустой
        }
    }
    return 1; // Блок пустой
}

int is_end_of_device(int block_num) {
    struct stat st;
    off_t offset;

    // Открываем файл блока устройства SSD
    int fd = open("/dev/nvme0n1", O_RDONLY);
    if (fd == -1) {
        return 1; // В случае ошибки считаем, что это конец устройства
    }

    // Вычисляем смещение для чтения блока
    offset = (off_t) block_num * BLOCK_SIZE;

    // Проверяем, существует ли блок с заданным смещением
    if (fstat(fd, &st) == -1) {
        close(fd);
        return 1; // В случае ошибки считаем, что это конец устройства
    }

    // Проверяем, достигли ли конца файла (устройства)
    if (st.st_size <= offset) {
        close(fd);
        return 1; // Да, это конец устройства
    }

    // Нет, это не конец устройства
    close(fd);
    return 0;
}

// Функция для форматирования блочного устройства
void format_block_device() {
    // Буфер для хранения данных блока
    char block_buffer[BLOCK_SIZE];

    // Заполнение блока нулями
    memset(block_buffer, 0, BLOCK_SIZE);

    // Запись нулевого блока во все блоки устройства
    for (int i = 0; i < TOTAL_BLOCKS; ++i) {
        write_block(i, block_buffer, BLOCK_SIZE);
    }
}