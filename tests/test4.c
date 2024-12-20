// Increasing size test using calloc
// check each location is set to 0

#ifndef DEMO_TEST
#include <malloc.h>
#else
#include <stdlib.h>
#endif

#include <stdio.h>
#include <string.h>

int allones = ~0; // allones for int

int main() {

  int i, j;
  size_t size;

  fprintf(stderr, 
      "=======================================================================\n"
      "This test uses calloc to allocate gradually allocate 2^2, 2^3, ...,\n"
      "2^29 bytes of memory. It checks whether the memory is set to 0. No more\n"
      "=======================================================================\n");

  for (i = 2; i < 30; ++i) {
    size = 2UL << i;
    fprintf(stderr, "%zu bytes...", size);
    int *data = (int *) calloc(1, size);
    if (data != NULL) {
      // test if all elements of data are ==0
      for (j = 0; j < size / sizeof(int); ++j) {
      }
      free(data);
    }

    else {
      fprintf(stderr, "\nMax size allocated: %lu bytes\n", 2UL << (i - 1));
      break;
    }
  }
  fprintf(stderr, "\n");
  return 0;
}
