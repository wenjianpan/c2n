

typedef struct {
	char c;
} sc;

typedef struct {
	int i;
} si;

typedef struct {
	short s;
} ss;

typedef struct {
	char c;
	short s;
} scs;

typedef struct {
	int i;
	short s;
	char c;
} sic;

typedef union {
  short s;
  char c;	
} usc;

typedef struct {
  int i;
  char c;	
  int i1;
  char c1;	 
} sicic;


void main()
{
	sc sc2;
	sc sc1;
	si si;
	ss ss;
	scs scs;
	sic sic;
	usc usc;
	sicic sicic;

  sc1 = 1;
}

