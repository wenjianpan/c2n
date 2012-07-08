/*
5, 6, 8
3 6 57 6 7 9
1 3 4 5
*/


struct {int a[2]; int b;} y[2] = {{.a[1] = 7, 8}, 1, 2, 3};

struct w1 {int a; int b;} w;

struct {struct w1 ww0, ww1; int d;} WW= {{1,2},{3},5};

int AR[2][3][3] = {[0][2][1] = 3, [0][0][2] = 6, 45};
int AR2[2][2] = {1,3,{4, 5}};

extern int printf(char * str, ...);



union e4 {struct w1 w; char a; int b; } u = {.w = 3,6};

struct MM {int a; struct w1 e;} M2 = {5, .e = 6, 8};

struct { int a[2][2][2]; int b; int c; } M3 = {3,4,6};

struct {
   struct {
	struct w1 ww0;
	int m;
	struct w1 ww1;
   } ss;

   int a;
   char *p;
} stt = {{.m = 5}, 6, 0xffff};

void func(int pra, int prb)
{
	return;
}

int main(int argc, char * argv[]) 
{
   int a = 1;
   int b = 2;
   int d = b;
   int c = a += b;
   int AR[2][3][3] = {[0][2][1] = 3, [0][0][2] = 6, 57, [1] = 6,7,8,9};

   printf("%d, %d, %d\n", M2.a, M2.e.a, M2.e.b);
   printf("%d %d %d %d %d %d\n", AR[0][2][1], AR[0][0][2], AR[0][0][3], AR[1][0][0], AR[1][0][1], AR[1][1][0]);
   printf("%d %d %d %d\n", AR2[0][0], AR2[0][1], AR2[1][0], AR2[1][1]);

   return 0;
}

