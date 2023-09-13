#include <time.h>

void intToBinary(int integer, char* buffer, int* offset);
void stringToBinary(char* string, char* buffer, int* offset);
void timestampToBinary(struct timespec time, char* buffer, int* offset);
void createHeader(char* buffer, int column_count, int* offset);
void createTrailer(char* buffer, int* offset);
