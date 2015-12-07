#pragma once
#include "common.h"

/*
	编译器参数类
*/
class Args
{
public:
	static bool showChar;//显示字符
	static bool showToken;//显示词法记号
	static bool showSym;//显示符号表
	static bool showIr;//显示中间代码
	static bool showOr;//显示优化后的中间代码
	static bool showBlock;//显示基本块和流图关系
	static bool showHelp;//显示帮助
	static bool opt;//是否执行优化
};

/*
	编译器类
*/
class Compiler
{

public:
	
	//核心函数
	void compile(char*file);//编译一个文件
};

