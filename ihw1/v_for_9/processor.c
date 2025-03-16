//
// Created by Сергей Растворов on 16/3/25.
//
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define CHUNK_SIZE 128

int is_valid_separator(char c) {
    return isspace(c) || c == '(' || c == ')' || c == '{' || c == '}' ||
           c == ';' || c == ',' || c == '+' || c == '-' || c == '*' ||
           c == '/' || c == '%' || c == '=' || c == '[' || c == ']' ||
           c == '!' || c == '&' || c == '|' || c == '<' || c == '>' ||
           c == '\0';
}

ssize_t findKeyWord(char *source, char *result) {
    char *keywords[] = {"int", "return", "char", "while", "for"};
    int counts[5] = {0};

    int in_string = 0;
    int in_comment = 0;
    char *ptr = source;

    while (*ptr) {
        if (*ptr == '"' && (ptr == source || *(ptr - 1) != '\\')) {
            in_string = !in_string;
        }

        if (!in_string && *ptr == '/' && *(ptr + 1) == '/') {
            while (*ptr && *ptr != '\n') ptr++;
            continue;
        }

        if (!in_string && *ptr == '/' && *(ptr + 1) == '*') {
            in_comment = 1;
            ptr += 2;
            while (*ptr && *ptr != '\0' && !( *ptr == '*' && *(ptr + 1) == '/')) ptr++;
            if (*ptr && *(ptr + 1) == '/') {
                in_comment = 0;
                ptr+=2;
                continue;
            }
        }

        if (!in_string && !in_comment) {
            for (int i = 0; i < 5; i++) {
                int len = strlen(keywords[i]);
                if (strncmp(ptr, keywords[i], len) == 0) {
                    char before = (ptr == source) ? ' ' : *(ptr - 1);
                    char after = *(ptr + len);

                    if (is_valid_separator(before) && is_valid_separator(after)) {
                        counts[i]++;
                        ptr += len;
                        continue;
                    }
                }
            }
        }
        ptr++;
    }

    char temp[256];
    result[0] = '\0';

    for (int i = 0; i < 5; i++) {
        snprintf(temp, sizeof(temp), "Ключевое слово \"%s\" встретилось %d раз(а)\n", keywords[i], counts[i]);
        strncat(result, temp, CHUNK_SIZE);
    }

    return strlen(result);
}

void processChunks(int input_fd, int output_fd) {
    char buffer[CHUNK_SIZE + 1]; // +1 для \0
    char result[CHUNK_SIZE * 2]; // Увеличенный размер результата
    char accumulator[CHUNK_SIZE * 2] = ""; // Буфер для накопления данных между вызовами read

    ssize_t bytes_read;
    while ((bytes_read = read(input_fd, buffer, CHUNK_SIZE)) > 0) {
        buffer[bytes_read] = '\0'; // Убедимся, что строка корректно завершается

        // Объединяем данные из прошлого чанка с новым буфером
        strncat(accumulator, buffer, sizeof(accumulator) - strlen(accumulator) - 1);

        // Ищем последнюю полную строку
        char *last_newline = strrchr(accumulator, '\n');
        if (last_newline) {
            *(last_newline + 1) = '\0'; // Обрезаем всё, что после последнего \n

            // Обрабатываем накопленные данные
            ssize_t processed_size = findKeyWord(accumulator, result);
            if (write(output_fd, result, processed_size) != processed_size) {
                perror("write to FIFO");
                exit(EXIT_FAILURE);
            }

            // Сохраняем остаток, который не обработан (часть строки после \n)
            memmove(accumulator, last_newline + 1, strlen(last_newline + 1) + 1);
        }
    }

    // Обрабатываем оставшиеся данные после выхода из цикла
    if (strlen(accumulator) > 0) {
        ssize_t processed_size = findKeyWord(accumulator, result);
        if (write(output_fd, result, processed_size) != processed_size) {
            perror("write to FIFO");
            exit(EXIT_FAILURE);
        }
    }

    close(input_fd);
    close(output_fd);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <input_fifo> <output_fifo>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int input_fd = open(argv[1], O_RDONLY);
  if (input_fd < 0) {
    perror("open input FIFO");
    exit(EXIT_FAILURE);
  }

  int output_fd = open(argv[2], O_WRONLY);
  if (output_fd < 0) {
    perror("open output FIFO");
    exit(EXIT_FAILURE);
  }

  processChunks(input_fd, output_fd);
  return 0;
}