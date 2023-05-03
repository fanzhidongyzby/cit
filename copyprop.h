#pragma once
#include "common.h"
#include "set.h"
#include <ext/hash_map>

using namespace __gnu_cxx;

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

	// 定义哈希函数
    struct var_pointer_hash{
        size_t operator()(const Var *ptr) const{
            return reinterpret_cast<size_t>(ptr);
        }
    };

    vector<int> uf;// 并查集,用来判断两个变量是否是等价的比如b=a;c=b;那么a,b和c就是等价的
    hash_map<Var *, int, var_pointer_hash> varToIndex;// 将每个变量映射到一个数字
    int findParent(int x);// 找出该变量所属的联通分量

	bool translate(Block*block);//复写传播传递函数
	void analyse();//复写传播数据流分析
	Var* __find(Set& in,Var*var,Var*src);//递归检测var赋值的源头的内部实现
	Var* find(Set& in,Var*var);//递归检测var赋值的源头
public:
	CopyPropagation(DFG*g);//构造函数
	void propagate();//执行复写传播
};
