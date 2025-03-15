//
// Created by Сергей Растворов on 9/3/25.
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

ssize_t readFromFile(const char* path, char* buffer, size_t size) {
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

ssize_t writeToFile(const char* path, const char* buffer, size_t size) {
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

ssize_t findKeyWord(char* source, char* result) {
    char *keywords[] = {"int", "return", "char", "while", "for"};
    int counts[5] = {0};

    char *word = strtok(source, " ,.!?;:\"(){}[]\n\t");
    while (word != NULL) {
        for (int i = 0; i < 5; i++) {
            if (strcmp(word, keywords[i]) == 0) {
                counts[i]++;
            }
        }
        word = strtok(NULL, " ,.!?;:\"(){}[]\n\t");
    }

    char temp[BUFFER_SIZE];
    result[0] = '\0'; // Очищаем строку перед записью

    for (int i = 0; i < 5; i++) {
        snprintf(temp, sizeof(temp), "Ключевое слово \"%s\" встретилось %d раз(а)\n", keywords[i], counts[i]);
        strncat(result, temp, BUFFER_SIZE - strlen(result) - 1);
    }

    return strlen(result);
}

void readProcess(const char *first_pipe, char *buffer, const char *input) {
    memset(buffer, 0, BUFFER_SIZE);
    ssize_t read_file = readFromFile(input, buffer, BUFFER_SIZE);

    int fd = open(first_pipe, O_WRONLY);
    if (fd < 0) {
        perror("open FIFO for writing");
        exit(EXIT_FAILURE);
    }

    if (write(fd, buffer, read_file) != read_file) {
        perror("write to pipe");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);
}

void HandleProcess(const char *first_pipe, const char *second_pipe, char *buffer) {
    memset(buffer, 0, BUFFER_SIZE);

    int first_fd = open(first_pipe, O_RDONLY);
    if (first_fd < 0) {
        perror("open FIFO for reading");
        exit(EXIT_FAILURE);
    }

    ssize_t read_pipe = read(first_fd, buffer, BUFFER_SIZE);
    if (read_pipe < 0) {
        perror("read from pipe");
        close(first_fd);
        exit(EXIT_FAILURE);
    }

    close(first_fd);

    ssize_t count = findKeyWord(buffer, buffer);

    int second_fd = open(second_pipe, O_WRONLY);
    if (second_fd < 0) {
        perror("open FIFO for writing");
        exit(EXIT_FAILURE);
    }

    if (write(second_fd, buffer, count) != count) {
        perror("write to pipe");
        close(second_fd);
        exit(EXIT_FAILURE);
    }

    close(second_fd);
}

void WriteProcess(const char *second_pipe, char *buffer, const char *output_name) {
    memset(buffer, 0, BUFFER_SIZE);

    int fd = open(second_pipe, O_RDONLY);
    if (fd < 0) {
        perror("open FIFO for reading");
        exit(EXIT_FAILURE);
    }

    ssize_t read_pipe = read(fd, buffer, BUFFER_SIZE);
    if (read_pipe < 0) {
        perror("read from pipe");
        close(fd);
        exit(EXIT_FAILURE);
    }

    close(fd);

    if (writeToFile(output_name, buffer, read_pipe) != read_pipe) {
        perror("write to file");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <input_file> <output_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char* input = argv[1];
    char* output = argv[2];
    char buffer[BUFFER_SIZE];

    const char first_pipe[] = "first.fifo";
    const char second_pipe[] = "second.fifo";

    unlink(first_pipe);
    unlink(second_pipe);

    if (mkfifo(first_pipe, 0666) < 0 || mkfifo(second_pipe, 0666) < 0) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }

    int pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        WriteProcess(second_pipe, buffer, output);
        exit(EXIT_SUCCESS);
    }

    int reader_pid = fork();
    if (reader_pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (reader_pid == 0) {
        HandleProcess(first_pipe, second_pipe, buffer);
        exit(EXIT_SUCCESS);
    }

    readProcess(first_pipe, buffer, input);
    return 0;
}