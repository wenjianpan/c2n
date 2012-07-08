typedef struct {
   int a;
   int b;
} s;

int callee(s ss, int b, char c)
{
   ss.a = 0xff;
   ss.b = 0xdeadbeaf;
   b = 0xaa;
   c = 'c';
   return 0;	
}

int caller()
{
   int ret;
   s ss;
   
   ret = func(ss, 0xffaaffaa, 'c');
   return ret;
}

temp.o:     file format elf32-i386

Disassembly of section .text:

00000000 <callee>:
   0:	55                   	push   %ebp
   1:	89 e5                	mov    %esp,%ebp
   3:	81 ec 00 00 00 00    	sub    $0x0,%esp
   9:	90                   	nop    
   a:	b8 ff 00 00 00       	mov    $0xff,%eax
   f:	89 45 08             	mov    %eax,0x8(%ebp)
  12:	b8 af be ad de       	mov    $0xdeadbeaf,%eax
  17:	89 45 0c             	mov    %eax,0xc(%ebp)
  1a:	b8 aa 00 00 00       	mov    $0xaa,%eax
  1f:	89 45 10             	mov    %eax,0x10(%ebp)
  22:	b8 63 00 00 00       	mov    $0x63,%eax
  27:	88 45 14             	mov    %al,0x14(%ebp)
  2a:	b8 00 00 00 00       	mov    $0x0,%eax
  2f:	e9 00 00 00 00       	jmp    34 <callee+0x34>
  34:	c9                   	leave  
  35:	c3                   	ret    

00000036 <caller>:
  36:	55                   	push   %ebp
  37:	89 e5                	mov    %esp,%ebp
  39:	81 ec 10 00 00 00    	sub    $0x10,%esp
  3f:	90                   	nop    
  40:	b8 63 00 00 00       	mov    $0x63,%eax
  45:	50                   	push   %eax
  46:	b8 aa ff aa ff       	mov    $0xffaaffaa,%eax
  4b:	50                   	push   %eax
  4c:	81 ec 08 00 00 00    	sub    $0x8,%esp
  52:	89 e0                	mov    %esp,%eax
  54:	b9 08 00 00 00       	mov    $0x8,%ecx
  59:	51                   	push   %ecx
  5a:	8d 4d f4             	lea    -0xc(%ebp),%ecx
  5d:	51                   	push   %ecx
  5e:	50                   	push   %eax
  5f:	89 45 f0             	mov    %eax,-0x10(%ebp)
  62:	e8 fc ff ff ff       	call   63 <caller+0x2d>
  67:	83 c4 0c             	add    $0xc,%esp
  6a:	e8 fc ff ff ff       	call   6b <caller+0x35>
  6f:	83 c4 10             	add    $0x10,%esp
  72:	89 45 fc             	mov    %eax,-0x4(%ebp)
  75:	8b 45 fc             	mov    -0x4(%ebp),%eax
  78:	e9 00 00 00 00       	jmp    7d <caller+0x47>
  7d:	c9                   	leave  
  7e:	c3                   	ret    
