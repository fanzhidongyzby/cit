#include "compiler.h"
#include "lexer.h"
#include "error.h"
#include "symtab.h"
#include "genir.h"
#include "parser.h"

/*
	编译参数初始化
*/
bool Args::showChar=false;
bool Args::showToken=false;
bool Args::showSym=false;
bool Args::showIr=false;
bool Args::showOr=false;
bool Args::showBlock=false;
bool Args::showHelp=false;
bool Args::opt=false;

/*
	编译一个文件
*/
void Compiler::compile(char*file)
{
	//准备
	Scanner scanner(file);//扫描器
	Error error(&scanner);//错误处理
	Lexer lexer(scanner);//词法分析器
	SymTab symtab;//符号表
	GenIR ir(symtab);//中间代码生成器
	Parser parser(lexer,symtab,ir);//语法分析器
	//分析
	parser.analyse();//分析
	//报错
	if(Error::getErrorNum()+Error::getWarnNum())return;//出错不进行后续操作
	//中间结果
	if(Args::showSym)symtab.toString();//输出符号表
	if(Args::showIr)symtab.printInterCode();//输出中间代码
	//优化
	symtab.optimize();//执行优化
	if(Args::showOr)symtab.printOptCode();//输出优化后的中间代码
	//生成汇编代码
	symtab.genAsm(file);
}







