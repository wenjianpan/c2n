signed TS_SIGNED;    
unsigned TS_UNSIGNED;
char TS_CHAR;
signed char TS_SCHAR;
unsigned char TS_UCHAR;
int  TS_INT;
signed int TS_SINT;
unsigned int TS_UINT;
short TS_SHORT;
signed short TS_SSHORT;
unsigned short TS_USHORT;
short int TS_SHORT_INT;
signed short int TS_SSHORT_INT;
unsigned short int TS_USHORT_INT;
long TS_LONG;
signed long TS_SLONG;
unsigned long TS_ULONG;
long int TS_LONG_INT;
signed long int TS_SLONG_INT;
unsigned long int TS_ULONG_INT;
float TS_FLOAT;   
long long TS_LONG_LONG;
signed long long TS_SLONG_LONG;
unsigned long long TS_ULONG_LONG;
long long int TS_LONG_LONG_INT;
signed long long int TS_SLONG_LONG_INT;
unsigned long long int TS_ULONG_LONG_INT;
double TS_DOUBLE;
long double TS_LONG_DOUBLE;
double long TS_DOUBLE_LONG;  


typedef int III;


int a_func(int a)
{
}

int a_func(int a);
int a_func(a);


int b_func(a, b, c)
register int a,b;
int c();
{
	
}   
   
enum en;
enum en;


enum en {A,B,C};
enum {D=2,E=4, F} enn;

enum en en1;

typedef void (*ptr_to_func) (int);

ptr_to_func signal2(int, ptr_to_func);
void * const func(int, ...);
const char *pc;
void restoretoken2(char *s);
int f1;
char a;
extern char *ar[];

typedef char t_a;
t_a cha;

char (*ar2)[10];

void b(int *, int b, int c);
void ffffffff(void);
void (*signal(int sig, void (*func)(long int a, int signed b)))(int b);  
typedef int *INT;
typedef INT * p;

typedef p pp;

typedef int const *ii;
extern ii * const p2;

pp f2();

long int li; 

unsigned const long typedef int volatile *kum;

typedef int int1;
volatile  int1 const int2;

 char r( p param);


 const p extern extp;

void v( const void *);

int function(a,b,c);

typedef int ia[10];

ia *ia_p;

void f__(ia *, int a);



int i1,i2,*i3;

struct a;
struct a;
struct a{ int v;
};


union {
    int c;
}cc1, cc2, cc3;

union uuu{int a;};

typedef union ttt{
   int v;
} ttt;

  
typedef struct {

  struct {
    int b;
  }b;

	struct a am;
  struct empty {
     char *p;   
     
 /*    struct empty {   // error    
     	 int i;
     } e;  */
     
     struct empty *ep;
  };
  
  struct empty ppp;
  
  union c {char *p;} c1;
  
  ttt ll;
	int a;
	
}at;

at c1, c2, c3;
at *c4;
     
struct empty out_ppp;

unsigned si;

long li1;

long signed const lsi;
signed long  lsi2;

struct ss{
	int ss; 
};

struct {

	int a :1;
	int b :1;
	int c :1;
	t_a ch :4;
	at aa;
	float ff;
	char *p;
}b_b, *p_bb;

struct {

	int :1;
	const int b :1, c:1;
	double f;
}a_a;

extern at aa;
struct ss aaaaa;

typedef char *ppc, c_, ffc(int);

/* ffc ffffc();    // error */

typedef int _;

typedef char a_[100];

a_ *px;

/* a_ FFF();       error */

/* ffc bbb[10];    error */

int Fu(struct ss ss, char);
int Fu1(struct ss{int g;} ss, char, ...);

int Fu2(struct ss__SS{int g;} ss, char, ...);

/* struct ss__SS ffff__SS;   //error */


struct struct_a;
/*
struct struct_a *p_ddd;
struct struct_a ___b;
*/

struct ss ss_a[10];

char *ps;
void print(char *s, ...){}

int function2(a);

 int function2(float a);

int function2(float a);

function2(float a) {

}

 main()
{	

	03545;
	55ull;
	5ul;
	5lu;
	5LLu;
	4675486;
	45.6565;
	-675;
	100.;
	.565;
	.100e-1;
	0xa4.44P0f;
	0x.4AP-1;
	'\333';
	"hello\n";
	'\x5454';
	L'\x3444';
	L"hello\n";
	'c';
	'\?';
			
	++si;
	--si;
	+si;
	-si;
	*p_bb;
	sizeof(int);
	aaaaa.ss;
	b_b.a;
	b_b.p;
	p_bb;
	p_bb->a;
	p_bb->p;
	&p_bb;

	si++;
	si--;
	++p_bb->a;
	/* &p_bb->a; */

	c1.c1.p;
	c4->c1.p;
	*c4;

	ss_a->ss;  
	ar; 
	ar[1];
	ar+1; 
	/*c1->c1;   //error*/
	a_func;
	(long)si;
	(int *)p_bb;

	si  = 4;
	si >>= 4*8-3;
	*(ar+1);
	&ar[3];

	(&b_b)->aa;

	c4->a += c4->a + 67;

	a_func((long int)1);
	a_func(1);

	print(ps);
	print(ps, 1 == 4, (&b_b)->aa);

	(&b_b)->aa, si, 4;
 
}

a_func(int d);

const *nn;

*m1,m2, m4;

arr[20];


//int f(int (*)(), double (*)[3]);
//int f(int (*)(char *), double (*)[]);

/*
test comment
*
  //
*/


int blala(int pa)
{
	pa++;
	c4->a = pa;
	int i;
	

	for(i = 0; i < 10; i++)
		arr[i] = 9;

	if(1) ;
	else {
		int a;
		a = 0x12;
	}
	return 0;
}

int haha()
{

	int aa;
	struct ll {
		int a;
	};
	struct ll  c1;


	HAHA2:

	c1.a = sizeof(struct mm {int m;});

	struct mm la;

goto HAHA;

HAHA:
	la.m = sizeof(5+7);
	goto HAHA2;

}

struct tnode {
	int count;
	struct tnode *left, *right;
};

void haha2()
{
	int haha(int a);
	int haha(int a);

	haha(1);
	struct tnode tn;
	tn.left = &tn;
	tn.right = &tn;

	int i;
	
LABEL:
	
	switch(i) {
		case 3:
			i = 1;
			while(1) {
			  case 1: 
				print("ao\n");
				break;
			  case 2:
				break;
			  default:
				break;
			}
			print("break while\n");
		break;
		case 4:
			print("first\n");
		case 5:
			print("seconed\n");
			break;
	}



	do {
		struct tnode * ptn;
		ptn->left = ptn->right = &tn;   // test
		switch(i)
			case (1): continue;
	} while (1);

	//ptn = 0;
}   


typedef struct a2 haha3, haha4;


struct a2;

struct a2 {int ba;};


struct a2 bd;
haha3 bd2;
haha4 bd3;

enum ena{N};
//enum ena{M};  // error
enum ena enb;

typedef struct aaal{int b;};
struct aaal aaal;


int test_struct()
{
	bd2.ba;
	bd3.ba;
	aaal.b;
}



//int fff3(){}, ii1, ii2;         // error
int fff4(a);
int fff4(b,c);
int fff4(a, b, c)       
float c,a;
{
	//int c = li = 9;    // error
	//int a = test_struct();    // error
	int c1 = li = 9;    
	int a1 = test_struct();    
	c = 0.0;

	int aa=aa, bb=aa;

	*(&aa) = 1;
}

//typedef int III2 = 7;  // error
//int gfgf() = 5;   // error

