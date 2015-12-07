#pragma once
#include "common.h"

/*
	错误类
*/
class Error
{	
	
	static Scanner *scanner;//当前使用的扫描器
	
public:
	
	//构造与初始化
	Error(Scanner*sc);
	
	static int errorNum;//错误个数
	static int warnNum;	//警告个数
	//外界接口
	static int getErrorNum();
	static int getWarnNum();
	
	//错误接口
	static void lexError(int code);//打印词法错误
	static void synError(int code,Token*t);//打印语法错误
	static void semError(int code,string name="");//打印语义错误
	static void semWarn(int code,string name="");//打印语义警告
	
};

//错误级别,可选，用于修饰错误信息头部
#define FATAL "<fatal>:"
#define ERROR "<error>:"
#define WARN	"<warn>:"

//调试输出
#ifdef DEBUG
#define PDEBUG(fmt, args...) printf(fmt, ##args)
#else
#define PDEBUG(fmt, args...)
#endif	//DEBUG




void SEMWARN(int code);//打印语义警告




