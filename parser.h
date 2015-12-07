#pragma once
#include "common.h"

/*******************************************************************************
                                   语法分析器
*******************************************************************************/

class Parser
{
	//文法开始
	void program();
	void segment();
	Tag type();
	
	//声明与定义
	Var* defdata(bool ext,Tag t);
	void deflist(bool ext,Tag t);
	Var* varrdef(bool ext,Tag t,bool ptr,string name);
	Var* init(bool ext,Tag t,bool ptr,string name);
	void def(bool ext,Tag t);
	void idtail(bool ext,Tag t,bool ptr,string name);
	
	//函数
	Var* paradatatail(Tag t,string name);
	Var* paradata(Tag t);
	void para(vector<Var*>&list);
	void paralist(vector<Var*>&list);
	void funtail(Fun*f);
	void block();
	void subprogram();
	void localdef();
	
	//语句
	void statement();
	void whilestat();
	void dowhilestat();
	void forstat();
	void forinit();
	void ifstat();
	void elsestat();
	void switchstat();
	void casestat(Var*cond);
	Var* caselabel();
	
	//表达式
	Var* altexpr();
	Var* expr();
	Var* assexpr();
	Var* asstail(Var*lval);
	Var* orexpr();
	Var* ortail(Var*lval);
	Var* andexpr();
	Var* andtail(Var*lval);
	Var* cmpexpr();
	Var* cmptail(Var*lval);
	Tag cmps();
	Var* aloexpr();
	Var* alotail(Var*lval);
	Tag adds();
	Var* item();
	Var* itemtail(Var*lval);
	Tag muls();
	Var* factor();
	Tag lop();
	Var* val();
	Tag rop();
	Var* elem();
	Var* literal();
	Var* idexpr(string name);
	void realarg(vector<Var*> &args);
	void arglist(vector<Var*> &args);
	Var* arg();
	
	//词法分析
	Lexer &lexer;//词法分析器
	Token*look;//超前查看的字符
	
	//符号表
	SymTab &symtab;
	
	//中间代码生成器
	GenIR &ir;
	
	//语法分析与错误修复
	void move();//移进
	bool match(Tag t);//匹配,成功则移进
	void recovery(bool cond,SynError lost,SynError wrong);//错误修复

public:
	
	//构造与初始化
	Parser(Lexer&lex,SymTab&tab,GenIR&inter);
	
	//外部调用接口
	void analyse();//语法分析主程序
};


































