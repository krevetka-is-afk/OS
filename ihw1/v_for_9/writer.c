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

void writeToFile(const char* path, int fd) {
  char buffer[CHUNK_SIZE];
  int file = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (file < 0) {
    perror("open output file");
    exit(EXIT_FAILURE);
  }

  ssize_t bytes_read;
  while ((bytes_read = read(fd, buffer, CHUNK_SIZE)) > 0) {
    if (write(file, buffer, bytes_read) != bytes_read) {
      perror("write to file");
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
    fprintf(stderr, "Usage: %s <fifo> <output_file>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror("open FIFO for reading");
    exit(EXIT_FAILURE);
  }

  writeToFile(argv[2], fd);
  return 0;
}