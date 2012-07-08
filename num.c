#include <stdio.h>
#include <math.h>

#include "C_type.h"

#define isoct(c)  (c >= '0' && c <= '7')
#define isnum(c)  (c >= '0' && c <= '9')
#define ishex(c)  (isnum(c) || (c >= 'a' && c <= 'f') || \
					(c >= 'A' && c <= 'F'))

int str2int(char *str, unsigned long long *out)
{
	unsigned long long n = 0, nn; 
	bool overflow = F;
	bool is_ll = F;
	int tmp;
	char c, *p = str;
	int ret = TS_LONG;
	
	//printf("%s\n", str);
	c = *p++;
	if(c == '0') {
		c = *p++;
		if(c == 'x' || c == 'X') {
			while(1){
				c = *p++;
				if(ishex(c)) {
					if(c <= '9') {
						tmp = c - '0';
					}else if(c <= 'F') {
						tmp = c - 'A' + 10;
					}else if(c <= 'f') {
						tmp = c - 'a' + 10;
					}
					
					nn = n;
					n = (n << 4) + tmp;

					if (n < nn)
						overflow = T;
				}else {
					break;
				}
			}
			
		}else if(isnum(c)) {
			while(isnum(c)) {
				if(!isoct(c)){
					printf("invalid digit \"%c\" in octal constant", c);
					exit(0);
				}
				nn = n;
            	n = (n << 3) + c - '0';

				if (n < nn)
					overflow = T;
				c = *p++;
			}
		}
	} else if(c >= '1' && c <= '9') {
		// dec 
		p--;
		while(1) {
			c = *p++;
			if(isnum(c)) {
				nn = n;
            	n = n * 10  + c - '0';

				if (n < nn)
					overflow = T;
			}
			else  { 
				break; 
			}
		}
	}

	if(c == 'l') {
		c = *p++;
		ret |= LONG_MASK;
		if(c == 'l'){
			is_ll = T;
			ret |= (LONG_MASK << 1);
			c = *p++;
			if(c == 'u' || c== 'U') {
				ret |= UNSIGNED_MASK;
				c = *p++;
			} else goto OUT;
		} else if( c == 'u' || c == 'U') {
			ret |= UNSIGNED_MASK;
			c = *p++;
		} else goto OUT;

	} else if(c == 'L') {
		c = *p++;
		ret |= LONG_MASK;
		if(c == 'L'){
			is_ll = T;
			ret |= (LONG_MASK << 1);
			c = *p++;
			if(c == 'u' || c== 'U') {
				ret |= UNSIGNED_MASK;
				c = *p++;
			} else goto OUT;
		} else if( c == 'u' || c == 'U') {
			ret |= UNSIGNED_MASK;
			c = *p++;
		} else goto OUT;		

	} else if(c == 'u' || c == 'U') {
		ret |= UNSIGNED_MASK;
		c = *p++;
		if(c == 'l'){
			c = *p++;
			ret |= LONG_MASK;
			if(c == 'l') {
				is_ll = T;
				ret |= (LONG_MASK << 1);
				c = *p++;
			} else goto OUT;
		}else if(c == 'L') {
			c = *p++;
			ret |= LONG_MASK;
			if(c == 'L') {
				is_ll = T;
				ret |= (LONG_MASK << 1);
				c = *p++;
			} else goto OUT;
		}else goto OUT;
	} 

OUT:

	if(c == 0){
		if(overflow) {
			printf("warning: integer constant overflow\n");
		}
		*out = n;
		return ret;
	} else {
		printf("invalid suffix '%s' on integer constant\n", p);
		exit(0);
	}
}


int  str2float(char *str, long double *out)
{
	long double f = 0.0;
	long double i = 1.0;
	long long n = 0;
	bool minus = F;
	int tmp;
	int ret = TS_FLOAT;
	char c, *p = str;
	//printf("%s\n", str);
	c = *p++;

	if(c == '.') {
		c = *p++;
		if(isnum(c)) {
			while(isnum(c)) {
				i = i/10.0;
	   			f += (c - '0')* i;		
				c = *p++;
			}
			goto dec_exp;
		}
		goto error;
	} else if(*(p-1) == '0' && (*p == 'X' || *p == 'x')) {
		*p++;
		c = *p++;
		if(ishex(c)) {
			while(ishex(c)) {
				if(c <= '9') {
					tmp = c - '0';
				}else if(c <= 'F') {
					tmp = c - 'A' + 10;
				}else if(c <= 'f') {
					tmp = c - 'a' + 10;
				}

				f = f*16.0 + tmp;
				c = *p++;
			}

			if(c == '.') {
				c = *p++;
				if(ishex(c)) {
					while(ishex(c)) {
						if(c <= '9') {
							tmp = c - '0';
						}else if(c <= 'F') {
							tmp = c - 'A' + 10;
						}else if(c <= 'f') {
							tmp = c - 'a' + 10;
						}
						i = i/16.0;
	   					f += tmp * i;
						c = *p++;
					} 
					goto bin_exp;
				}
				goto bin_exp;
			} else 
				goto bin_exp;
		} else if(c == '.') {
			c = *p++;
			if(ishex(c)) {
				while(ishex(c)) {
					if(c <= '9') {
						tmp = c - '0';
					}else if(c <= 'F') {
						tmp = c - 'A' + 10;
					}else if(c <= 'f') {
						tmp = c - 'a' + 10;
					}

					i = i/16.0;
	   				f += tmp * i;
					c = *p++;
				}
				goto bin_exp;
			}
			goto error;
		} else {
			goto error;
		}
	} else if(isnum(c)) {
		while(isnum(c)) {
			f = f*10 + (c - '0');
			c = *p++;
		}
		if(c == '.') {
			c = *p++;
			if(isnum(c)) {
				while(isnum(c)) {
					i = i/10.0;
	   				f += (c - '0')* i;
					c = *p++;
				}
			} 
		} 
	}

dec_exp:
	if(c == 'e' || c == 'E') {
		c = *p++;
		if(c == '+') {	
			c = *p++;
		}
		else if(c == '-') {
			minus = T;
			c = *p++;
		}

		if(isnum(c)) {
			while(isnum(c)){
            	n = n * 10  + c - '0';
				c = *p++;
			}
		} else 
			goto error;
	}
	if(minus)
		n = -n;
	f = f * pow(10.0, n); 
	goto f_suffix;
bin_exp:
	if(c == 'p' || c == 'P') {
		c = *p++;
		if(c == '+') {	
			c = *p++;
		}
		else if(c == '-') {
			minus = T;
			c = *p++;
		}

		if(isnum(c)) {
			while(isnum(c)){
				n = n * 10  + c - '0';
				c = *p++;
			}
		} else 
			goto error;
	}
	if(minus)
		n = -n;
	f = ldexp(f, n);

	ret = DOUBLE_MASK;
f_suffix:
	if(c == 'l' || c == 'L') {
		ret |= LONG_MASK;
	} else if(c == 'f' || c == 'F') {

	} else if(c == '\0') {}
	else goto error;
	*out = f;
	return ret;
	
error:
	printf("invalid suffix '%s' on float constant", p);
	exit(0);
}

long escap_hex_char(char **p)
{
	char *pp = *p;
	char c;	
	long n = 0;
	int tmp;
	
	c = *pp++;
	
	if(c == '\'') {
		*p = pp;
		return '\'';
	}else if(c == '"') {
		*p = pp;
		return '\"';
	}else if(c == '?') {
		*p = pp;
		return '\?';
	}else if(c == '\\') {
		*p = pp;
		return '\\';
	}else if(c == 'a') {
		*p = pp;
		return '\a';
	}else if(c == 'b') {
		*p = pp;
		return '\b';
	}else if(c == 'f') {
		*p = pp;
		return '\f';
	}else if(c == 'n') {
		*p = pp;
		return '\n';
	}else if(c == 'r') {
		*p = pp;
		return '\r';
	}else if(c == 't') {
		*p = pp;
		return '\t';
	}else if(c == 'v') {
		*p = pp;
		return '\v';
	}else if(isoct(c)) {
		n = (n << 3) + c - '0';
		c = *pp++;
		if(isoct(c)) {
			n = (n << 3) + c - '0';
			c = *pp++;
			if(isoct(c)) {
				n = (n << 3) + c - '0';
				*p = pp;
				return n;
			}else {
				*p = --pp;
				return n;
			}
		} else {
			*p = --pp;
			return n;
		}
	}else if(c == 'x') {
		c = *pp++;
		while(ishex(c)) {
			if(c <= '9') {
				tmp = c - '0';
			}else if(c <= 'F') {
				tmp = c - 'A' + 10;
			}else if(c <= 'f') {
				tmp = c - 'a' + 10;
			}
			n = (n<<4) + tmp;
			c = *pp++;
		}
		*p = --pp;
		return n;
	}else goto error;

error:
	printf("error: parse error in char constant\n");
	exit(0);	
}
long str2char(char * s, bool *is_L)
{
	char c;
	char *p = s;
	long r = 0;

	*is_L = F;
	c = *p++;
	if(c == 'L') {
		*is_L = T;
		c = *p++;
	}
	
	if(c == '\'') {
		while(1) {
			c = *p++;
			if(c == '\\') {
				r = escap_hex_char(&p);
			} else if(c == '\'') 
				break;
			else if(c == '\0')
				goto error;
			else r = c;
		}
	} else goto error;

	return r;
error:
	printf("error: not char constant\n");
	exit(0);
}


long * str_literal(char * s, bool *is_L)
{
	char c;
	char *p = s;
	long *lptr;
	char *cptr;
	long r;
	int i = 0;

	*is_L = F;

	c = *p++;
	if(c == 'L') {
		*is_L = T;
		lptr = (long *)malloc(sizeof(long) * strlen(s));
		if (lptr == NULL) {
		   printf("error: Out of memory\n");
		   exit(-1);
		}

		c = *p++;
	}
	else {
		cptr = (char *)malloc(sizeof(char) * strlen(s));
		if (cptr == NULL) {
			printf("error: Out of memory\n");
			exit(-1);
		}
	}
	
	if(c == '"') {
		while(1) {
			c = *p++;
			if(c == '\\') {
				r = escap_hex_char(&p);
			} else if(c == '"') 
				break;
			else if(c == '\0')
				goto error;
			else r = c;

			if(*is_L == T) {
				lptr[i++] = r;
			}else
				cptr[i++] = (char)r;
		}
	} else goto error;

	if(*is_L) {
		lptr[i] = 0;
		return lptr;
	}
	else {
		cptr[i] = '\0';	
		return (long *)cptr;
	}
error:
	printf("error: not char constant\n");
	exit(0);
}


#if 0	// test finction
void print_float(float f)
{
	long *p = (long *)&f;
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
		printf("calculate from above: %f\n", ld);
		
}



int main() 
{
    float f; 
	char *pf;
	float dpf;

	int a;
	int pa;

#if 0		
		f = 100.e-1;
		print_float(f);
		pf = "100.e-1";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);
		
		f = 3.14159;
		print_float(f);
		pf = "3.14159";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);
		
		f = 1.0;
		print_float(f);
		pf = "1.0";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);
		
		f = 0xa4.44P0;
		print_float(f);	
		pf = "0xa4.44P0";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);
			
		f = 0x.4AP-1;
		print_float(f);	
		pf = "0x.4AP-1";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);
			
		f = 565.45454;
		print_float(f);			
		pf = "565.45454";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);

		f = 6666666666666666666666666693565.4545453;
		print_float(f);					
		pf = "6666666666666666666666666693565.4545453";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);

		f = 123426456562536156315652635162563546532645365463546354632565462354.4545453;
		print_float(f);		
		pf = "123426456562536156315652635162563546532645365463546354632565462354.4545453";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);	


		f = 123426456562536156315652635162563.4454545e-34535465757676;
		print_float(f); 	
		pf = "123426456562536156315652635162563.4454545e-34535465757676";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);

		f = 100e-1;
		print_float(f);
		pf = "100e-1";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);

		f = .100e-1;
		print_float(f);
		pf = ".100e-1";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);

		f = 100.;
		print_float(f);
		pf = "100";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);

		f = 0xa4P2;
		print_float(f);	
		pf = "0xa4P2";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);
			
		f = 0X56AP-1;
		print_float(f);	
		pf = "0X56AP-1";
		printf("str: %s\n", pf);
		dpf = str2float(pf);	
		print_float(dpf);	


		// now start to test dec
		
		a = 0x3858454578476885685;
		printf("%lld\n", a);	
		pf = "0x3858454578476885685";
		printf("str: %s\n", pf);
		pa = str2int(pf);	
		printf("%lld\n", pa);	

		a = 0;
		printf("%d\n", a);	
		pf = "0";
		printf("str: %s\n", pf);
		pa = str2int(pf);	
		printf("%d\n", pa);
		
		a = 46457563545657;
		printf("%lld\n", a);	
		pf = "46457563545657";
		printf("str: %s\n", pf);
		pa = str2int(pf);	
		printf("%lld\n", pa);


		a = 046457563745657;
		printf("%d\n", a);	
		pf = "046457563745657";
		printf("str: %s\n", pf);
		pa = str2int(pf);	
		printf("%d\n", pa);

		a = 0657;
		printf("%d\n", a);	
		pf = "0657";
		printf("str: %s\n", pf);
		pa = str2int(pf);	
		printf("%d\n", pa);

#endif
		// now test char
		char c;
		char str[100];
		char *str2;

		c = '\\';
		printf("%c = %d\n", c, c);
		str[0] = '\'';
		str[1] = '\\';
		str[2] = '\\';
		str[3] = '\'';
		str[4] = '\0';
		a = str2char(str);
		printf("%c = %d\n", a, a);

		c = '\"';
		printf("%c = %d\n", c, c);		
		str[0] = '\'';
		str[1] = '\\';
		str[2] = '"';
		str[3] = '\'';
		str[4] = '\0';
		a = str2char(str);
		printf("%c = %d\n", a, a);

		c = '\?';
		printf("%c = %d\n", c, c);
		str[0] = '\'';
		str[1] = '\\';
		str[2] = '?';
		str[3] = '\'';
		str[4] = '\0';
		a = str2char(str);
		printf("%c = %d\n", a, a);

		
		c = '\'';
		printf("%c = %d\n", c, c);
		str[0] = '\'';
		str[1] = '\\';
		str[2] = '\'';
		str[3] = '\'';
		str[4] = '\0';
		a = str2char(str);
		printf("%c = %d\n", a, a);		


		c = '\x45';
		printf("%c = %d\n", c, c);
		str2 = "\'\\x45\'";
		a = str2char(str2);
		printf("%c = %d\n", a, a);
		

		c = '\456546576';
		printf("%c = %d\n", c, c);
		str2 = "\'\\456546576\'";
		a = str2char(str2);
		printf("%c = %d\n", a, a);
		

		c = '\x3543657654645764576575687584564356457654634';
		printf("%c = %d\n", c, c);
		str2 = "\'\\x3543657654645764576575687584564356457654634\'";
		a = str2char(str2);
		c = (char)a;
		printf("%c = %d\n", c, c);	

		c = '\333';
		printf("%c = %d\n", c, c);
		str2 = "\'\\333\'";
		c = str2char(str2);
		printf("%c = %d\n", c, c);	

		c = '\35';
		printf("%c = %d\n", c, c);
		str2 = "\'\\35\'";
		c = str2char(str2);
		printf("%c = %d\n", c, c);

		c = '\6';
		printf("%c = %d\n", c, c);
		str2 = "\'\\6\'";
		c = str2char(str2);
		printf("%c = %d\n", c, c);		
}


#endif


