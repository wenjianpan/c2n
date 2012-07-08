

struct a {
	int v;
 char b;

 
};   // 8

struct b {
	int v;
 char b;
 int c;
 
};   // 12

struct c{
	 char d;
   char a[10];
   char b;
   int c;	
};   // 16

union u {
	 char a;
	 int b;
	 char c[5];
	 struct c ll;
};


int func(char p1, int p2, void * p3)
{
	 int temp;
	 
   p1 = 'a';
   p2 = 0;
   p3 = (void *)0;
   
   return 0;	
}
 

int main()
{
	  char str[12];
    struct a aa;
    struct b bb;
    struct c cc;
    struct c *pc = &cc;
    union u uu;
    
    
    char v1;
    int v2 = -1;
    unsigned int v3 = 7;
    int nn,mm = 0;
    
    
    str[6] = 'c';
    aa.b = 'x';
    pc->c = 88;
    pc->a[4] = 'v';
    uu.ll.a[3] = 'l';
    v2 = nn++;
    v2 = nn--;
    v2 = --nn;
    v2 = ++nn;
    v2 = nn + mm;
    v2 = nn - mm;
    v2 = nn * mm;
    v2 = nn / mm;
    v2 = nn % mm;
    v2 = nn & mm; 
    v2 = nn | mm;
    v2 = ~nn;
    v2 = nn ^ mm;
    
    v2 = nn >> 1;
    v2 = nn << 1;
    
    v2 += mm;
    v2 -= mm;
    v2 *= mm;
    v2 /= mm;
    v2 %= mm;
    v2 >>= mm;
    v2 <<= mm;
    v2 &= mm;
    v2 |= mm;
    v2 ^= mm;
    
    v2=v2=mm, mm=nn;
    func(v1, v2, (void*)pc); 
    
    return 0;
}

