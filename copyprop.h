#pragma once
#include "common.h"
#include "set.h"

/*
	复写传播数据流分析框架
*/
class CopyPropagation
{
	DFG*dfg;//数据流图指针
	//bool direct;//数据流问题方向：前向
	list<InterInst*>optCode;//记录前面阶段优化后的代码
	vector<InterInst*>copyExpr;//复写表达式列表
	Set U;//全集
	Set E;//空集
	bool translate(Block*block);//复写传播传递函数
	void analyse();//复写传播数据流分析
	Var* __find(Set& in,Var*var,Var*src);//递归检测var赋值的源头的内部实现
	Var* find(Set& in,Var*var);//递归检测var赋值的源头
public:
	CopyPropagation(DFG*g);//构造函数
	void propagate();//执行复写传播
};
