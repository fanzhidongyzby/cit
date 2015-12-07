#pragma once
#include"common.h"
/*
	ARM平台信息
*/
class Plat
{
	static void roundLeftShiftTwoBit(unsigned int& num);//循环左移两位
	static bool __constExpr(unsigned int num);//判断num是否是常数表达式，8位数字循环右移偶数位得到
public:
	static bool constExpr(int num);//同时处理正数和负数
	static bool isDisp(int num);//判定是否是合法的偏移
	static bool isReg(string s);//判断是否是合法的寄存器名
	/*
		进栈指令序列:
			mov ip,sp
			stmfd sp!,{fp,ip,lr,pc}
			sub fp,ip,#4
		出栈指令序列：
			ldmea fp,{fp,sp,pc}
		栈基址fp指向pc，继续入栈需要在偏移12字节基础之上！
	*/
	static const int regNum=8;//可使用的通用寄存器的个数r0-r7
	static const char* regName[regNum];//可以使用的通用寄存器的个数
	static const int stackBase=12;//不加保护现场的栈基址=12
	static const int stackBase_protect=12+regNum*4;//加保护现场的栈基址=12+sizeof({r0-r7})=44
};

