
int func()
{
   char a[10];
   
   a[0] = 1;
   if(a[0]) {
      char b[10];
      a[1] = b[0] = 0xab;	
   }	
}

