#pragma once
#include "common.h"
#include "set.h"

/*
	四元式类，定义了中间代码的指令的形式
*/
class InterInst
{
private:
	string label;//标签
	Operator op;//操作符
	//union{
	Var *result;//运算结果
	InterInst*target;//跳转标号
	//};
	//union{
	Var*arg1;//参数1
	Fun*fun;//函数
	//};
	Var *arg2;//参数2

	bool first;//是否是首指令
	void init();//初始化

public:	

	//初始化
	Block*block;//指令所在的基本块指针

	//数据流信息
	vector<double>inVals;//常量传播in集合
	vector<double>outVals;//常量传播out集合
	Set e_use;//使用的表达式集合
	Set e_kill;//杀死的表达式集合
	RedundInfo info;//冗余删除数据流信息
	CopyInfo copyInfo;//复写传播数据流信息
	LiveInfo liveInfo;//活跃变量数据流信息
	bool isDead;//标识指令是否是死代码

	//参数不通过拷贝，而通过push入栈
	//指令在的作用域路径
	//vector<int>path;//该字段为ARG指令准备，ARG的参数并不代表ARG的位置！！！尤其是常量！！！
	//int offset;//参数的栈帧偏移
	
	//构造
	InterInst (Operator op,Var *rs,Var *arg1,Var *arg2=NULL);//一般运算指令
	InterInst (Operator op,Fun *fun,Var *rs=NULL);//函数调用指令,ENTRY,EXIT
	InterInst (Operator op,Var *arg1=NULL);//参数进栈指令,NOP
	InterInst ();//产生唯一标号
	InterInst (Operator op,InterInst *tar,Var *arg1=NULL,Var *arg2=NULL);//条件跳转指令,return
	void replace(Operator op,Var *rs,Var *arg1,Var *arg2=NULL);//替换表达式指令信息，用于常量表达式处理
	void replace(Operator op,InterInst *tar,Var *arg1=NULL,Var *arg2=NULL);//替换跳转指令信息，条件跳转优化
	~InterInst();//清理常量内存
	
	//外部调用接口
	void setFirst();//标记首指令
	
	bool isJcond();//是否条件转移指令JT,JF,Jcond
	bool isJmp();//是否直接转移指令JMP,return
	bool isFirst();//是首指令
	bool isLb();//是否是标签
	bool isDec();//是否是声明
	bool isExpr();//是基本类型表达式运算,可以对指针取值
	bool unknown();//不确定运算结果影响的运算(指针赋值，函数调用)
	
	Operator getOp();//获取操作符
	void callToProc();//替换操作符，用于将CALL转化为PROC
	InterInst* getTarget();//获取跳转指令的目标指令
	Var* getResult();//获取返回值
	Var* getArg1();//获取第一个参数
	Var* getArg2();//获取第二个参数
	string getLabel();//获取标签
	Fun* getFun();//获取函数对象
	void setArg1(Var*arg1);//设置第一个参数
	void toString();//输出指令
};

/*
	中间代码
*/
class InterCode
{
	vector<InterInst*>code;

public:
	//内存管理
	~InterCode();//清除内存
	
	//管理操作
	void addInst(InterInst*inst);//添加一条中间代码
	
	//关键操作
	void markFirst();//标识“首指令”

	//外部调用接口
	void toString();//输出指令
	vector<InterInst*>& getCode();//获取中间代码序列
};

