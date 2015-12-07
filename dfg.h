#pragma once
#include"common.h"
#include"set.h"

/*
	基本块类
*/
class Block
{

public:
	
	//基本信息和关系
	list<InterInst*> insts;//基本块的指令序列
	list<Block*>prevs;//基本块的前驱序列
	list<Block*>succs;//基本块的后继序列
	bool visited;//访问标记
	bool canReach;//块可达标记,不可达块不作为数据流处理对象

	//数据流分析信息
	vector<double> inVals;//常量传播输入值集合
	vector<double> outVals;//常量传播输出值集合
	RedundInfo info;//冗余删除数据流信息
	CopyInfo copyInfo;//复写传播数据流信息
	LiveInfo liveInfo;//活跃变量数据流信息
	
	//构造与初始化
	Block(vector<InterInst*>&codes);

	//外部调用接口
	void toString();//输出基本块指令
};

/*
	数据流图类
*/
class DFG
{
	void createBlocks();//创建基本块
	void linkBlocks();//链接基本块

	void resetVisit();//重置访问标记
	bool reachable(Block*block);//测试块是否可达
	void release(Block*block);//如果块不可达，则删除所有后继，并继续处理所有后继


public:

	vector<InterInst*> codeList;//中间代码序列
	vector<Block*>blocks;//流图的所有基本块
	vector<InterInst*> newCode;//优化生成的中间代码序列，内存管理

	//构造与初始化
	DFG(InterCode&code);//初始化
	~DFG();//清理操作
	void delLink(Block*begin,Block*end);//删除块间联系，如果块不可达，则删除所有后继联系

	//核心实现
	void toCode(list<InterInst*>& opt);//导出数据流图为中间代码

	//外部调用接口
	void toString();//输出基本块
};



