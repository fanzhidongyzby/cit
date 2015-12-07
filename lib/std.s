#只读数据段，存储格式化字符串
.section	.rodata
.int_format:
	.ascii	"%d\000"
.char_format:
	.ascii	"%c\000"
.str_format:
	.ascii	"%s\000"
#库代码
.text
#函数print_int代码
.global print_int
print_int:
	# 函数入口
	mov ip,sp
	stmfd sp!,{r0-r7,fp,ip,lr,pc}
	sub fp,ip,#4
	# 开辟栈帧
	ldr	r0, =.int_format
	ldr	r1, [fp,#4]
	bl	printf
	# 函数出口
	ldmea fp,{r0-r7,fp,sp,pc}
#函数print_char代码
.global print_char
print_char:
	# 函数入口
	mov ip,sp
	stmfd sp!,{r0-r7,fp,ip,lr,pc}
	sub fp,ip,#4
	# 开辟栈帧
	ldr	r0, =.char_format
	ldrb	r1, [fp,#4]
	bl	printf
	# 函数出口
	ldmea fp,{r0-r7,fp,sp,pc}
#函数print_str代码
.global print_str
print_str:
	# 函数入口
	mov ip,sp
	stmfd sp!,{r0-r7,fp,ip,lr,pc}
	sub fp,ip,#4
	# 开辟栈帧
	ldr	r0, =.str_format
	ldr	r1, [fp,#4]
	bl	printf
	# 函数出口
	ldmea fp,{r0-r7,fp,sp,pc}
