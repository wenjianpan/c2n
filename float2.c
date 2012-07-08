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

float getfloat( char *pf);

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
		f = -0xa4.44P0;
		print_float(f);	
		f = -0xa4.44P1;
		print_float(f);			
		f = -0x.4AP-1;
		print_float(f);
		f = 0.1111;
		print_float(f);		
		
		f = 6666666666666666666666666693565.4545453;
		print_float(f);			

		f = 123426456562536156315652635162563546532645365463546354632565462354.4545453;
		print_float(f);		
		
		char *pf = "565.45454";
		printf("str: %s\n", pf);
		float dpf;
		dpf = getfloat(pf);	
		print_float(dpf);
				
		pf = "6666666666666666666666666693565.4545453";
		printf("str: %s\n", pf);
		dpf = getfloat(pf);	
		print_float(dpf);
		
		pf = "123426456562536156315652635162563546532645365463546354632565462354.4545453";
		printf("str: %s\n", pf);
		dpf = getfloat(pf);	
		print_float(dpf);		

}

float getfloat(char *pf)
{
	double f = 0.0;
	char c;
	double i = 1.0;
		
	while(1) {
	   c = *pf++;
	   if(c == '.') break;
	   f = f*10 + (c-'0');
	}
	c = *pf++;  /* c == '.' */
	
	while(1) {
	   c = *pf++;
	   if(c == '\0') break;
	   i = i/10.0;
	   f += (c-'0')* i;
	}	
	return (float)f;
}

