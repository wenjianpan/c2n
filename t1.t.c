

int ff()
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

int ff2()
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

