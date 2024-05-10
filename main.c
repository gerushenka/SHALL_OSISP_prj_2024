#include <stdio.h>
#include <stdlib.h>
#include <ncurses.h>
#include <ctype.h>
#include "block_device.h"


// Функция для отображения меню выбора действия
int display_menu(const char* block_device_name) {
    clear();
    WINDOW *menu_win = newwin(8, 35, (LINES - 7) / 2, (COLS - 30) / 2);
    box(menu_win, 0, 0); // Добавление рамки вокруг окна
    mvwprintw(menu_win, 1, 1, "Block device: %s", block_device_name);
    mvwprintw(menu_win, 2, 1, "Choose an action:");
    mvwprintw(menu_win, 3, 1, "1. View content of block device");
    mvwprintw(menu_win, 4, 1, "2. Format block device");
    mvwprintw(menu_win, 5, 1, "3. Change block device");
    mvwprintw(menu_win, 6, 1, "4. Exit");
    refresh();
    wrefresh(menu_win);
    int choice = getch();
    return choice;
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

    const char* block_device_name = NULL;

    // Выбор блочного устройства
    while (block_device_name == NULL) {
        block_device_name = choose_block_device();
    }

    int choice;
    while ((choice = display_menu(block_device_name)) != '4') {
        if (choice == '1') {
            view_block_device_content(block_device_name);
        } else if (choice == '2') {
            mvprintw(20, 45, "Do you want to format the block device? (y/n)");
            refresh();
            int format_choice = mvgetch(20, 0);
            if (format_choice == 'y' || format_choice == 'Y') {
                format_block_device(block_device_name);
                fprintf(log_file, "Block device formatted\n");
                block_device_name = choose_block_device();
                } else {
                    mvprintw(30, 45, "Formatting canceled. Press any key to continue.");
                    refresh();
                    getch();
                }
        } else if (choice == '3') {
            // Изменение блочного устройства
            block_device_name = choose_block_device();
        } else {
            mvprintw(20, 45, "Invalid choice. Please enter a valid option.");
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
