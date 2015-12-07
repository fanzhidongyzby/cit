#pragma once
#include "common.h"
#include "set.h"

/*
	表达式对象
*/
class Expr
{
	Var* newRs;//新的结果变量
	Var* rs;//结果变量，作为创建新结果变量的参考
	Var* genResult(SymTab*tab);//产生返回结果
public:
	Operator op;//操作符
	Var*arg1;//操作数1
	Var*arg2;//操作数2
	int index;//索引位置
	InterInst* genInst(SymTab*tab,DFG*dfg);//产生插入指令
	InterInst* genDec(DFG*dfg);//产生声明指令
	Var* getNewRs();//获取表达式的临时结果变量，必须保证已经在处理插入点时产生了结果！
	Expr(Var*r,Operator o,Var*a1,Var*a2);
	bool operator==(Expr&e);
	bool use(Var*v);
};

/*
	局部冗余消除算法
*/
class RedundElim
{
	//基本信息
	SymTab*symtab;//符号表指针
	DFG*dfg;//数据流图指针
	list<InterInst*>optCode;//记录前面阶段优化后的代码
	
	//表达式信息
	vector<Expr>exprList;//所有的表达式
	Set U;//全集
	Set E;//空集
	Set BU;//块全集
	Set BE;//块空集
	int findExpr(Expr&e);//查询表达式位置

	//数据流分析操作
	bool translate_anticipated(Block*block);//被预期执行表达式传递函数
	bool translate_available(Block*block);//可用表达式传递函数
	bool translate_postponable(Block*block);//可后延表达式传递函数
	bool translate_used(Block*block);//被使用表达式传递函数
	bool translate_dom(Block*block);//支配节点传递函数
	void analyse_anticipated();//被预期执行表达式数据流分析
	void analyse_available();//可用表达式数据流分析
	void analyse_postponable();//可后延表达式数据流分析
	void analyse_used();//被使用表达式数据流分析
	void analyse_dom();//支配节点数据流分析

public:

	//构造与初始化
	RedundElim(DFG*g,SymTab*tab);
	void elimate();//冗余消除
};

