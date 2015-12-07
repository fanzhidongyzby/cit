#include "platform.h"

const char* Plat::regName[regNum]={
		"r0","r1","r2","r3","r4","r5","r6","r7"
		//,"r8"//用于加载操作数1,保存表达式结果
		//,"r9"//用于加载操作数2,写回表达式结果,立即数，标签地址
		//,"r10"//用于保存乘法结果，虽然mul r8,r8,r9也能正常执行，但是避免交叉编译提示错误！
		//,"fp"//r11,局部变量寻址
		//,"ip"//"r12"，临时寄存器
		//,"sp"//r13，栈指针
		//,"lr"//r14，链接寄存器
		//,"pc"//r15，程序计数器
	};

/*
	循环左移两位
*/
void Plat::roundLeftShiftTwoBit(unsigned int& num)
{
	unsigned int overFlow=num & 0xc0000000;//取左移即将溢出的两位
	num=(num<<2) | (overFlow>>30);//将溢出部分追加到尾部
}

/*
	判断num是否是常数表达式，8位数字循环右移偶数位得到
*/
bool Plat::__constExpr(unsigned int num)
{
	for(int i=0;i<16;i++){
		if(num<=0xff)return true;//有效位图
		else roundLeftShiftTwoBit(num);//循环左移2位
	}
}

/*
	同时处理正数和负数
*/
bool Plat::constExpr(int num)
{
	return __constExpr(num)||__constExpr(-num);
}

/*
	判定是否是合法的偏移(-4096,4096)
*/
bool Plat::isDisp(int num)
{
	return num<4096 && num >-4096;
}

/*
	判断是否是合法的寄存器名
*/
bool Plat::isReg(string s)
{
	return s=="r0" || s=="r1" || s=="r2" || s=="r3" || s=="r4"
			|| s=="r5" || s=="r6" || s=="r7" || s=="r8" || s=="r9"
			|| s=="r10" || s=="fp" || s=="ip" || s=="sp" || s=="lr"
			|| s=="pc";
}