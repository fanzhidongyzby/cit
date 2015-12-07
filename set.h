#pragma once
#include"common.h" 

/*
	集合类——使用位图表达集合运算
*/
class Set
{
	vector<unsigned int>bmList;//位图列表：位图长度大于32时

public:
	int count;//记录集合元素个数

	//构造与初始化
	Set();
	Set(int size,bool val);//构造一个容纳size个元素的集合，并初始化
	void init(int size,bool val);
	void p();//调试输出函数

	//集合基本运算
	Set operator &(Set val);//交集运算
	Set operator |(Set val);//并集运算
	Set operator -(Set val);//差集运算
	Set operator ^(Set val);//异或运算
	Set operator ~();//补集运算
	bool operator ==(Set& val);//比较运算
	bool operator !=(Set& val);//比较运算
	bool get(int i);//索引运算
	void set(int i);//置位运算
	void reset(int i);//复位运算
	int max();//返回最高位的1的索引
	bool empty()
	{
		return bmList.size()==0;
	}
};


/*
	数据流传播信息
*/
struct TransInfo
{
	Set in;//输入集合
	Set out;//输出集合
};

/*
	冗余消除数据流信息
*/
struct RedundInfo
{
	TransInfo anticipated;//被预期执行表达式集合
	TransInfo available;//可用表达式集合
	TransInfo postponable;//可后延表达式集合
	TransInfo used;//被使用表达式集合
	Set earliest;//最前放置表达式集合：earliest(B)=anticipated(B).in-available(B).in
	Set latest;//最后放置表达式集合：latest(B)=(earliest(B) | postponable(B).in) &
						 //	(e_use(B) | ~(&( earliest(B.succ[i]) | postponable(B.succ[i]).in )))
	TransInfo dom;//基本块的前驱集合
	int index;//基本块的索引
};

/*
	复写传播的数据流信息
*/
struct CopyInfo
{
	Set in;//输入集合
	Set out;//输出集合
	Set gen;//产生复写表达式集合
	Set kill;//杀死复写表达式集合
};

/*
	活跃变量的数据流信息
*/
struct LiveInfo
{
	Set in;//输入集合
	Set out;//输出集合
	Set use;//使用变量集合——变量的使用先与定值
	Set def;//定值变量集合——变量的定值先与使用
};