#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <ctype.h>
#include <string.h>
#include "block_device.h"

#define BYTES_PER_LINE 16
#define BLOCK_SIZE 512 // Примерное значение, замените его на фактический размер блока
#define TOP_MARGIN 1   // Отступ сверху
#define LEFT_MARGIN 4  // Отступ слева

// Функция для вывода данных блока с адреса start_address
void print_block_data(int start_address, char *block_buffer, WINDOW *win) {
    // Вывод заголовка с форматом данных
    mvwprintw(win, 0, LEFT_MARGIN, "Addr");
    for (int j = 0; j < BYTES_PER_LINE; ++j) {
        mvwprintw(win, 0, LEFT_MARGIN + 9 + j * 3, " %02x ", j);
    }
    mvwprintw(win, 0, LEFT_MARGIN + 9 + BYTES_PER_LINE * 3, " ASCII");

    // Вывод данных блока с учетом отступов
    for (int i = 0; i < BLOCK_SIZE; i += BYTES_PER_LINE) {
        // Вывод адреса с учетом отступа сверху
        mvwprintw(win, i / BYTES_PER_LINE + TOP_MARGIN, LEFT_MARGIN, "%08x  ", start_address + i);

        // Вывод шестнадцатеричных значений байтов
        for (int j = 0; j < BYTES_PER_LINE; ++j) {
            if (i + j < BLOCK_SIZE) {
                wprintw(win, "%02x ", (unsigned char) block_buffer[i + j]);
            } else {
                wprintw(win, "   ");
            }
        }

        // Вывод ASCII значений
        for (int j = 0; j < BYTES_PER_LINE; ++j) {
            if (i + j < BLOCK_SIZE) {
                wprintw(win, "%c", isprint(block_buffer[i + j]) ? block_buffer[i + j] : '.');
            } else {
                break;
            }
        }
    }
}

int main() {
    // Открытие файла для логов
    FILE *log_file = fopen("log.txt", "w");
    if (log_file == NULL) {
        perror("Failed to open log file");
        return EXIT_FAILURE;
    }

    // Инициализация ncurses
    initscr();
    cbreak(); // Разрешить Ctrl+C
    noecho(); // Не отображать ввод пользователя
    keypad(stdscr, TRUE); // Разрешить обработку специальных клавиш

    while (1) {
        clear();
        WINDOW *menu_win = newwin(6, 35, (LINES - 7) / 2, (COLS - 30) / 2);
        box(menu_win, 0, 0); // Добавление рамки вокруг окна
        mvwprintw(menu_win, 1, 1, "Choose an action:");
        mvwprintw(menu_win, 2, 1, "1. View content of block device");
        mvwprintw(menu_win, 3, 1, "2. Format block device");
        mvwprintw(menu_win, 4, 1, "3. Exit");
        refresh();
        wrefresh(menu_win);
        int choice = getch();
        if (choice == '1') {
            // Просмотр содержимого блочного устройства
            int current_block = 0;
            while (1) {
                // Очистка окна
                clear();

                // Создание окна для данных блока
                WINDOW *data_win = newwin(BLOCK_SIZE / BYTES_PER_LINE + TOP_MARGIN + 1, 100, 1, 1);
                box(data_win, 0, 0); // Добавление рамки вокруг окна

                // Буфер для хранения данных блока
                char block_buffer[BLOCK_SIZE];

                // Чтение блока с номером current_block
                read_block(current_block, block_buffer);

                // Вывод данных блока в окно с учетом отступов
                print_block_data(current_block * BLOCK_SIZE, block_buffer, data_win);

                // Вывод информации о текущем блоке
                mvprintw(LINES - 3, 0, "Block: %d", current_block);
                mvprintw(LINES - 2, 0, "Press 'w' to modify the current block.");
                mvprintw(LINES - 1, 0, "Press 'q' to quit");

                // Обновление экрана
                refresh();
                wrefresh(data_win);

                // Ожидание ввода пользователя
                int ch = getch();
                // Выход из просмотра блоков при нажатии 'q'
                if (ch == 'q') {
                    fprintf(log_file, "Exited block view\n");
                    break;
                } else if (ch == KEY_LEFT || ch == KEY_UP) {
                    // Переход к предыдущему блоку
                    if (current_block > 0) {
                        current_block--;
                    }
                } else if (ch == KEY_RIGHT || ch == KEY_DOWN) {
                    // Переход к следующему блоку
                    current_block++;
                } else if (ch == 'w') {
                    // Запись данных в блок при нажатии 'w'
                    clear();
                    mvprintw(0, 0, "Enter new data for the block:");
                    refresh();

                    // Ввод новых данных
                    char new_data[BLOCK_SIZE];
                    fprintf(log_file, "\n\n%s\n", new_data);
                    echo(); // Включаем отображение ввода
                    mvgetnstr(1, 0, new_data, BLOCK_SIZE);
                    noecho(); // Выключаем отображение ввода
                    new_data[strcspn(new_data, "\n")] = '\0';
                    fprintf(log_file, "%s\n\n", new_data);

                    // Запись новых данных в блок
                    int length = strlen(new_data);
                    write_block(current_block, new_data, length);

                    // Запись действия в лог
                }
            }
        } else if (choice == '2') {
            // Форматирование блочного устройства
            fprintf(log_file, "Block device formatted\n");
            break;
        } else if (choice == '3') {
            // Выход
            break;
        } else {
            mvprintw(6, 0, "Invalid choice. Please enter a valid option.");
            refresh();
            getch();
        }
    }

    // Закрытие файла для логов
    fclose(log_file);

    // Очистка окна и завершение работы с ncurses
    endwin();

    return 0;
}
