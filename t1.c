
struct aa{
   int s1;
   int s2;
   char s3;
} aa;


int mytest(int a, int b, char c)
{
    char b1[2][2];
 
    char buf[16];
    int i1;
    struct aa a1;
    struct aa *paa;	
    short int qq;
    short int ww;
    
    ww = qq = 0x0fff;
    i1 = 1;
    i1 = 1;
    b1[1][i1] = 'a';	
    b1[1][1] = 'a';	
    buf[15] = '0';	
    buf[i1] = '1';

    a1.s2 = 0xdeadeaf;
    a1.s1 = 0x1234567;
    
    paa = &a1;
    paa->s1 = 0xaaa;
    paa->s2 = 0xbbb;
    (*paa).s3 = '0';

    (*paa).s3 = '1';
    
    return 0;
}

void haha()
{
	long int l1;
	int l2;
	unsigned int ul1;
	unsigned long int ul2;
  short s1;
  short s2;
  unsigned short us1;
  unsigned short us2;
  char c1;
  char c2;
  unsigned char uc1;
  unsigned char uc2;
  
  l1 = 0xdeadbeaf;
  l1 = l2;
  l1 = ul2;
  ul1 = l2;
  ul2 = l1;
  
  l1 = 0xdeadbeaf;
  l1 = s1;
  ul1 = s2;
  ul1 = us1;
  l1 = us1;
  l1 = c1;
  ul1 = c2;
  ul1 = uc1;
  l1 = uc1; 
  s1 = c1;
  us1 = c2;
  us1 = uc1;
  s1 = uc1; 
    
  c1 = c2;
  c2 = uc1;
  uc1 = uc2;
  uc1 = c1;
  s1 = s2;
  us1 = s1;
  s1 = us2;
  us2 = us1;
   
 
  l1 = 0xdeadbeaf;
  c1 = l1;
  c1 = ul1;
  uc1 = l1;
  uc1 = ul1;
  s1 = l1;
  s1 = ul1;
  us1 = l1;
  us1 = ul1;
  l1 = 0xdeadbeaf;
  c1 = s1;
  uc1 = s1;
  c1 = us1;
  uc1 = us1;
}

void haha2()
{
	char *pc;
	char *pi;
  short s;
  char c;

  c = pi;
  s = pc;
  c = pc;
  s = pi;
  pi = s;
  pi = c;
  pc = s;
  pc = c;
}

void func()
{
	
}

typedef struct st{
	int i;
	char c;
}st;

typedef struct {
	int i;
	char c;
	struct aa  a;
	struct aa  *pa;	
}stt;

	
void func2()
{
	st st1;
	st st2;
	char c;
	
	st1 = st2;
	
	st1.c = c;
	st1.i = c;
}


void func3(a,b,c)
int a; char b[]; char c;
{
	a = 1;
	b[7] = 2;
	c = 'c';
}


void func4(int a, char v, short b, char c)
{
	a = 1;
	v = '0';
	b = 2;
	c = 'c';
}

void func5(struct aa mm, char v, int a)
{
	mm.s1 = 9;
	v = mm.s1;
	v = 'a';
	a = 1;
}


void func6(char buf[100], char v, int a)
{
	char *p;
	char c;
	
	v = 'a';
	a = 1;
	p[2] = 'x';  
	(&c)[3] = 'y';  
	buf[5] = 0;     
}


void func7()
{
	char * buf[10];
	char c;
	
	buf[9][2] = 0xff;
	c = buf[9][2];

}

void func8()
{
	st * buf[10];
	st c;
	
	c = buf[9][2];

}


typedef struct {
   int s1;
   int s2;
   char s3[11164];
} s;

void func9()
{
		s s1;
		s s2;
		
		s1 = s2;
}

char ffff()
{
   return 'c';	
}

void func10()
{
  char *p1;
  char *p2;
  char *p3;
  char *p4;
  char *p5;
  char *p6;
  char *p7;
  char *p8;
  char buf[10];
  int gap;
  st s1;
  st *p;
  stt *px;
  
  *p1 = *p2 = *p3 = buf[1];	
  gap = 0x11111111;
  *p1 = *p2 = *p3 = *p4 = *p5 = *p6 = '5';
  gap = 0x22222222;
  *p1 = *p2 = *p3 = *p4 = *p5 = *p6 = buf[1] = buf[2];
  gap = 0x33333333;
  *p1 = *p2 = *p3 = *p4 = *p5 = *p6 = *p7 = *p8;
  gap = 0x44444444;
  *p1 = *p2 = *p3 = *p4 = *p5 = *p6 = *p7 = &gap;
  gap = 0x55555555;
  *p1 = *p2 = *p3 = *p4 = *p5 = *p6 = *p7 = (*(&s1)).c; 
  gap = 0x66666666;
  gap = (*(&s1)).c; 
  (*(&s1)).c = 0xfae;
  gap = 0x77777777;
  gap = &(p->i);  
  gap = &(p->c);  
  gap = 0x77777777;
  gap = &(*p);   
  gap = *(&p);
  gap = 0x88888888;
  gap = px->pa->s2;
  gap = 0x88888888;
  gap = &(px->pa->s2);
  gap = 0x88888888;
  gap = &(px->a.s2);
  gap = 0x88888888;
  gap = px->a.s2;    
  gap = 0x99999999;
  *p1 = *p2 = *p3 = *p4 = *p5 = *p6 = *p7 = ffff();
  gap = 0x11111111;
  *p1 = ffff();
}

func_()
{
  st s1;
  st *p;
  
  *p = s1;   	
}

int func11()
{
	int a,b;
	int i;
	char c;

	i = mytest(a, b, c);

	return i;
}

int func12()
{
	int a,b;
	int i;
	char c;

	i = mytest(a, mytest(a=0xbeaf, b, c), c);

	return i;
}

struct st func13(int a)
{
	 st s;
   // s s1;  // should be error 
	 a = 4;
	 
	 s.i = 10;
	 return s;
}


st func14(int a)
{
	 st s;
	 a = 4;
	 
	 return s;
}

func15()
{
   int *a;
   int gap;
	
	 gap = *a;
	 *a;
   &(*a);   
   &*a;
   gap = &(*a); 
   gap = &*a; 
   
}

void func16(int a, int b)
{
	
}

typedef int INT32;
INT32 func17(int a)
{
	 st s;
	 a = 4;
	 int *b;
	 
	 b = &a;
	 
	 ffff();
	 func16(10, *b);
	 func14(10);
	 s = func14(10);
	 return a;
}


func18()  // debug
{
   int *a;
   int gap;
	
	 gap = *a;
	 *a;
   &(*a);   
   &*a;
   gap = &(*a); 
   gap = &*a; 
   
}

int func19(int *p1, int *p2, int *p3, int *p4, int *p5)
{
	return 10;
}

func20()
{
	int i;
	int a,b,c,d,e;
	
	i = func19(&a,&b,&c,&d,&e);
}

int printf(char *format, ...);

func21()
{
   char *f = "Hello, World! %x\n";  // TODO
   int a;
   
   a = 0xdeadbeaf;
   
   printf(f, a);
   	
}

/* 
// TODO: global var initialize 
int aaaa = 10;    // TODO
//int bbbb = aaaa;  // error
char ssss[] = "hello";
char ssss1[10] = "hello2";

*/

void test_initializer()
{
  int aaaa = 10; 
  int bbbb = aaaa;  
  int AR[2][3][3] = {0xbeaf, [0][2][1] = 3, [0][0][2] = 6, 57, 56, 56, [1] = 6,7,8,9,10};
  
  struct {int a[2]; int b;} y[2] = {{.a[1] = 7, 8}, 1, 2, 3};

  typedef struct {int a[2]; int b;} st;
  st y2[2] = {{.a[1] = 7, 8}, 1, 2, 3};


  //char c[2] = {'a', 'b', 'c'};   // error
  char c[2] = {'a', 'b'};


  typedef union {
    int a[2];
	int c[10];
	int b;
  } u;

  //u u1 = {1,2,3};  // error 
  u u1 = {1,2};
  u u2 = {.a[1]=4, .c[0]=5};


  struct w1 {int a; char b;} w = {1,'2'};

  struct {struct w1 ww0, ww1; int d;} WW= {{1,2},{3},5};

  int AR2[2][3][3] = {[0][2][1] = 3, [0][0][2] = 6, 45};
  int AR3[2][2] = {1,3,{4, 5}};
  
  //char ssss[] = "hello";  // TODO
  //char ssss1[10] = "hello2";   // TODO
}


void init2()
{
	//char str[2][4][2];
	//char c, str2[2]; 
	
	int a[][2] ={{2,3},4};  
  int b, b2[][2] ={{2,3},4};
  
 /* error 
  char ch1[10];
  char ch2[10];
  ch1 = ch2;
 */ 
}


int func23()
{
	 char c;
	 int i;
	 unsigned ui;
	 long l;
	 char *pc;
	 long *pi;
	 st *pst;
	 
	 c++;
	 i++;
	 ui++;
	 l++;
	 pc++;
	 (*pc)++;
	 pi++;
	 pst++;
	 l = *(pi++);	
	 
	 l = 0xfff;
	 c = *pc++;
	 c = (*pc)++;
}

int func24()
{
   int i;
   int i2 = i;	
   int a;
   
   *&a;
}

int func25()
{
	int i1,i2,i3;
	
	i1 = --i2,i3--;
	i1 = (--i2,i3--);
	i1--;
	i1 = sizeof i1;
}

int func26()
{
	int i;
	int i1 = i++;
	int i2;
	
	i2 = +i;
	
}

int func27()
{
  char c;
  unsigned char uc;
  int i;
  unsigned int ui;
  
  uc = -c;
  ui = -i;
  ui = -c;	
}

int func28()
{
  char c;
  unsigned char uc;
  int i;
  unsigned int ui;
  
  uc = ~c;
  ui = ~i;
  ui = ~c;	
}

int func29()
{
  char c;
  unsigned char uc;
  int i;
  unsigned short us;
  char *p;
  	
  c = !us;
  i = !c;
  i = !*p;
}

int func30()
{
  char c1, c2;
  unsigned char uc1, uc2;
  int i1,i2;
  unsigned long ul1, ul2;
  short s1,s2;
  unsigned short us1,us2;
  char *p;
  	
  c1 = c1 > c2;
  i1 = uc1 < uc2;
  i1 = i1 <= i2;
  i1 = s1 >= s2;
  i1 = ul1 == ul2;
  i1 = us1 != us2;
}

int func31()
{
  char c1, c2;
  unsigned char uc1, uc2;
  int i1,i2;
  unsigned long ul1, ul2;
  short s1,s2;
  unsigned short us1,us2;
  int gap;
  	
  c1 = c1 + c2;
  gap = 0x1111;
  i1 = i1 + i2;
  gap = 0x2222;
  i1 = uc1 - uc2;
  gap = 0x3333;
  i1 = i1 - i2;
  gap = 0x4444;
  i1 = i1 * i2;
  gap = 0x5555;
  ul1 = ul1 * ul2;
  gap = 0x6666;
  i1 = us1 & us2;
  gap = 0x7777;
  c1 = c1 | c2;
  gap = 0x8888;
  i1 = s1 ^ s2;
  gap = 0x9999;
  i1 = i1 >> i2;
  gap = 0x0000;
  i1 = ul1 << ul2;
}


int func32()
{
  char c1, c2;
  unsigned char uc1, uc2;
  int i1,i2;
  unsigned long ul1, ul2;
  short s1,s2;
  unsigned short us1,us2;
  int gap;
  	
  i1 = s1++ / s2;
  gap = 0x1111;
  i1 = us1 / us2;
  gap = 0x2222;
  i1 = ul1 % ul2;
}


int func33()
{
  char c1, c2;
  unsigned char uc1, uc2;
  int i1,i2;
  unsigned long ul1, ul2;
  short s1,s2;
  unsigned short us1,us2;
  int gap;
  	
  i1 /= s2;
  gap = 0x1111;
  i1 /= us2;
  gap = 0x2222;
  i1 %= ul2;
}

int func34()
{
  int i;
  int i2;
  
  i = (i2 && i); 	
  
  i = (i2 || i);	
}

int func35()
{
   int i,a;
   int i1;
   char c;
   
   i = (a? i1:c);	
}

int func36()
{
   st st1,st2;
   int i;
   
   st1 = st2;
   st1 = (i? st1:st2);	
}

int func37()
{
  int i;

  if(i) 
	  if(i) 
	    i = 1; 
	  else 
	    i = 2;
  else 
    i = 4;
  	
}

int func38()
{
  int i;

  while(i) {
    i++;
  }
}

int func39()
{
  int i;

  do {
    i++;
  }while(i);
}

int func40()
{
	int i1;
	int i;
	
  for (i=0; i1; i++)
  {
  	i1++;
  }	
}

int func41()
{
	int i1;
	int i;
	
  for (; ;)
  {
  	i1++;
  }	
}

int func42()
{
	int i1;
	int i;
	
  for (i=0; ; )
  {
  	i1++;
  }	
}

int func43()
{
	int i1;
	int i;
	
  for (i=0; i1; )
  {
  	i1++;
  }	
}

int func44()
{
	int i1;
	int i;
	
  for (; i1; )
  {
  	i1++;
  }	
}

int func45()
{
	int i1;
	int i;
	
  for (;;i++)
  {
  	i1++;
  }	
}

int func46()
{
	int i1;
	int i;
	
  for (i=0; ; i++)
  {
  	i1++;
  }	
}

int func47()
{
	int i1;
	int i;
	
  for (; i1; i++)
  {
  	i1++;
  }	
}

int func48()
{
	int i1;
	int i;
	
  for (; i1<i; i++)
  {
  	i1++;
  }	
}

int func49()
{
	int i1;
	int i;
	
  for (; i1<i; i++)
  {
  	i1++;
  	//if(i == i1)   // fix this
  		goto out;
  }	
  
out:  
	i1++;
}


int func50()
{
	int i1;
	int i;
	
  for (; i<i1; i++)
  {
    continue;
  }	
  
}

int func51()
{
	int i1;
	int i;
	
  while(i)
  {
    continue;
  }	
  
}

int func52()
{
	int i1;
	int i;
	
  do
  {
    continue;
  }	while(i);
  

}

int func53()
{
  int i;
  
  switch(i) {
   	case 1:
   	case 2:
   	case 3:
   		++i;
   	default:
   		i+=i;
  };	
	
}

int func54()
{
  int i;
  
  switch(i) {
   	case 1:
   	case 2:
   		break;
   	case 3:
   		++i;
   	default:
   		i+=i;
  };	
	
}

