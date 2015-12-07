#pragma once 
#include"common.h"
#include "iloc.h"

/*
	指令选择器
*/
class Selector
{
	vector<InterInst*>& ir;
	ILoc& iloc;
	void translate(InterInst*inst);//翻译
public:
	Selector(vector<InterInst*>& ir,ILoc& iloc);
	void select();//指令选择
};
