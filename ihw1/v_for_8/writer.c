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

void writeToFile(const char *path, const char *buffer, ssize_t size) {
  int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd < 0) {
    perror("open output file");
    exit(EXIT_FAILURE);
  }

  if (write(fd, buffer, size) != size) {
    perror("write to file");
    close(fd);
    exit(EXIT_FAILURE);
  }

  close(fd);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <fifo> <output_file>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char buffer[BUFFER_SIZE];
  readFromPipe(argv[1], buffer);
  writeToFile(argv[2], buffer, strlen(buffer));

  return 0;
}