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

#define BUFFER_SIZE 5000

void readFromFile(const char* path, char* buffer, size_t size) {
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
    perror("close input file");
    exit(EXIT_FAILURE);
  }
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
    fprintf(stderr, "Usage: %s <input_file> <fifo>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  readFromFile(argv[1], buffer, BUFFER_SIZE);
  sendToPipe(argv[2], buffer, strlen(buffer));

  return 0;
}