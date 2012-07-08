#include <stdio.h>

struct aa
{
	int a,b,c,d;
	char c_ar[10];
} aa = {.a = 1, .c_ar[0] = 'M', .c = 3};

int ar[] = {[0] = 9, [1] = 5, [6] = 8};

struct
{
	struct aa a;
	int b,c,d;
} bb;
	
int f()
{
   return 99;	
}

struct { int a[3], b; } w[] = {1,2,3,4, {2,2,2,2}};

int main()
{
	struct aa aa = {.a = f(), .c_ar[0] = 'M', .c = 3};
	
	printf("%d %d %d %d %c\n", aa.a, aa.b, aa.c, aa.d, aa.c_ar[0]);
	printf("%d %d %d %d\n", bb.a, bb.b, bb.c, bb.d);
	printf("sizeof ar[] is %d\n", sizeof(ar));
	
	int i = {f()};
	for(i = 0; i < sizeof(ar)/sizeof(int); i++) {
		struct aa a = {.a = 1, .c_ar[0] = 'M', .c = 3};
		printf("a[%d] = %d\n", i, ar[i]);
	}
	
	printf("%d %d %d %d \n", w[0].a[0], w[0].a[1], w[0].a[2], w[0].b);
	printf("%d %d %d %d \n", w[1].a[0], w[1].a[1], w[1].a[2], w[1].b);
}

