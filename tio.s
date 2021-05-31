	.file	"tio.c"
	.text
	.align 16
	.globl	inc_cursor
	.type	inc_cursor, @function
inc_cursor:
	movl	term_col, %eax
	movl	term_row, %edx
	addl	$1, %eax
	movl	%eax, term_col
	cmpl	$79, %eax
	jbe	.L2
	movl	$0, term_col
	addl	$1, %edx
	movl	%edx, term_row
.L2:
	cmpl	$25, %edx
	ja	.L16
	ret
.L16:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	pushl	%ebx
	movl	video_buff, %ebx
	leal	320(%ebx), %ecx
	leal	4160(%ebx), %esi
	.align 16
.L5:
	leal	-160(%ecx), %eax
	.align 16
.L4:
	movzwl	(%eax), %edx
	addl	$2, %eax
	movw	%dx, -162(%eax)
	cmpl	%ecx, %eax
	jne	.L4
	leal	160(%eax), %ecx
	cmpl	%esi, %ecx
	jne	.L5
	leal	3840(%ebx), %eax
	leal	4000(%ebx), %edx
	.align 16
.L6:
	movl	$32, %ecx
	addl	$2, %eax
	movw	%cx, -2(%eax)
	cmpl	%eax, %edx
	jne	.L6
	popl	%ebx
	popl	%esi
	popl	%ebp
	ret
	.size	inc_cursor, .-inc_cursor
	.align 16
	.globl	term_write_char_color
	.type	term_write_char_color, @function
term_write_char_color:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	movl	8(%ebp), %eax
	pushl	%esi
	pushl	%ebx
	cmpb	$10, %al
	je	.L37
	cmpb	$13, %al
	je	.L38
	movl	term_row, %esi
	movl	12(%ebp), %edi
	movzbl	%al, %eax
	movl	term_col, %edx
	movl	video_buff, %ebx
	leal	(%esi,%esi,4), %ecx
	sall	$8, %edi
	sall	$4, %ecx
	orl	%edi, %eax
	addl	%edx, %ecx
	addl	$1, %edx
	movw	%ax, (%ebx,%ecx,2)
	movl	%edx, term_col
	cmpl	$79, %edx
	jbe	.L26
	movl	$0, term_col
	addl	$1, %esi
	movl	%esi, term_row
.L26:
	cmpl	$25, %esi
	ja	.L39
.L17:
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
.L38:
	movl	$0, term_col
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
.L37:
	movl	term_row, %eax
	movl	$0, term_col
	addl	$1, %eax
	movl	%eax, term_row
	cmpl	$25, %eax
	jbe	.L17
	movl	video_buff, %ebx
	leal	320(%ebx), %ecx
	leal	4160(%ebx), %esi
.L22:
	leal	-160(%ecx), %eax
	.align 16
.L21:
	movzwl	(%eax), %edx
	addl	$2, %eax
	movw	%dx, -162(%eax)
	cmpl	%eax, %ecx
	jne	.L21
	addl	$160, %ecx
	cmpl	%ecx, %esi
	jne	.L22
	leal	3840(%ebx), %eax
	leal	4000(%ebx), %edx
	.align 16
.L23:
	movl	$32, %ebx
	addl	$2, %eax
	movw	%bx, -2(%eax)
	cmpl	%eax, %edx
	jne	.L23
	jmp	.L17
.L39:
	leal	320(%ebx), %ecx
	leal	4160(%ebx), %esi
	.align 16
.L28:
	leal	-160(%ecx), %eax
	.align 16
.L27:
	movzwl	(%eax), %edx
	addl	$2, %eax
	movw	%dx, -162(%eax)
	cmpl	%ecx, %eax
	jne	.L27
	leal	160(%eax), %ecx
	cmpl	%esi, %ecx
	jne	.L28
	leal	3840(%ebx), %eax
	leal	4000(%ebx), %edx
	.align 16
.L29:
	movl	$32, %ecx
	addl	$2, %eax
	movw	%cx, -2(%eax)
	cmpl	%edx, %eax
	jne	.L29
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.size	term_write_char_color, .-term_write_char_color
	.align 16
	.globl	term_write_char
	.type	term_write_char, @function
term_write_char:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$16, %esp
	movsbl	8(%ebp), %eax
	pushl	$15
	pushl	%eax
	call	term_write_char_color
	addl	$16, %esp
	leave
	ret
	.size	term_write_char, .-term_write_char
	.align 16
	.globl	term_write
	.type	term_write, @function
term_write:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%ebx
	subl	$4, %esp
	movl	8(%ebp), %ebx
	movsbl	(%ebx), %eax
	testb	%al, %al
	je	.L42
	.align 16
.L44:
	subl	$8, %esp
	addl	$1, %ebx
	pushl	$15
	pushl	%eax
	call	term_write_char_color
	movsbl	(%ebx), %eax
	addl	$16, %esp
	testb	%al, %al
	jne	.L44
.L42:
	movl	-4(%ebp), %ebx
	leave
	ret
	.size	term_write, .-term_write
	.align 16
	.globl	term_write_color
	.type	term_write_color, @function
term_write_color:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%esi
	movl	8(%ebp), %esi
	pushl	%ebx
	movl	12(%ebp), %ebx
	movsbl	(%esi), %eax
	testb	%al, %al
	je	.L50
	.align 16
.L52:
	subl	$8, %esp
	addl	$1, %esi
	pushl	%ebx
	pushl	%eax
	call	term_write_char_color
	movsbl	(%esi), %eax
	addl	$16, %esp
	testb	%al, %al
	jne	.L52
.L50:
	leal	-8(%ebp), %esp
	popl	%ebx
	popl	%esi
	popl	%ebp
	ret
	.size	term_write_color, .-term_write_color
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"0x"
.LC1:
	.string	"0b"
	.text
	.align 16
	.globl	term_write_int32
	.type	term_write_int32, @function
term_write_int32:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$44, %esp
	movl	12(%ebp), %ebx
	movl	8(%ebp), %esi
	cmpl	$16, %ebx
	je	.L92
	cmpl	$2, %ebx
	je	.L93
	cmpl	$8, %ebx
	je	.L65
.L61:
	testl	%esi, %esi
	jle	.L66
.L96:
	movl	%esi, %eax
	xorl	%ecx, %ecx
	.align 16
.L67:
	movl	%eax, %edi
	xorl	%edx, %edx
	addl	$1, %ecx
	divl	%ebx
	cmpl	%ebx, %edi
	jnb	.L67
	movzbl	%cl, %ecx
	leal	-1(%ecx), %eax
	movl	%eax, -48(%ebp)
	leal	15(%ecx), %eax
	andl	$496, %eax
	subl	%eax, %esp
	xorl	%eax, %eax
	movl	%esp, -44(%ebp)
	movl	-44(%ebp), %edx
	testl	%ecx, %ecx
	je	.L77
	.align 16
.L69:
	movb	$48, (%edx,%eax)
	addl	$1, %eax
	cmpl	%eax, %ecx
	jg	.L69
.L77:
	movl	-44(%ebp), %edi
	jmp	.L72
	.align 16
.L95:
	addl	$48, %edx
	addl	$1, %edi
	movb	%dl, -1(%edi)
	cmpl	%ebx, %ecx
	jb	.L94
.L72:
	movl	%esi, %eax
	xorl	%edx, %edx
	movl	%esi, %ecx
	divl	%ebx
	movl	%eax, %esi
	cmpl	$10, %edx
	jbe	.L95
	leal	-10(%ecx), %eax
	xorl	%edx, %edx
	addl	$1, %edi
	divl	%ebx
	addl	$97, %edx
	movb	%dl, -1(%edi)
	cmpl	%ebx, %ecx
	jnb	.L72
.L94:
	cmpl	$-1, -48(%ebp)
	je	.L58
.L74:
	movl	-48(%ebp), %ebx
	movl	-44(%ebp), %esi
	addl	%esi, %ebx
	.align 16
.L75:
	subl	$8, %esp
	pushl	$15
	movsbl	(%ebx), %eax
	pushl	%eax
	call	term_write_char_color
	movl	%ebx, %eax
	addl	$16, %esp
	subl	$1, %ebx
	cmpl	%eax, %esi
	jne	.L75
.L58:
	leal	-12(%ebp), %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.align 16
.L92:
	movl	$.LC0, %edi
	movl	$48, %eax
	.align 16
.L60:
	subl	$8, %esp
	addl	$1, %edi
	pushl	$15
	pushl	%eax
	call	term_write_char_color
	movsbl	(%edi), %eax
	addl	$16, %esp
	testb	%al, %al
	jne	.L60
	testl	%esi, %esi
	jg	.L96
	.align 16
.L66:
	je	.L58
	leal	-25(%ebp), %eax
	movb	$45, -25(%ebp)
	movl	$0, -48(%ebp)
	movl	%eax, -44(%ebp)
	jmp	.L74
	.align 16
.L93:
	movl	$.LC1, %edi
	movl	$48, %eax
	.align 16
.L63:
	subl	$8, %esp
	addl	$1, %edi
	pushl	$15
	pushl	%eax
	call	term_write_char_color
	movsbl	(%edi), %eax
	addl	$16, %esp
	testb	%al, %al
	jne	.L63
	jmp	.L61
	.align 16
.L65:
	subl	$8, %esp
	pushl	$15
	pushl	$48
	call	term_write_char_color
	addl	$16, %esp
	jmp	.L61
	.size	term_write_int32, .-term_write_int32
	.align 16
	.globl	term_write_int
	.type	term_write_int, @function
term_write_int:
	jmp	term_write_int32
	.size	term_write_int, .-term_write_int
	.align 16
	.globl	term_write_uint32
	.type	term_write_uint32, @function
term_write_uint32:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$44, %esp
	movl	12(%ebp), %ebx
	movl	8(%ebp), %esi
	cmpl	$16, %ebx
	je	.L139
	cmpl	$2, %ebx
	je	.L140
	cmpl	$8, %ebx
	je	.L141
.L104:
	movl	%esi, %eax
	testl	%esi, %esi
	jle	.L142
.L120:
	movl	$1, %ecx
	.align 16
.L107:
	movl	%eax, %edi
	xorl	%edx, %edx
	addl	$1, %ecx
	divl	%ebx
	cmpl	%ebx, %edi
	jnb	.L107
	cmpl	$16, %ebx
	je	.L122
	movzbl	%cl, %ecx
	leal	-1(%ecx), %eax
	movl	%eax, -44(%ebp)
	leal	15(%ecx), %eax
	andl	$496, %eax
	subl	%eax, %esp
	movl	%esp, %edi
	testl	%ecx, %ecx
	je	.L110
.L109:
	xorl	%eax, %eax
	.align 16
.L111:
	movb	$48, (%edi,%eax)
	addl	$1, %eax
	cmpl	%ecx, %eax
	jl	.L111
.L110:
	testl	%esi, %esi
	je	.L118
.L112:
	movl	%edi, -48(%ebp)
	movl	%edi, %ecx
	jmp	.L117
	.align 16
.L144:
	addl	$48, %edx
	addl	$1, %ecx
	movb	%dl, -1(%ecx)
	cmpl	%esi, %ebx
	ja	.L143
.L124:
	movl	%edi, %esi
.L117:
	movl	%esi, %eax
	xorl	%edx, %edx
	divl	%ebx
	movl	%eax, %edi
	cmpl	$9, %edx
	jbe	.L144
	leal	-10(%esi), %eax
	xorl	%edx, %edx
	addl	$1, %ecx
	divl	%ebx
	addl	$97, %edx
	movb	%dl, -1(%ecx)
	cmpl	%esi, %ebx
	jbe	.L124
.L143:
	movl	-48(%ebp), %edi
.L118:
	cmpl	$-1, -44(%ebp)
	je	.L98
.L113:
	movl	-44(%ebp), %ebx
	addl	%edi, %ebx
	.align 16
.L119:
	subl	$8, %esp
	pushl	$15
	movsbl	(%ebx), %eax
	pushl	%eax
	call	term_write_char_color
	movl	%ebx, %eax
	addl	$16, %esp
	subl	$1, %ebx
	cmpl	%eax, %edi
	jne	.L119
.L98:
	leal	-12(%ebp), %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.align 16
.L122:
	movl	$7, -44(%ebp)
	leal	-32(%ebp), %edi
	movl	$8, %ecx
	jmp	.L109
	.align 16
.L139:
	movl	$.LC0, %edi
	movl	$48, %eax
	.align 16
.L100:
	subl	$8, %esp
	addl	$1, %edi
	pushl	$15
	pushl	%eax
	call	term_write_char_color
	movsbl	(%edi), %eax
	addl	$16, %esp
	testb	%al, %al
	jne	.L100
	movl	%esi, %eax
	testl	%esi, %esi
	jg	.L120
.L106:
	cmpl	$16, %ebx
	je	.L122
	subl	$16, %esp
	movl	$0, -44(%ebp)
	movl	$1, %ecx
	movl	%esp, %edi
	jmp	.L109
	.align 16
.L140:
	movl	$.LC1, %edi
	movl	$48, %eax
	.align 16
.L103:
	subl	$8, %esp
	addl	$1, %edi
	pushl	$15
	pushl	%eax
	call	term_write_char_color
	movsbl	(%edi), %eax
	addl	$16, %esp
	testb	%al, %al
	jne	.L103
	jmp	.L104
	.align 16
.L141:
	subl	$8, %esp
	pushl	$15
	pushl	$48
	call	term_write_char_color
	addl	$16, %esp
	movl	%esi, %eax
	testl	%esi, %esi
	jg	.L120
	jmp	.L106
.L142:
	subl	$16, %esp
	movl	$0, -44(%ebp)
	movb	$48, (%esp)
	movl	%esp, %edi
	testl	%esi, %esi
	jne	.L112
	jmp	.L113
	.size	term_write_uint32, .-term_write_uint32
	.align 16
	.globl	shift_term_line_up
	.type	shift_term_line_up, @function
shift_term_line_up:
	pushl	%ebp
	movl	$25, %edx
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	movl	$25, %ebx
	subl	$16, %esp
	movl	video_buff, %edi
	subw	8(%ebp), %bx
	leal	160(%edi), %ecx
	movl	%ecx, -24(%ebp)
	imull	$-80, 8(%ebp), %ecx
	subl	8(%ebp), %edx
	movl	%edx, -20(%ebp)
	je	.L158
	movw	%bx, -26(%ebp)
	movl	-24(%ebp), %ebx
	xorl	%esi, %esi
	xorl	%eax, %eax
	.align 16
.L146:
	addl	8(%ebp), %eax
	movl	%ebx, -16(%ebp)
	leal	(%eax,%eax,4), %edx
	sall	$5, %edx
	leal	(%edi,%edx), %eax
	addl	%ebx, %edx
	.align 16
.L148:
	movzwl	(%eax), %ebx
	movw	%bx, (%eax,%ecx,2)
	addl	$2, %eax
	cmpl	%eax, %edx
	jne	.L148
	addl	$1, %esi
	movl	-16(%ebp), %ebx
	movzwl	%si, %eax
	cmpl	-20(%ebp), %eax
	jb	.L146
	movzwl	-26(%ebp), %ebx
	cmpw	$24, %bx
	ja	.L145
.L158:
	movl	-24(%ebp), %ecx
	.align 16
.L147:
	movzwl	%bx, %eax
	leal	(%eax,%eax,4), %edx
	sall	$5, %edx
	leal	(%edi,%edx), %eax
	addl	%ecx, %edx
	.align 16
.L150:
	movl	$32, %esi
	addl	$2, %eax
	movw	%si, -2(%eax)
	cmpl	%eax, %edx
	jne	.L150
	addl	$1, %ebx
	cmpw	$25, %bx
	jne	.L147
.L145:
	addl	$16, %esp
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.size	shift_term_line_up, .-shift_term_line_up
	.globl	term_col
	.section	.bss
	.align 4
	.type	term_col, @object
	.size	term_col, 4
term_col:
	.zero	4
	.globl	term_row
	.align 4
	.type	term_row, @object
	.size	term_row, 4
term_row:
	.zero	4
	.globl	video_buff
	.data
	.align 4
	.type	video_buff, @object
	.size	video_buff, 4
video_buff:
	.long	753664
	.ident	"GCC: (GNU) 11.1.0"
