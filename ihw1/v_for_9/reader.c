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

#define CHUNK_SIZE 128

void readFromFile(const char* path, int fd) {
  char buffer[CHUNK_SIZE];
  int file = open(path, O_RDONLY);
  if (file < 0) {
    perror("open input file");
    exit(EXIT_FAILURE);
  }

  ssize_t bytes_read;
  while ((bytes_read = read(file, buffer, CHUNK_SIZE)) > 0) {
    if (write(fd, buffer, bytes_read) != bytes_read) {
      perror("write to FIFO");
      close(file);
      close(fd);
      exit(EXIT_FAILURE);
    }
  }

  close(file);
  close(fd);
}

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s <input_file> <fifo>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int fd = open(argv[2], O_WRONLY);
  if (fd < 0) {
    perror("open FIFO for writing");
    exit(EXIT_FAILURE);
  }

  readFromFile(argv[1], fd);
  return 0;
}