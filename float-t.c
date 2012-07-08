void func()
{
  float f;
  double df;
  int i;
  
  f = 0.001;
  df = 0.002343;
  
  f++;
  f--;
  i = f==df;
	df = f*df;
	//f >>1;  // error
	//df<<1;  // error
	
	//i = df%i; // error
	i^2;
	//f^2;  // error
	//df^2; // error
	//df|i; //error
	//~df; //error
	!df;
	//df&i; //error
	i = 0x11111111;
	
	f = f+f;
	f = f-f;
	f = f*f;
	f = f/f;

	i = 0x22222222;	
	df = df+df;
	df = df-df;
	df = df*df;
	df = df/df;	
	
	i = 0x33333333;	
	df = df+f;
	df = df-f;
	df = df*f;
	df = df/f;	
}


