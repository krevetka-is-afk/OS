//
// Created by Сергей Растворов on 8/3/25.
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
        perror("open");
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
        perror("open");
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

void readProcess(int *fdReader, char *buffer, const char *input) {
    close(fdReader[0]); // Закрываем конец для чтения

    ssize_t read_file = readFromFile(input, buffer, BUFFER_SIZE);
    if (write(fdReader[1], buffer, read_file) != read_file) {
        perror("write to pipe");
        exit(EXIT_FAILURE);
    }

    close(fdReader[1]); // Закрываем конец для записи
}

void HandleProcess(int *fdWriter, int *fdReader, char *buffer) {
    close(fdReader[1]); // Закрываем ненужный конец

    ssize_t read_pipe = read(fdReader[0], buffer, BUFFER_SIZE);
    if (read_pipe < 0) {
        perror("read from pipe");
        exit(EXIT_FAILURE);
    }

    ssize_t count = findKeyWord(buffer, buffer);
    close(fdReader[0]); // Закрываем после чтения

    if (write(fdWriter[1], buffer, count) != count) {
        perror("write to pipe");
        exit(EXIT_FAILURE);
    }

    close(fdWriter[1]); // Закрываем конец записи
}

void WriteProcess(int *fdWriter, char *buffer, const char *output_name) {
    close(fdWriter[1]); // Закрываем конец записи

    ssize_t read_pipe = read(fdWriter[0], buffer, BUFFER_SIZE);
    if (read_pipe < 0) {
        perror("read from pipe");
        exit(EXIT_FAILURE);
    }

    ssize_t write_count = writeToFile(output_name, buffer, read_pipe);
    if (write_count != read_pipe) {
        perror("write to file");
        exit(EXIT_FAILURE);
    }

    close(fdWriter[0]); // Закрываем конец чтения
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *input = argv[1];
    char *output = argv[2];
    char buffer[BUFFER_SIZE];

    int fdReader[2], fdWriter[2];

    if (pipe(fdReader) < 0 || pipe(fdWriter) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        WriteProcess(fdWriter, buffer, output);
        exit(EXIT_SUCCESS);
    }

    int reader_pid = fork();
    if (reader_pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (reader_pid == 0) {
        HandleProcess(fdWriter, fdReader, buffer);
        exit(EXIT_SUCCESS);
    }

    readProcess(fdReader, buffer, input);
    return 0;
}