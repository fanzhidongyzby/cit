.section .rodata
.data
#优化代码
.text
#函数fun代码
	.global fun
fun:
	# 函数入口
	mov ip,sp
	stmfd sp!,{r0-r7,fp,ip,lr,pc}
	sub fp,ip,#4
	# 开辟栈帧
	# 加载参数变量到寄存器
	# 函数内代码
	mov r0,#10
	.L6 :
	.L10 :
	.L11 :
	mov r1,r0
	sub r0,r0,#1
	cmp r1,#0
	bne .L11
	.L12 :
	mov r8,#28
	.L1 :
	# 函数出口
	ldmea fp,{r0-r7,fp,sp,pc}
#函数main代码
	.global main
main:
	# 函数入口
	mov ip,sp
	stmfd sp!,{r0-r7,fp,ip,lr,pc}
	sub fp,ip,#4
	# 开辟栈帧
	# 加载参数变量到寄存器
	# 函数内代码
	mov r0,#500
	.L19 :
	mov r2,r0
	sub r0,r0,#1
	cmp r2,#0
	beq .L20
	bl fun
	mov r1,r8
	b .L19
	.L20 :
	mov r8,r1
	.L18 :
	# 函数出口
	ldmea fp,{r0-r7,fp,sp,pc}
