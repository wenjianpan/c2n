#include <stdio.h>
#include <math.h>

void print_float(float f)
{
		long *p = &f;
    unsigned long l1,l2,l;
    int i;
    
    double fd = 0.0;
    double ld;
    
		printf("%f = \n", f);
		
		l1 = ((*p) & 0x80000000) >> 31;
		l2 = ((*p) & 0x7F800000) >> 23;
		printf("(-1)^%d * 2^(%d - 127) * (1", l1, l2);
		
		if(l1)
			ld = -1 * ldexp(1, (l2-127));
		else
			ld = ldexp(1, (l2-127));
		
		l = (*p)& 0x007fffff;
		
		for (i = 0; i<23; i++) {
			if(l & (0x00400000 >> i)) {
				printf(" + 2^(-%d)", i+1);
				fd = fd + 1.0/ldexp(1, (i+1));
			}
		}
		printf(")\n");

		ld = ld * (1.0 + fd);
		printf("calculate from above: %f\n\n\r", ld);
		
}

int main() 
{
    float f; 
		
		f = 100.e-1;
		print_float(f);
		f = 3.14159;
		print_float(f);
		f = 1.0;
		print_float(f);
		f = -1.0;
		print_float(f);
		f = 01.012312;
		print_float(f);
		f = 0.012312;
		print_float(f);
		f = -0.012312;
		print_float(f);
}


