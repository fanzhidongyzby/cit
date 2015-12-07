#pragma once
#include"common.h"
#include "set.h"

/*
	底层汇编指令：ARM形式
*/
struct Arm
{
	bool dead;//标识代码是否无效
	string opcode;//操作码
	string result;//结果
	string arg1;//操作数1
	string arg2;//操作数2
	string addition;//附加信息
	Arm(string op,string rs="",string a1="",string a2="",string add="");
	void replace(string op,string rs="",string a1="",string a2="",string add="");
	void setDead();//设置无效
	string outPut();//输出函数
};

/*
	底层汇编序列
*/
class ILoc
{
	list<Arm*> code;//代码序列
	/*
		整数的字符串表达
	*/
	string toStr(int num,int flag=true);//#123
	/*
		ldr str
	*/
	void ldr_imm(string rsReg,int num);//加载立即数 ldr r0,=#100
	void ldr_lb(string rsReg,string name);//加载符号值 ldr r0,=g ldr r0,[r0]
	void leaStack(string rsReg,int off);//加载栈内变量地址
public:
	//基址寻址 ldr r0,[fp,#100]
	void ldr_base(string rsReg,string base,int disp,bool isChar);
	//基址寻址 str r0,[fp,#100]
	void str_base(string srcReg,string base,int disp,string tmpReg,bool isChar);
	/*
		label 注释 一般指令
	*/
	void label(string name);//产生标签
	void comment(string str);//产生注释
	void inst(string op,string rs);//0个操作数
	void inst(string op,string rs,string arg1);//一个操作数
	void inst(string op,string rs,string arg1,string arg2);//两个操作数
	/*
		变量操作
	*/
	void init_var(Var*var,string initReg,string tmpReg);//变量的初始化
	void ldr_var(string rsReg,Var*var);//加载变量到寄存器
	void lea_var(string rsReg,Var*var);//加载变量地址到寄存器
	void str_var(string srcReg,Var*var,string addrReg);//保存寄存器到变量
	/*
		函数调用
	*/
	void call_lib(string fun,string rsReg,string reg1,string reg2);//调用库函数
	void call_fun(Fun*fun,string tmpReg);//调用函数fun
	/*
		栈处理
	*/
	void allocStack(Fun*fun,string tmpReg);//分配栈帧
	void ldr_args(Fun*fun);//加载函数的参数到寄存器
	/*
		逻辑运算
	*/
	void logic_and(string rsReg,string reg1,string reg2);//逻辑与
	void logic_or(string rsReg,string reg1,string reg2);//逻辑或
	void logic_not(string rsReg,string reg1);//逻辑非
	void cmp(string cmp,string cmpnot,string rsReg,string reg1,string reg2);//比较
	/*
		占位指令，避免跨语句的优化，以防变量无法正确赋值
	*/
	void nop();//占位
	/*
		功能函数
	*/
	void outPut(FILE*file);//输出汇编
	list<Arm*>& getCode();//获取代码
	~ILoc();
};



