#pragma once 
#include "common.h"
/*
	中间代码生成器
*/
class GenIR
{
	static int lbNum;//标签号码，用于产生唯一的标签
	
	SymTab &symtab;//符号表
	
	//break continue辅助标签列表
	vector< InterInst* > heads;
	vector< InterInst* > tails;
	void push(InterInst*head,InterInst*tail);//添加一个作用域
	void pop();//删除一个作用域
	
	
	//函数调用
	void genPara(Var*arg);//参数传递语句
	
	//双目运算
	Var* genAssign(Var*lval,Var*rval);//赋值语句
	
	Var* genOr(Var*lval,Var*rval);//或运算语句
	Var* genAnd(Var*lval,Var*rval);//与运算语句
	Var* genGt(Var*lval,Var*rval);//大于语句
	Var* genGe(Var*lval,Var*rval);//大于等于语句
	Var* genLt(Var*lval,Var*rval);//小于语句
	Var* genLe(Var*lval,Var*rval);//小于等于语句
	Var* genEqu(Var*lval,Var*rval);//等于语句
	Var* genNequ(Var*lval,Var*rval);//不等于语句
	Var* genAdd(Var*lval,Var*rval);//加法语句
	Var* genSub(Var*lval,Var*rval);//减法语句
	Var* genMul(Var*lval,Var*rval);//乘法语句
	Var* genDiv(Var*lval,Var*rval);//除法语句
	Var* genMod(Var*lval,Var*rval);//取模
	
	//单目运算
	Var* genNot(Var*val);//非
	Var* genMinus(Var*val);//负
	Var* genIncL(Var*val);//左自加语句
	Var* genDecL(Var*val);//右自减语句
	Var* genLea(Var*val);//取址语句
	Var* genPtr(Var*val);//指针取值语句	
	Var* genIncR(Var*val);//右自加语句
	Var* genDecR(Var*val);//右自减
	
public:

	GenIR(SymTab &tab);//重置内部数据

	Var* genAssign(Var*val);//变量拷贝赋值，用于指针左值引用和变量复制
	
	//产生符号和语句
	Var* genArray(Var*array,Var*index);//数组索引语句
	Var* genCall(Fun*function,vector<Var*> & args);//函数调用语句
	Var* genTwoOp(Var*lval,Tag opt,Var*rval);//双目运算语句	
	Var* genOneOpLeft(Tag opt,Var*val);//左单目运算语句	
	Var* genOneOpRight(Var*val,Tag opt);//右单目运算语句
	
	//产生复合语句
	void genWhileHead(InterInst*& _while,InterInst*& _exit);//while循环头部
	void genWhileCond(Var*cond,InterInst* _exit);//while条件
	void genWhileTail(InterInst*& _while,InterInst*& _exit);//while尾部
	void genDoWhileHead(InterInst*& _do,InterInst*& _exit);//do-while头部
	void genDoWhileTail(Var*cond,InterInst* _do,InterInst* _exit);//do-while尾部
	void genForHead(InterInst*& _for,InterInst*& _exit);//for循环头部
	void genForCondBegin(Var*cond,InterInst*& _step,InterInst*& _block,InterInst* _exit);//for条件开始
	void genForCondEnd(InterInst* _for,InterInst* _block);//for循环条件结束部分
	void genForTail(InterInst*& _step,InterInst*& _exit);//for循环尾部
	void genIfHead(Var*cond,InterInst*& _else);//if头部
	void genIfTail(InterInst*& _else);//if尾部
	void genElseHead(InterInst* _else,InterInst*& _exit);//else头部
	void genElseTail(InterInst*& _exit);//else尾部
	void genSwitchHead(InterInst*& _exit);//switch头部
	void genSwitchTail(InterInst* _exit);//switch尾部
	void genCaseHead(Var*cond,Var*lb,InterInst*& _case_exit);//case头部
	void genCaseTail(InterInst* _case_exit);//case尾部	
	
	//产生特殊语句
	void genBreak();//产生break语句
	void genContinue();//产生continue语句
	void genReturn(Var*ret);//产生return语句
	bool genVarInit(Var*var);//产生变量初始化语句
	void genFunHead(Fun*function);//产生函数入口语句
	void genFunTail(Fun*function);//产生函数出口语句
	
	//全局函数
	static string genLb();//产生唯一的标签	
	static bool typeCheck(Var*lval,Var*rval);//检查类型是否可以转换
};

