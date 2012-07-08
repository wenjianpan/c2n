
t1.t.o:     file format pe-i386


Disassembly of section .text:

00000000 <_func>:
   0:	55                   	push   %ebp
   1:	89 e5                	mov    %esp,%ebp
   3:	83 ec 08             	sub    $0x8,%esp
   
   6:	c7 45 fc 04 00 00 00 	movl   $0x4,-0x4(%ebp)
   d:	c6 45 fb 02          	movb   $0x2,-0x5(%ebp)
   
  11:	66 0f be 55 fb       	movsbw -0x5(%ebp),%dx
  16:	8b 45 fc             	mov    -0x4(%ebp),%eax
  19:	8d 04 02             	lea    (%edx,%eax,1),%eax
  1c:	66 89 45 f8          	mov    %ax,-0x8(%ebp)
  
  20:	8b 45 fc             	mov    -0x4(%ebp),%eax
  23:	66 0f be 55 fb       	movsbw -0x5(%ebp),%dx
  28:	29 d0                	sub    %edx,%eax
  2a:	66 89 45 f8          	mov    %ax,-0x8(%ebp)
  
  2e:	0f be 45 fb          	movsbl -0x5(%ebp),%eax
  32:	0f af 45 fc          	imul   -0x4(%ebp),%eax
  36:	66 89 45 f8          	mov    %ax,-0x8(%ebp)
  
  3a:	0f be 55 fb          	movsbl -0x5(%ebp),%edx
  3e:	8b 45 fc             	mov    -0x4(%ebp),%eax
  41:	89 d1                	mov    %edx,%ecx
  43:	99                   	cltd   
  44:	f7 f9                	idiv   %ecx
  46:	66 89 45 f8          	mov    %ax,-0x8(%ebp)
  
  4a:	0f be 55 fb          	movsbl -0x5(%ebp),%edx
  4e:	8b 45 fc             	mov    -0x4(%ebp),%eax
  51:	89 d1                	mov    %edx,%ecx
  53:	99                   	cltd   
  54:	f7 f9                	idiv   %ecx
  56:	66 89 55 f8          	mov    %dx,-0x8(%ebp)
  
  5a:	66 0f be 55 fb       	movsbw -0x5(%ebp),%dx
  5f:	8b 45 fc             	mov    -0x4(%ebp),%eax
  62:	21 d0                	and    %edx,%eax
  64:	66 89 45 f8          	mov    %ax,-0x8(%ebp)
  
  68:	66 0f be 55 fb       	movsbw -0x5(%ebp),%dx
  6d:	8b 45 fc             	mov    -0x4(%ebp),%eax
  70:	09 d0                	or     %edx,%eax
  72:	66 89 45 f8          	mov    %ax,-0x8(%ebp)
  
  76:	66 0f be 55 fb       	movsbw -0x5(%ebp),%dx
  7b:	8b 45 fc             	mov    -0x4(%ebp),%eax
  7e:	31 d0                	xor    %edx,%eax
  80:	66 89 45 f8          	mov    %ax,-0x8(%ebp)
  
  84:	0f be 4d fb          	movsbl -0x5(%ebp),%ecx
  88:	8b 45 fc             	mov    -0x4(%ebp),%eax
  8b:	d3 f8                	sar    %cl,%eax
  8d:	66 89 45 f8          	mov    %ax,-0x8(%ebp)
  
  91:	0f be 4d fb          	movsbl -0x5(%ebp),%ecx
  95:	8b 45 fc             	mov    -0x4(%ebp),%eax
  98:	d3 e0                	shl    %cl,%eax
  9a:	66 89 45 f8          	mov    %ax,-0x8(%ebp)
  
  9e:	c9                   	leave  
  9f:	c3                   	ret    


000000a0 <_func1>:
  a0:   55                      push   %ebp
  a1:   89 e5                   mov    %esp,%ebp
  a3:   83 ec 0c                sub    $0xc,%esp
  
  a6:   8b 45 f8                mov    -0x8(%ebp),%eax
  a9:   3b 45 f4                cmp    -0xc(%ebp),%eax
  ac:   0f 9f c0                setg   %al
  af:   0f b6 c0                movzbl %al,%eax
  b2:   89 45 fc                mov    %eax,-0x4(%ebp)
  
  b5:   8b 45 f8                mov    -0x8(%ebp),%eax
  b8:   3b 45 f4                cmp    -0xc(%ebp),%eax
  bb:   0f 9c c0                setl   %al
  be:   0f b6 c0                movzbl %al,%eax
  
  c1:   89 45 fc                mov    %eax,-0x4(%ebp)
  c4:   8b 45 f8                mov    -0x8(%ebp),%eax
  c7:   3b 45 f4                cmp    -0xc(%ebp),%eax
  ca:   0f 9d c0                setge  %al
  cd:   0f b6 c0                movzbl %al,%eax
  
  d0:   89 45 fc                mov    %eax,-0x4(%ebp)
  d3:   8b 45 f8                mov    -0x8(%ebp),%eax
  d6:   3b 45 f4                cmp    -0xc(%ebp),%eax
  d9:   0f 9e c0                setle  %al
  dc:   0f b6 c0                movzbl %al,%eax
  
  df:   89 45 fc                mov    %eax,-0x4(%ebp)
  e2:   8b 45 f8                mov    -0x8(%ebp),%eax
  e5:   3b 45 f4                cmp    -0xc(%ebp),%eax
  e8:   0f 94 c0                sete   %al
  eb:   0f b6 c0                movzbl %al,%eax
  
  ee:   89 45 fc                mov    %eax,-0x4(%ebp)
  f1:   8b 45 f8                mov    -0x8(%ebp),%eax
  f4:   3b 45 f4                cmp    -0xc(%ebp),%eax
  f7:   0f 95 c0                setne  %al
  fa:   0f b6 c0                movzbl %al,%eax
  
 100:   c9                      leave
 101:   c3                      ret
