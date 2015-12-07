#pragma once

//(ARM处理器体系结构寄存器类型)
#define MAXBIT 32	//位图最大长度
//定义支持32个32位寄存器的位图
#define R0 0x1
#define R1 0x2
#define R2 0x4
#define R3 0x8
#define R4 0x10
#define R5 0x20
#define R6 0x40
#define R7 0x80
#define R8 0x100
#define R9 0x200
#define R10 0x400
#define R11 0x800
#define R12 0x1000
#define R13 0x2000
#define R14 0x4000
#define R15 0x8000
#define R16 0x10000
#define R17 0x20000
#define R18 0x40000
#define R19 0x80000
#define R20 0x100000
#define R21 0x200000
#define R22 0x400000
#define R23 0x800000
#define R24 0x1000000
#define R25 0x2000000
#define R26 0x4000000
#define R27 0x8000000
#define R28 0x10000000
#define R29 0x20000000
#define R30 0x40000000
#define R31 0x80000000
//可用寄存器名称
#define r1 R1
#define r2 R2
#define r3 R3
#define r4 R4
#define r5 R5
#define r6 R6
#define r7 R7
#define r8 R8
#define r9 R9
#define r10 R10
#define r11 R11
#define r12 R12
#define r13 R13
#define r14 R14
#define r15 R15
#define r16 R16
#define r17 R17
//寄存器别名
//参数传递
#define A1 R0
#define A2 R1
#define A3 R2
#define A4 R3

#define a1 A1
#define a2 A2
#define a3 A3
#define a4 A4
//局部变量
#define V1 R4
#define V2 R5
#define V3 R6
#define V4 R7
#define V5 R8
#define V6 R9
#define V7 R10
#define V8 R11

#define v1 V1
#define v2 V2
#define v3 V3
#define v4 V4
#define v5 V5
#define v6 V6
#define v7 V7
#define v8 V8
//特殊寄存器
#define SB R9	//静态基址寄存器
#define sb SB
#define SL R10	//堆栈限制寄存器
#define sl SL
#define FP R11	//帧指针
#define fp FP

#define IP R12	//过程调用中间临时寄存器
#define ip IP
#define SP R13	//堆栈指针
#define sp SP
#define LR R14	//链接寄存器，保存返回地址
#define lr LR
#define PC R15	//程序计数器
#define pc PC
#define CPSR R16	//当前程序状态寄存器
#define cpsr CPSR
#define SPSR R17	//保存的程序状态寄存器
#define spsr SPSR

#define R_LEGAL 0xfffc0000	//合法的可以使用的寄存器，R0-R17
#define R_USABLE R_LEGAL|SPSR|CPSR|PC|LR|SP	//默认可以分配的寄存器，R0-R12

