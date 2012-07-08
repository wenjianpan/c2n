typedef struct {
	int a;
	char b;
	char c[64];
} s;


void func(s ss, char c)
{
	ss.a = 10;
}

int func2(int i, char c)
{
	i = c;
	return i;
}

void func3()
{	
	s ss;
	s s1;
	
	s1 = ss;
  func(ss, 'c');

}

int main()
{
	func3();
	printf("haha\n");
	
	return 0;
}
