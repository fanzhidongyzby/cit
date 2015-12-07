#pragma once
#include "common.h"
#include "set.h"
/*
	活跃变量数据流分析框架
*/
class LiveVar
{
	SymTab* tab;//符号表
	DFG*dfg;//数据流图指针
	//bool direct;//数据流问题方向：逆向
	list<InterInst*>optCode;//记录前面阶段优化后的代码
	vector<Var*>varList;//变量列表
	Set U;//全集
	Set E;//空集
	Set G;//全局变量集
	bool translate(Block*block);//活跃变量传递函数
public:
	void analyse();//活跃变量数据流分析
	LiveVar(DFG*g,SymTab*t,vector<Var*>&paraVar);//构造函数
	void elimateDeadCode(int stop=false);//死代码消除
	Set& getE();//返回空集
	vector<Var*> getCoVar(Set liveout);//根据提供的liveout集合提取优化后的变量集合——冲突变量
};
