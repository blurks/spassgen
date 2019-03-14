#include <stdio.h>

int main(int argc, char** argv)
{
  printf("char* ascii = \"");
  char c;
  for(c = 0x20; c<0x7f; c++) {
    putchar(c);
  }
  printf("\"\n");
}
