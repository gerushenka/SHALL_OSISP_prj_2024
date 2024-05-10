#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h> 
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h> 
#include <string.h>
#include <sys/ioctl.h> // Для использования ioctl
#include <linux/fs.h> // Для определения BLKGETSIZE64
#include "block_device.h"


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
    off_t offset = (off_t) block_num * BLOCK_LENGTH;

    // Позиционируемся на нужном блоке
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Читаем содержимое блока
    ssize_t bytes_read = read(fd, buffer, BLOCK_LENGTH);
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
    off_t offset = (off_t) block_num * BLOCK_LENGTH;

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
    char block_buffer[BLOCK_LENGTH];

    // Заполнение блока нулями
    memset(block_buffer, 0, BLOCK_LENGTH);

    // Запись нулевого блока во все блоки устройства
    for (int i = 0; i < TOTAL_BLOCKS; ++i) {
        write_block(block_device_name, i, block_buffer, BLOCK_LENGTH);
    }
}

// Функция для выбора блочного устройства
BlockDevice* choose_block_device() {
    // Очистка окна
    clear();

    // Вывод запроса на выбор блочного устройства
    mvprintw(0, 0, "Choose a block device (enter device number or press 'q' to quit):");

    // Переменные для работы с каталогами
    DIR *dir;
    struct dirent *ent;
    int device_number = 1;
    BlockDevice* devices[MAX_DEVICES];

    // Открытие каталога /dev/
    if ((dir = opendir("/dev/")) != NULL) {
        // Чтение содержимого каталога
        while ((ent = readdir (dir)) != NULL && device_number <= MAX_DEVICES) {
            // Формирование пути к файлу
            char path[512];
            snprintf(path, sizeof(path), "/dev/%s", ent->d_name);

            // Проверка, является ли файл блочным устройством
            struct stat st;
            if (stat(path, &st) == 0 && S_ISBLK(st.st_mode)) {
                // Открытие блочного устройства для получения информации о его размере
                int fd = open(path, O_RDONLY);
                if (fd != -1) {
                    // Получение информации о размере блока устройства
                    unsigned long long block_count;
                    if (ioctl(fd, BLKGETSIZE64, &block_count) != -1) {
                        // Выделение памяти под структуру и заполнение информации о блочном устройстве
                        devices[device_number - 1] = malloc(sizeof(BlockDevice));
                        if (devices[device_number - 1] != NULL) {
                            strcpy(devices[device_number - 1]->name, ent->d_name);
                            strcpy(devices[device_number - 1]->path, path);
                            devices[device_number - 1]->size = block_count; // Размер блока устройства 
                        } else {
                            perror("Memory allocation failed");
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        perror("Failed to get block device size");
                        exit(EXIT_FAILURE);
                    }
                    close(fd);
                } else {
                    perror("Failed to open block device");
                    exit(EXIT_FAILURE);
                }

                // Вывод информации о блочном устройстве
                mvprintw(device_number, 0, "%d. %s (%s, %ld bytes)", device_number, ent->d_name, path, devices[device_number - 1]->size);
                device_number++;
            }
        }
        closedir (dir);
    } else {
        // Если каталог не может быть открыт, выведите ошибку
        perror ("Unable to open directory");
        exit(EXIT_FAILURE);
    }

    // Отдельная строка для ввода числа устройства
    mvprintw(device_number + 1, 0, "> ");

    // Обновление экрана
    refresh();

    // Ожидание ввода пользователя
    char input[10];
    while (1) {
        // Получение ввода пользователя
        mvgetstr(device_number + 1, 2, input); // Ввод текста в координатах строки device_number + 1, столбца 2

        // Проверка на выход
        if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0) {
            return NULL;
        }

        // Проверка валидности ввода и возврат выбранного блочного устройства
        int choice = atoi(input);
        if (choice >= 1 && choice <= device_number - 1) {
            return devices[choice - 1];
        } else {
            mvprintw(device_number + 2, 0, "Invalid choice. Please enter a valid option or press 'q' to quit.");
            refresh();
        }
    }
}

// Функция для вывода данных блока с адреса start_address
void print_block_data(int start_address, char *block_buffer, WINDOW *win, int cursor_offset, bool edit_mode) {
    // Вывод заголовка с форматом данных
    mvwprintw(win, 0, LEFT_MARGIN, "Addr");
    for (int j = 0; j < BYTES_PER_LINE; ++j) {
        mvwprintw(win, 0, LEFT_MARGIN + 9 + j * 3, " %02x ", j);
    }
    mvwprintw(win, 0, LEFT_MARGIN + 9 + BYTES_PER_LINE * 3, " ASCII");

    // Вывод данных блока с учетом отступов
    for (int i = 0; i < BLOCK_LENGTH; i += BYTES_PER_LINE) {
        // Вывод адреса с учетом отступа сверху
        mvwprintw(win, i / BYTES_PER_LINE + TOP_MARGIN, LEFT_MARGIN, "%08x  ", start_address + i);

        // Вывод шестнадцатеричных значений байтов
        for (int j = 0; j < BYTES_PER_LINE; ++j) {
            if (i + j < BLOCK_LENGTH) {
                if (edit_mode && cursor_offset / 3 == i + j) {
                    wattron(win, A_UNDERLINE);
                    wprintw(win, "%02x ", (unsigned char) block_buffer[i + j]);
                    wattroff(win, A_UNDERLINE);
                } else {
                    wprintw(win, "%02x ", (unsigned char) block_buffer[i + j]);
                }
            } else {
                wprintw(win, "   ");
            }
        }

        // Вывод ASCII значений
        for (int j = 0; j < BYTES_PER_LINE; ++j) {
            if (i + j < BLOCK_LENGTH) {
                wprintw(win, "%c", isprint(block_buffer[i + j]) ? block_buffer[i + j] : '.');
            } else {
                break;
            }
        }
    }
}

// Функция для просмотра содержимого блочного устройства
void view_block_device_content(const char* block_device_name) {
    int current_block = 0;
    int cursor_offset = 0;
    bool edit_mode = false;

    while (1) {
        // Очистка окна
        clear();

        // Создание окна для данных блока
        WINDOW *data_win = newwin(BLOCK_LENGTH / BYTES_PER_LINE + TOP_MARGIN + 1, 100, 1, 1);
        box(data_win, 0, 0); // Добавление рамки вокруг окна

        // Буфер для хранения данных блока
        char block_buffer[BLOCK_LENGTH];

        // Чтение блока с номером current_block
        read_block(block_device_name, current_block, block_buffer); // Используем функцию чтения блока

        // Вывод данных блока в окно с учетом отступов
        print_block_data(current_block * BLOCK_LENGTH, block_buffer, data_win, cursor_offset, edit_mode);

        // Вывод информации о текущем блоке и режиме редактирования
        mvprintw(LINES - 3, 0, "Block: %d", current_block);
        mvprintw(LINES - 2, 0, "Use arrow keys to navigate. Press 'q' to quit. Press 'E' to toggle edit mode.");
        mvprintw(LINES - 1, 0, "Editing: %s", edit_mode ? "hex mode" : "view mode");

        // Обновление экрана
        refresh();
        wrefresh(data_win);

        // Ожидание ввода пользователя
        int ch = getch();
        // Выход из просмотра блоков при нажатии 'q'
        if (ch == 'q') {
            break;
        } else if (ch == 'E' || ch == 'e') {
            // Включение/выключение режима редактирования
            edit_mode = !edit_mode;
            // Если включен режим редактирования, установим курсор на текущий байт
            if (edit_mode) {
                cursor_offset = 0;
            }
        } else if (edit_mode && ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f'))) {
            // Редактирование содержимого блока в шестнадцатеричном виде
            int digit = ch <= '9' ? ch - '0' : ch - 'a' + 10;
            if (cursor_offset % 3 == 0) {
                // Позиция курсора на левом байте
                block_buffer[cursor_offset / 3] = (block_buffer[cursor_offset / 3] & 0xF0) | digit;
                cursor_offset++;
            } else {
                // Позиция курсора на правом байте
                block_buffer[cursor_offset / 3] = (block_buffer[cursor_offset / 3] & 0x0F) | (digit << 4);
                cursor_offset--;
            }
            
            write_block(block_device_name, current_block, block_buffer, BLOCK_LENGTH); // Используем функцию записи блока
            // Перемещение курсора
            // Проверка на выход за пределы блока
            if (cursor_offset >= BLOCK_LENGTH * 3) {
                cursor_offset = BLOCK_LENGTH * 3 - 1;
            }
        } else if (edit_mode && (ch == KEY_UP || ch == KEY_DOWN)) {
            // Перемещение между строками вверх и вниз в режиме редактирования
            int line_offset = cursor_offset / (BYTES_PER_LINE * 3) * BYTES_PER_LINE * 3;
            if (ch == KEY_UP && line_offset >= BYTES_PER_LINE * 3) {
                cursor_offset -= BYTES_PER_LINE * 3; // Перемещение на строку вверх
            } else if (ch == KEY_DOWN && line_offset < BLOCK_LENGTH * 3 - BYTES_PER_LINE * 3) {
                cursor_offset += BYTES_PER_LINE * 3; // Перемещение на строку вниз
            }
        } else if (!edit_mode && (ch == KEY_LEFT || ch == KEY_UP)) {
            // Переход к предыдущему блоку в режиме просмотра
            if (current_block > 0) {
                current_block--;
            }
        } else if (!edit_mode && (ch == KEY_RIGHT || ch == KEY_DOWN)) {
            // Переход к следующему блоку в режиме просмотра
            current_block++;
        } else if (edit_mode && (ch == KEY_LEFT || ch == KEY_RIGHT)) {
            // Перемещение курсора на другой байт стрелочками в режиме редактирования
            if (ch == KEY_LEFT && cursor_offset > 0) {
                if(cursor_offset % 3  == 0){
                    cursor_offset -= 3;
                }else{
                    cursor_offset -= 4; 
                }
            } else if (ch == KEY_RIGHT && cursor_offset < BLOCK_LENGTH * 3 - 3) {
                // Перемещение на следующий байт
                if(cursor_offset % 3  == 0){
                    cursor_offset += 3;
                }else{
                    cursor_offset += 2; 
                }
            }
        } else if (!edit_mode && (ch == KEY_UP || ch == KEY_DOWN)) {
            // Перемещение между блоками стрелочками в режиме просмотра
            if (ch == KEY_UP && current_block > 0) {
                current_block--;
            } else if (ch == KEY_DOWN && current_block < TOTAL_BLOCKS - 1) {
                current_block++;
            }
        }
    }
}

