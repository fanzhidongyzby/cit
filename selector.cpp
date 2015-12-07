#include "selector.h"
#include "intercode.h"
#include "symbol.h"
#include "platform.h"

Selector::Selector(vector<InterInst*>& irCode,ILoc& ilocCode)
	:ir(irCode),iloc(ilocCode)
{}

/*
	指令选择
*/
void Selector::select()
{
	for(unsigned int i=0;i<ir.size();i++){
		translate(ir[i]);
	}
}

/*
	翻译
*/
void Selector::translate(InterInst*inst)
{
	string label=inst->getLabel();
	if(label!=""){//标签打印
		iloc.label(label);
		return;
	}
	Operator op=inst->getOp();//操作符
	if(op==OP_ENTRY){//函数入口
		Fun*fun=inst->getFun();//函数
		iloc.comment("函数入口");
		iloc.inst("mov","ip","sp");
		if(fun->isRelocated())
			iloc.inst("stmfd","sp!","{r0-r7,fp,ip,lr,pc}");//保护现场
		else
			iloc.inst("stmfd","sp!","{fp,ip,lr,pc}");//不用保护现场
		iloc.inst("sub","fp","ip","#4");
		iloc.comment("开辟栈帧");
		iloc.allocStack(fun,"r8");//为fun分配栈帧
		iloc.comment("加载参数变量到寄存器");
		iloc.ldr_args(fun);//加载参数到对应的寄存器
		iloc.comment("函数内代码");
	}
	else if(op==OP_EXIT){//函数出口
		iloc.comment("函数出口");
		if(inst->getFun()->isRelocated())
			iloc.inst("ldmea","fp","{r0-r7,fp,sp,pc}");//恢复现场
		else
			iloc.inst("ldmea","fp","{fp,sp,pc}");//不用恢复现场
	}
	else if(op==OP_DEC){//处理声明的初始化
		iloc.init_var(inst->getArg1(),"r8","r9");//初始值在r8，可能用到r9
	}
	else if(op==OP_LEA){//取地址
		iloc.lea_var("r8",inst->getArg1());//&arg1 -> r8
		iloc.str_var("r8",inst->getResult(),"r9");//r8 -> rs 可能用到r9
	}
	else if(op==OP_SET){//设置指针值
		iloc.ldr_var("r8",inst->getResult());//rs -> r8
		iloc.ldr_var("r9",inst->getArg1());//arg1 -> r9
		//虽然基址寄存器r9不能修改（r9又是临时寄存器），但是disp=0，因此不会使用r9作为临时寄存器
		iloc.str_base("r8","r9",0,"r9",inst->getArg1()->isCharPtr());//rs -> *arg1
		iloc.nop();//占位
	}
	else if(op==OP_JMP||op==OP_JT||op==OP_JF||op==OP_JNE||op==OP_RET||op==OP_RETV){//跳转,函数返回
		string des=inst->getTarget()->getLabel();//目标地址
		iloc.ldr_var("r8",inst->getArg1());//arg1 -> r8
		iloc.ldr_var("r9",inst->getArg2());//arg2 -> r9 JNE需要
		switch(op){
			case OP_JMP:iloc.inst("b",des);break;
			case OP_JT:iloc.inst("cmp","r8","#0");iloc.inst("bne",des);break;
			case OP_JF:iloc.inst("cmp","r8","#0");iloc.inst("beq",des);break;
			case OP_RET:iloc.inst("b",des);break;
			case OP_RETV:iloc.inst("b",des);break;
			case OP_JNE:
				iloc.cmp("ne","eq","r8","r8","r9");//r8!=r9 -> r8
				iloc.inst("cmp","r8","#0");//r8!=0?
				iloc.inst("bne",des);
				break;
		}
	}
	else if(op==OP_ARG){//参数入栈,直接push arg1 !
		iloc.ldr_var("r8",inst->getArg1());//arg1 -> r8
		iloc.inst("stmfd","sp!","{r8}");//将r8压栈
		//iloc.str_base("r8","fp",inst->offset,"r9");//r8 -> [fp,#offset]
	}
	else if(op==OP_CALL){//调用返回值函数
		iloc.call_fun(inst->getFun(),"r9");//fun() -> r8,恢复栈帧可能用到r9
		iloc.str_var("r8",inst->getResult(),"r9");//r8 -> rs 可能用到r9
	}
	else if(op==OP_PROC){//调用无返回值函数
		iloc.call_fun(inst->getFun(),"r9");//fun()
	}
	else{//数值值表达式运算
		Var*rs=inst->getResult();
		Var*arg1=inst->getArg1();
		Var*arg2=inst->getArg2();
		iloc.ldr_var("r8",arg1);//arg1 -> r8
		iloc.ldr_var("r9",arg2);//arg2 -> r9
		switch(op)
		{
			//赋值
			case OP_AS:break;
			//算数
			case OP_ADD:iloc.inst("add","r8","r8","r9");break;
			case OP_SUB:iloc.inst("sub","r8","r8","r9");break;
			case OP_MUL:iloc.inst("mul","r10","r8","r9");iloc.inst("mov","r8","r10");break;
			case OP_DIV:iloc.call_lib("__divsi3","r8","r8","r9");break;
			case OP_MOD:iloc.call_lib("__modsi3","r8","r8","r9");break;
			case OP_NEG:iloc.inst("rsb","r8","r8","#0");break;//逆向减法r8=0-r8
			//比较
			case OP_GT:iloc.cmp("gt","le","r8","r8","r9");break;
			case OP_GE:iloc.cmp("ge","lt","r8","r8","r9");break;
			case OP_LT:iloc.cmp("lt","ge","r8","r8","r9");break;
			case OP_LE:iloc.cmp("le","gt","r8","r8","r9");break;
			case OP_EQU:iloc.cmp("eq","ne","r8","r8","r9");break;
			case OP_NE:iloc.cmp("ne","eq","r8","r8","r9");break;
			//逻辑
			case OP_AND:iloc.logic_and("r8","r8","r9");break;
			case OP_OR:iloc.logic_or("r8","r8","r9");break;
			case OP_NOT:iloc.logic_not("r8","r8");break;
			//指针,此处需要将类型转换考虑进去
			case OP_GET:iloc.ldr_base("r8","r8",0,rs->isChar());break;//a=*p
		}
		iloc.str_var("r8",rs,"r9");//r8 -> rs 可能用到r9
	}
}