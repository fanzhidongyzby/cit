#include "intercode.h"
#include "symbol.h"
#include "genir.h"
#include "platform.h"

/*******************************************************************************
                                   四元式
*******************************************************************************/

/*
	初始化
*/
void InterInst::init()
{
	op=OP_NOP;
	this->result=NULL;
	this->target=NULL;
	this->arg1=NULL;
	this->fun=NULL;
	this->arg2=NULL;
	first=false;
	isDead=false;
}

/*
	一般运算指令
*/
InterInst::InterInst (Operator op,Var *rs,Var *arg1,Var *arg2)
{
	init();
	this->op=op;
	this->result=rs;
	this->arg1=arg1;
	this->arg2=arg2;
}

/*
	函数调用指令
*/
InterInst::InterInst (Operator op,Fun *fun,Var *rs)
{
	init();
	this->op=op;
	this->result=rs;
	this->fun=fun;
	this->arg2=NULL;
}

/*
	参数进栈指令
*/
InterInst::InterInst (Operator op,Var *arg1)
{
	init();
	this->op=op;
	this->result=NULL;
	this->arg1=arg1;
	this->arg2=NULL;
}

/*
	产生唯一标号
*/
InterInst::InterInst ()
{
	init();
	label=GenIR::genLb();
}

/*
	条件跳转指令
*/
InterInst::InterInst (Operator op,InterInst *tar,Var *arg1,Var *arg2)
{
	init();
	this->op=op;
	this->target=tar;
	this->arg1=arg1;
	this->arg2=arg2;
}

/*
	替换表达式指令信息
*/
void InterInst::replace(Operator op,Var *rs,Var *arg1,Var *arg2)
{
	this->op=op;
	this->result=rs;
	this->arg1=arg1;
	this->arg2=arg2;
}

/*
	替换跳转指令信息，条件跳转优化
*/
void InterInst::replace(Operator op,InterInst *tar,Var *arg1,Var *arg2)
{
	this->op=op;
	this->target=tar;
	this->arg1=arg1;
	this->arg2=arg2;
}

/*
	替换操作符，用于将CALL转化为PROC
*/
void InterInst::callToProc()
{
	this->result=NULL;//清除返回值
	this->op=OP_PROC;
}


/*
	清理常量内存
*/
InterInst::~InterInst()
{
	//if(arg1&&arg1->isLiteral())delete arg1;
	//if(arg2&&arg2->isLiteral())delete arg2;
}

/*
	输出指令信息
*/
void InterInst::toString()
{
	if(label!=""){
		printf("%s:\n",label.c_str());
		return;
	}
	switch(op)
	{
		//case OP_NOP:printf("nop");break;
		case OP_DEC:printf("dec ");arg1->value();break;
		case OP_ENTRY:printf("entry");break;
		case OP_EXIT:printf("exit");break;
		case OP_AS:result->value();printf(" = ");arg1->value();break;
		case OP_ADD:result->value();printf(" = ");arg1->value();printf(" + ");arg2->value();break;
		case OP_SUB:result->value();printf(" = ");arg1->value();printf(" - ");arg2->value();break;
		case OP_MUL:result->value();printf(" = ");arg1->value();printf(" * ");arg2->value();break;
		case OP_DIV:result->value();printf(" = ");arg1->value();printf(" / ");arg2->value();break;
		case OP_MOD:result->value();printf(" = ");arg1->value();printf(" %% ");arg2->value();break;
		case OP_NEG:result->value();printf(" = ");printf("-");arg1->value();break;
		case OP_GT:result->value();printf(" = ");arg1->value();printf(" > ");arg2->value();break;
		case OP_GE:result->value();printf(" = ");arg1->value();printf(" >= ");arg2->value();break;
		case OP_LT:result->value();printf(" = ");arg1->value();printf(" < ");arg2->value();break;
		case OP_LE:result->value();printf(" = ");arg1->value();printf(" <= ");arg2->value();break;
		case OP_EQU:result->value();printf(" = ");arg1->value();printf(" == ");arg2->value();break;
		case OP_NE:result->value();printf(" = ");arg1->value();printf(" != ");arg2->value();break;
		case OP_NOT:result->value();printf(" = ");printf("!");arg1->value();break;
		case OP_AND:result->value();printf(" = ");arg1->value();printf(" && ");arg2->value();break;
		case OP_OR:result->value();printf(" = ");arg1->value();printf(" || ");arg2->value();break;
		case OP_JMP:printf("goto %s",target->label.c_str());break;
		case OP_JT:printf("if( ");arg1->value();printf(" )goto %s",target->label.c_str());break;
		case OP_JF:printf("if( !");arg1->value();printf(" )goto %s",target->label.c_str());break;
		// case OP_JG:printf("if( ");arg1->value();printf(" > ");arg2->value();printf(" )goto %s",
		// 	target->label.c_str());break;
		// case OP_JGE:printf("if( ");arg1->value();printf(" >= ");arg2->value();printf(" )goto %s",
		// 	target->label.c_str());break;
		// case OP_JL:printf("if( ");arg1->value();printf(" < ");arg2->value();printf(" )goto %s",
		// 	target->label.c_str());break;
		// case OP_JLE:printf("if( ");arg1->value();printf(" <= ");arg2->value();printf(" )goto %s",
		// 	target->label.c_str());break;
		// case OP_JE:printf("if( ");arg1->value();printf(" == ");arg2->value();printf(" )goto %s",
		// 	target->label.c_str());break;
		case OP_JNE:printf("if( ");arg1->value();printf(" != ");arg2->value();printf(" )goto %s",
			target->label.c_str());break;
		case OP_ARG:printf("arg ");arg1->value();break;
		case OP_PROC:printf("%s()",fun->getName().c_str());break;
		case OP_CALL:result->value();printf(" = %s()",fun->getName().c_str());break;
		case OP_RET:printf("return goto %s",target->label.c_str());break;
		case OP_RETV:printf("return ");arg1->value();printf(" goto %s",target->label.c_str());break;
		case OP_LEA:result->value();printf(" = ");printf("&");arg1->value();break;
		case OP_SET:printf("*");arg1->value();printf(" = ");result->value();break;
		case OP_GET:result->value();printf(" = ");printf("*");arg1->value();break;
	}
	printf("\n");	
}

/*
	是否条件转移指令JT,JF,Jcond
*/
bool InterInst::isJcond()
{
	return op>=OP_JT&&op<=OP_JNE;
}

/*
	是否直接转移指令JMP,return
*/
bool InterInst::isJmp()
{
	return op==OP_JMP||op==OP_RET||op==OP_RETV;
}

/*
	标记首指令
*/
void InterInst::setFirst()
{
	first=true;
}

/*
	是首指令
*/
bool InterInst::isFirst()
{
	return first;
}

/*
	是否是标签
*/
bool InterInst::isLb()
{
	return label!="";
}

/*
	是基本类型表达式运算,可以对指针取值
*/
bool InterInst::isExpr()
{
	return (op>=OP_AS&&op<=OP_OR||op==OP_GET);//&&result->isBase();
}

/*
	不确定运算结果影响的运算(指针赋值，函数调用)
*/
bool InterInst::unknown()
{
	return op==OP_SET||op==OP_PROC||op==OP_CALL;
}

/*
	获取操作符
*/
Operator InterInst::getOp()
{
	return op;
}


/*
	获取跳转指令的目标指令
*/
InterInst* InterInst::getTarget()
{
	return target;
}

/*
	获取返回值
*/
Var*InterInst::getResult()
{
	return result;
}

/*
	设置第一个参数
*/
void InterInst::setArg1(Var*arg1)
{
	this->arg1=arg1;
}



/*
	是否是声明
*/
bool InterInst::isDec()
{
	return op==OP_DEC;
}

/*
	获取第一个参数
*/
Var* InterInst::getArg1()
{
	return arg1;
}

/*
	获取第二个参数
*/
Var* InterInst::getArg2()
{
	return arg2;
}

/*
	获取标签
*/
string InterInst::getLabel()
{
	return label;
}

/*
	获取函数对象
*/
Fun* InterInst::getFun()
{
	return fun;
}



/*******************************************************************************
                                   中间代码
*******************************************************************************/

/*
	添加中间代码
*/
void InterCode::addInst(InterInst*inst)
{
	code.push_back(inst);
}

/*
	输出指令信息
*/
void InterCode::toString()
{
	for(int i=0;i<code.size();i++)
	{
		code[i]->toString();
	}
}

/*
	清除内存
*/
InterCode::~InterCode()
{
	for(int i=0;i<code.size();i++)
	{
		delete code[i];
	}
}

/*
	标识“首指令”
*/
void InterCode::markFirst()
{
	unsigned int len=code.size();//指令个数，最少为2
	//标识Entry与Exit
	code[0]->setFirst();
	code[len-1]->setFirst();
	//标识第一条实际指令，如果有的话
	if(len>2)code[1]->setFirst();
	//标识第1条实际指令到倒数第2条指令
	for(unsigned int i=1;i<len-1;++i){
		if(code[i]->isJmp()||code[i]->isJcond()){//（直接/条件）跳转指令目标和紧跟指令都是首指令
			code[i]->getTarget()->setFirst();
			code[i+1]->setFirst();
		}
	}
}

/*
	获取中间代码序列
*/
vector<InterInst*>& InterCode::getCode()
{
	return code;
}


