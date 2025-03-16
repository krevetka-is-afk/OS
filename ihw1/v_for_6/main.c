//
// Created by Сергей Растворов on 12/3/25.
//
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#define BUFFER_SIZE 5000

ssize_t readFromFile(const char *path, char *buffer, size_t size) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open input file");
        exit(EXIT_FAILURE);
    }

    ssize_t read_bytes = read(fd, buffer, size);
    if (read_bytes == -1) {
        perror("read");
        close(fd);
        exit(EXIT_FAILURE);
    }

    buffer[read_bytes] = '\0';

    if (close(fd) < 0) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    return read_bytes;
}

ssize_t writeToFile(const char *path, const char *buffer, size_t size) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("open output file");
        exit(EXIT_FAILURE);
    }

    ssize_t written = write(fd, buffer, size);
    if (written != size) {
        perror("write");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (close(fd) < 0) {
        perror("close");
        exit(EXIT_FAILURE);
    }

    return written;
}

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
        strncat(result, temp, BUFFER_SIZE - strlen(result) - 1);
    }

    return strlen(result);
}

// Функция для дочернего процесса (обработка данных)
void childProcess(int *pipe_parent_to_child, int *pipe_child_to_parent) {
    char buffer[BUFFER_SIZE];
    
    // Закрываем неиспользуемые концы каналов
    close(pipe_parent_to_child[1]); // Закрываем конец для записи в канал от родителя к ребенку
    close(pipe_child_to_parent[0]); // Закрываем конец для чтения из канала от ребенка к родителю
    
    // Читаем данные из канала от родителя
    ssize_t read_bytes = read(pipe_parent_to_child[0], buffer, BUFFER_SIZE);
    if (read_bytes < 0) {
        perror("child: read from pipe");
        exit(EXIT_FAILURE);
    }
    
    // Закрываем канал после чтения
    close(pipe_parent_to_child[0]);
    
    // Обрабатываем данные
    ssize_t result_size = findKeyWord(buffer, buffer);
    
    // Отправляем результат обратно родителю
    if (write(pipe_child_to_parent[1], buffer, result_size) != result_size) {
        perror("child: write to pipe");
        exit(EXIT_FAILURE);
    }
    
    // Закрываем канал после записи
    close(pipe_child_to_parent[1]);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *input = argv[1];
    char *output = argv[2];
    char buffer[BUFFER_SIZE];

    // Создаем два канала: 
    // pipe_parent_to_child: родитель -> ребенок
    // pipe_child_to_parent: ребенок -> родитель
    int pipe_parent_to_child[2], pipe_child_to_parent[2];

    if (pipe(pipe_parent_to_child) < 0 || pipe(pipe_child_to_parent) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Дочерний процесс
        childProcess(pipe_parent_to_child, pipe_child_to_parent);
        exit(EXIT_SUCCESS);
    }

    // Родительский процесс
    
    // Закрываем неиспользуемые концы каналов
    close(pipe_parent_to_child[0]); // Закрываем конец для чтения из канала от родителя к ребенку
    close(pipe_child_to_parent[1]); // Закрываем конец для записи в канал от ребенка к родителю
    
    // Читаем данные из входного файла
    ssize_t read_bytes = readFromFile(input, buffer, BUFFER_SIZE);
    
    // Отправляем данные дочернему процессу
    if (write(pipe_parent_to_child[1], buffer, read_bytes) != read_bytes) {
        perror("parent: write to pipe");
        exit(EXIT_FAILURE);
    }
    
    // Закрываем канал после записи
    close(pipe_parent_to_child[1]);
    
    // Получаем обработанные данные от дочернего процесса
    read_bytes = read(pipe_child_to_parent[0], buffer, BUFFER_SIZE);
    if (read_bytes < 0) {
        perror("parent: read from pipe");
        exit(EXIT_FAILURE);
    }
    
    // Закрываем канал после чтения
    close(pipe_child_to_parent[0]);
    
    // Записываем результат в выходной файл
    writeToFile(output, buffer, read_bytes);

    return 0;
}