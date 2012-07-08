#include <stdio.h>

int main()
{
  char s[] = "hellosffdsfsd";
  int a = 4;
  int b = 5;
  int AR[2][3][3] = {a+b, [0][2][1] = 3, [0][0][2] = 6, 57, [1] = 6,7,8,9};
     
  printf("size of %d\n", sizeof(s)); 
  printf("%d %d %d %d %d %d %d %d\n", AR[0][0][0], AR[0][2][1], AR[0][0][2], AR[0][1][0],
              AR[1][0][0], AR[1][0][1], AR[1][0][2], AR[1][1][0]);

  int i1=10;
  int i2=-10;
  printf("%d %d\n", i1, i2);
  
  char c1,c2,c3,c4;
  char ccc[] = {c1,c2,c3,c4+c3};
  printf("sizeof ccc %d\n", sizeof ccc);
}


