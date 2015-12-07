#pragma once

#include <stdio.h>
#include <vector>
#include <list>
#include <string>
using namespace std;


/*******************************************************************************
                                   词法记号的标签
*******************************************************************************/

/*
	词法记号标签
*/
enum Tag
{
	ERR,																					//错误，异常
	END,																					//文件结束标记
	ID,																						//标识符
	KW_INT,KW_CHAR,KW_VOID,												//数据类型
	KW_EXTERN,																		//extern
	NUM,CH,STR,																		//字面量
	NOT,LEA,																			//单目运算 ! - & *
	ADD,SUB,MUL,DIV,MOD,													//算术运算符
	INC,DEC,
	GT,GE,LT,LE,EQU,NEQU,													//比较运算符
	AND,OR,																				//逻辑运算
	LPAREN,RPAREN,																//()
	LBRACK,RBRACK,																//[]
	LBRACE,RBRACE,																//{}
	COMMA,COLON,SEMICON,													//逗号,冒号,分号
	ASSIGN,																				//赋值
	KW_IF,KW_ELSE,																//if-else
	KW_SWITCH,KW_CASE,KW_DEFAULT,									//swicth-case-deault
	KW_WHILE,KW_DO,KW_FOR,												//循环
	KW_BREAK,KW_CONTINUE,KW_RETURN								//break,continue,return
};

/*
	中间代码操作符枚举类型
*/
enum Operator
{
	//占位指令,默认值
	OP_NOP,
	//声明指令
	OP_DEC, //eg: DEC arg1 => (int/char */[]) arg1
	//函数入口和出口
	OP_ENTRY,OP_EXIT,
	//赋值运算
	OP_AS,//赋值 eg: AS result,arg1 => result=arg1
	//算数运算
	OP_ADD,OP_SUB,OP_MUL,OP_DIV,OP_MOD,//加减乘除模 eg: ADD result,arg1,arg2 => result=arg1 + arg2
	OP_NEG,//负 eg: NEG result,arg1 => result = -arg1
	//比较运算
	OP_GT,OP_GE,OP_LT,OP_LE,OP_EQU,OP_NE,//大小等 eg: GT result,arg1,arg2 => result=arg1 > arg2
	//逻辑运算
	OP_NOT,//非 eg: NOT result,arg1 => result=!arg1
	OP_AND,OP_OR,//与或 eg: AND result,arg1,arg2 => result=arg1 && arg2
	//数组运算
	//OP_INDL,//索引作为左值 eg: INDL result,arg1,arg2 => arg1[arg2]=result
	//OP_INDR,//索引作为右值 eg: INDR result,arg1,arg2 => result=arg1[arg2]
	//指针运算
	OP_LEA,//取址 eg: LEA result,arg1 => result=&arg1
	OP_SET,//设置左值 eg: SET result,arg1 => *arg1=result
	OP_GET,//取右值 eg: GET result,arg1 => result=*arg1
	//跳转
	OP_JMP,//无条件跳转 eg: JMP result => goto result
	OP_JT,//真跳转	 eg: JT result,arg1 => if(arg1) goto result
	OP_JF,//假跳转	 eg: JF result,arg1 => if(!arg1) goto result
	/*OP_JG,OP_JGE,OP_JL,OP_JLE,OP_JE,*/OP_JNE,//跳转 eg:JG result,arg1,arg2 => if(arg1 > arg2) goto result
	//函数调用
	OP_ARG,//参数传递 eg: ARG arg1 => 传递参数arg1
	OP_PROC,//调用过程 eg: PROC fun => 调用fun函数,fun()
	OP_CALL,//调用函数 eg: CALL result,fun => 调用fun函数,返回值result=fun()
	OP_RET,//直接返回 eg: RET => return
	OP_RETV//带数据返回 eg:RET arg1 => return arg1
};

/*
	词法错误码
*/
enum LexError
{
	STR_NO_R_QUTION,		//字符串没有右引号
	NUM_BIN_TYPE,				//2进制数没有实体数据
	NUM_HEX_TYPE,				//16进制数没有实体数据
	CHAR_NO_R_QUTION,		//字符没有右引号
	CHAR_NO_DATA,				//字符没有数据
	OR_NO_PAIR,					//||只有一个|
	COMMENT_NO_END,			//多行注释没有正常结束
	TOKEN_NO_EXIST			//不存在的词法记号
};

/*
	语法错误码
*/
enum SynError
{
	TYPE_LOST,					//类型
	TYPE_WRONG,					
	ID_LOST,						//标志符
	ID_WRONG,						
	NUM_LOST,						//数组长度
	NUM_WRONG,
	LITERAL_LOST,				//常量
	LITERAL_WRONG,
	COMMA_LOST,					//逗号
	COMMA_WRONG,
	SEMICON_LOST,				//分号
	SEMICON_WRONG,
	ASSIGN_LOST,				//=
	ASSIGN_WRONG,
	COLON_LOST,					//冒号
	COLON_WRONG,
	WHILE_LOST,					//while
	WHILE_WRONG,
	LPAREN_LOST,				//(
	LPAREN_WRONG,
	RPAREN_LOST,				//)
	RPAREN_WRONG,
	LBRACK_LOST,				//[
	LBRACK_WRONG,
	RBRACK_LOST,				//]
	RBRACK_WRONG,
	LBRACE_LOST,				//{
	LBRACE_WRONG,
	RBRACE_LOST,				//}
	RBRACE_WRONG
};

/*
	语义错误码
*/
enum SemError
{
	VAR_RE_DEF,					//变量重定义
	FUN_RE_DEF,					//函数重定义
	VAR_UN_DEC,					//变量未声明
	FUN_UN_DEC,					//函数未声明
	FUN_DEC_ERR,				//函数声明与定义不匹配
	FUN_CALL_ERR,				//行参实参不匹配
	DEC_INIT_DENY,			//声明不允许初始化
	EXTERN_FUN_DEF,			//函数声明不能使用extern
	ARRAY_LEN_INVALID,	//数组长度无效
	VAR_INIT_ERR,				//变量初始化类型错误
	GLB_INIT_ERR,				//全局变量初始化值不是常量
	VOID_VAR,						//void变量
	EXPR_NOT_LEFT_VAL,	//无效的左值表达式
	ASSIGN_TYPE_ERR,		//赋值类型不匹配
	EXPR_IS_BASE,				//表达式不能是基本类型
	EXPR_NOT_BASE,			//表达式不是基本类型
	ARR_TYPE_ERR,				//数组运算类型错误
	EXPR_IS_VOID,				//表达式不能是VOID类型
	BREAK_ERR,					//break不在循环或switch-case中
	CONTINUE_ERR,				//continue不在循环中
	RETURN_ERR					//return语句和函数返回值类型不匹配
};

/*
	语义警告码
*/
enum SemWarn
{
	FUN_DEC_CONFLICT,		//函数参数列表类型冲突
	FUN_RET_CONFLICT		//函数返回值类型冲突
};

extern const char * tokenName[];

class Token;
class Scanner;
class Error;
class Lexer;
class Parser;
class SymTab;
class GenIR;
class Var;
class Fun;
class InterInst;
class InterCode;
class Compiler;
class Args;
class Block;
class DFG;
class LiveVar;

#define CONST
#define RED
#define DEAD
#define REG
#define PEEP









