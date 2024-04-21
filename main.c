#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <ctype.h>
#include <string.h>
#include "block_device.h"

#define BYTES_PER_LINE 16
#define BLOCK_SIZE 512 // Примерное значение, замените его на фактический размер блока
#define TOTAL_BLOCKS 2048 // Примерное общее количество блоков в блочном устройстве
#define TOP_MARGIN 1   // Отступ сверху
#define LEFT_MARGIN 4  // Отступ слева

// Функция для вывода данных блока с адреса start_address
void print_block_data(int start_address, char *block_buffer, WINDOW *win, int cursor_offset, bool edit_mode) {
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

    int current_block = 0;
    int cursor_offset = 0;
    bool edit_mode = false;

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
                print_block_data(current_block * BLOCK_SIZE, block_buffer, data_win, cursor_offset, edit_mode);

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
                    fprintf(log_file, "Exited block view\n");
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
                    
                    write_block(current_block, block_buffer, BLOCK_SIZE);
                    // Перемещение курсора
                    // Проверка на выход за пределы блока
                    if (cursor_offset >= BLOCK_SIZE * 3) {
                        cursor_offset = BLOCK_SIZE * 3 - 1;
                    }
                } else if (edit_mode && (ch == KEY_UP || ch == KEY_DOWN)) {
                    // Перемещение между строками вверх и вниз в режиме редактирования
                    int line_offset = cursor_offset / (BYTES_PER_LINE * 3) * BYTES_PER_LINE * 3;
                    if (ch == KEY_UP && line_offset >= BYTES_PER_LINE * 3) {
                        cursor_offset -= BYTES_PER_LINE * 3; // Перемещение на строку вверх
                    } else if (ch == KEY_DOWN && line_offset < BLOCK_SIZE * 3 - BYTES_PER_LINE * 3) {
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
                    } else if (ch == KEY_RIGHT && cursor_offset < BLOCK_SIZE * 3 - 3) {
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
        } else if (choice == '2') {
            format_block_device(); // Вызов функции форматирования блочного устройства
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
