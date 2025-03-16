//
// Created by Сергей Растворов on 14/3/25.
//
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 5000

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

void readFromPipe(const char *fifo, char *buffer) {
  int fd = open(fifo, O_RDONLY);
  if (fd < 0) {
    perror("open FIFO for reading");
    exit(EXIT_FAILURE);
  }

  ssize_t read_bytes = read(fd, buffer, BUFFER_SIZE);
  if (read_bytes < 0) {
    perror("read from FIFO");
    close(fd);
    exit(EXIT_FAILURE);
  }

  buffer[read_bytes] = '\0';
  close(fd);
}

void sendToPipe(const char *fifo, const char *buffer, ssize_t size) {
  int fd = open(fifo, O_WRONLY);
  if (fd < 0) {
    perror("open FIFO for writing");
    exit(EXIT_FAILURE);
  }

  if (write(fd, buffer, size) != size) {
    perror("write to FIFO");
    close(fd);
    exit(EXIT_FAILURE);
  }

  close(fd);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <input_fifo> <output_fifo>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  readFromPipe(argv[1], buffer);
  ssize_t processed_size = findKeyWord(buffer, buffer);
  sendToPipe(argv[2], buffer, processed_size);

  return 0;
}