#ifndef _COMPILER_H_
#define _COMPILER_H_
#include<iostream>
using namespace std;
/**
 * 编译器封装类
 */
class Compiler
{
public:
	Compiler(bool l,bool sy,bool se,bool g,bool t);
	void genCommonFile();//产生必需共用的目标文件
	void compile(char* name);//编译
};
#endif
