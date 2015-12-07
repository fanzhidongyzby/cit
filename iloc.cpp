#include "iloc.h"
#include "platform.h"
#include "symbol.h"
#include <sstream>

Arm::Arm(string op,string rs,string a1,string a2,string add)
	:opcode(op),result(rs),arg1(a1),arg2(a2),addition(add),dead(false)
{}

/*
	指令内容替换
*/
void Arm::replace(string op,string rs,string a1,string a2,string add)
{
	opcode=op;
	result=rs;
	arg1=a1;
	arg2=a2;
	addition=add;
}

/*
	设置为无效指令
*/
void Arm::setDead()
{
	dead=true;
}

/*
	输出函数
*/
string Arm::outPut()
{
	if(dead)return "";
	if(opcode=="")return "";//占位指令
	string ret=opcode+" "+result;
	if(arg1!="")ret+=","+arg1;
	if(arg2!="")ret+=","+arg2;
	if(addition!="")ret+=","+addition;
	return ret;
}


#define emit(args...) code.push_back(new Arm(args))

/*
		整数的字符串表达
		flag:是否在数字前添加#符号
*/
string ILoc::toStr(int num,int flag)
{
	string ret="";
	if(flag)ret="#";
	stringstream ss;
	ss<<num;
	ret+=ss.str();//#123 =123
	return ret;
}

/*
	产生标签
*/
void ILoc::label(string name)
{
	emit(name,":");//.L1:
}

/*
	产生注释
*/
void ILoc::comment(string str)
{
	emit("#",str);//#hello world
}

/*
	0个操作数
*/
void ILoc::inst(string op,string rs)
{
	emit(op,rs);
}

/*
	一个操作数
*/
void ILoc::inst(string op,string rs,string arg1)
{
	emit(op,rs,arg1);
}

/*
	两个操作数
*/
void ILoc::inst(string op,string rs,string arg1,string arg2)
{
	emit(op,rs,arg1,arg2);
}

/*
	加载立即数 ldr r0,=#100
*/
void ILoc::ldr_imm(string rsReg,int num)
{
	if(Plat::constExpr(num))
		emit("mov",rsReg,toStr(num));//mov r8,#12
	else
		ldr_lb(rsReg,toStr(num,false));//ldr r8,=0xfff0
}

/*
	加载符号值 ldr r0,=g ldr r0,=.L1
*/
void ILoc::ldr_lb(string rsReg,string name)
{
	emit("ldr",rsReg,"="+name);//ldr r8,=.L1
}

/*
	基址寻址 ldr r0,[fp,#100]
*/
void ILoc::ldr_base(string rsReg,string base,int disp,bool isChar)
{
	if(Plat::isDisp(disp)){//有效的偏移常量
		if(disp)base+=","+toStr(disp);//[fp,#-16] [fp]
	}
	else{
		ldr_imm(rsReg,disp);//ldr r8,=-4096
		base+=","+rsReg;//fp,r8
	}
	base="["+base+"]";//内存寻址
	emit(isChar?"ldrb":"ldr",rsReg,base);//ldr r8,[fp,#-16] ldr r8,[fp,r8]
}

/*
	基址寻址 str r0,[fp,#100]
*/
void ILoc::str_base(string srcReg,string base,int disp,string tmpReg,bool isChar)
{
	if(Plat::isDisp(disp)){//有效的偏移常量
		if(disp)base+=","+toStr(disp);//[fp,#-16] [fp]
	}
	else{
		ldr_imm(tmpReg,disp);//ldr r9,=-4096
		base+=","+tmpReg;//fp,r9
	}
	base="["+base+"]";//内存寻址
	emit(isChar?"strb":"str",srcReg,base);//str r8,[fp,#-16] str r8,[fp,r9]
}

/*
	变量的初始化
*/
void ILoc::init_var(Var*var,string initReg,string tmpReg)
{
	if(!var->unInit()){//变量初始化了
		if(var->isBase())//基本类型初始化 100 'a'
			ldr_imm(initReg,var->getVal());//ldr r8,#100
		else//字符指针初始化
			ldr_lb(initReg,var->getPtrVal());//ldr r8,=.L1, .L1 byte "1234567"
		//初始值已经加载完成！一般在r8
		str_var(initReg,var,tmpReg);//将初始值保存到变量内存
	}
}

/*
	加载变量到寄存器，保证将变量放到reg中：r8/r9
*/
void ILoc::ldr_var(string rsReg,Var*var)
{
	if(!var)return;//无效变量
	bool isChar=var->isChar();//是否是字符变量，需要类型转换
	if(var->notConst()){//非常量
		int id=var->regId;//-1表示非寄存器，其他表示寄存器的索引值
		if(id!=-1)//寄存器变量
			emit("mov",rsReg,Plat::regName[id]);//mov r8,r2 | 这里有优化空间——消除r8
		else{//所有定义的变量和数组
			int off=var->getOffset();//栈帧偏移
			bool isGlb=!off;//变量偏移，0表示全局，其他表示局部
			bool isVar=!var->getArray();//纯变量，0表示数组，1表示变量
			if(isGlb){//全局符号
				ldr_lb(rsReg,var->getName());//ldr r8,=glb
				if(isVar)ldr_base(rsReg,rsReg,0,isChar);//ldr r8,[r8]
			}
			else{//局部符号
				if(isVar)ldr_base(rsReg,"fp",off,isChar);//ldr r8,[fp,#-16]
				else leaStack(rsReg,off);//add r8,fp,#-16
			}
		}
	}
	else{//常量
		if(var->isBase())//是基本类型 100 'a'
			ldr_imm(rsReg,var->getVal());//ldr r8,#100
		else//常量字符串
			ldr_lb(rsReg,var->getName());//ldr r8,=.L1, .L1 byte "1234567"
	}
}

/*
	加载变量地址到寄存器
*/
void ILoc::lea_var(string rsReg,Var*var)
{
	//被加载的变量肯定不是常量！
	//被加载的变量肯定不是寄存器变量！
	int off=var->getOffset();//栈帧偏移
	bool isGlb=!off;//变量偏移，0表示全局，其他表示局部
	if(isGlb)//全局符号
		ldr_lb(rsReg,var->getName());//ldr r9,=glb 地址寄存器加载变量地址
	else{//局部符号
		leaStack(rsReg,off);//lea r8,[fp,#-16]
	}
}

/*
	保存寄存器到变量，保证将计算结果（r8）保存到变量
*/
void ILoc::str_var(string srcReg,Var*var,string tmpReg)
{
	//被保存目标变量肯定不是常量！
	int id=var->regId;//-1表示非寄存器，其他表示寄存器的索引值
	bool isChar=var->isChar();//是否是字符变量，需要类型转换
	if(id!=-1){//寄存器变量
		emit("mov",Plat::regName[id],srcReg);//mov r2,r8 | 这里有优化空间——消除r8
	}
	else{//所有定义的变量,不可能是数组！
		int off=var->getOffset();//栈帧偏移
		bool isGlb=!off;//变量偏移，0表示全局，其他表示局部
		if(isGlb){//全局符号
			ldr_lb(tmpReg,var->getName());//ldr r9,=glb 地址寄存器加载变量地址
			str_base(srcReg,tmpReg,0,tmpReg,isChar);//str r8,[r9]
		}
		else{//局部符号
			str_base(srcReg,"fp",off,tmpReg,isChar);//str r8,[fp,#-16]
		}
	}
	nop();//分割优化
}

/*
	加载栈内变量地址
*/
void ILoc::leaStack(string rsReg,int off)
{
	if(Plat::constExpr(off))
		emit("add",rsReg,"fp",toStr(off));//add r8,fp,#-16
	else{
		ldr_imm(rsReg,off);//ldr r8,=-257
		emit("add",rsReg,"fp",rsReg);//add r8,fp,r8
	}
}

/*
	分配栈帧
*/
void ILoc::allocStack(Fun*fun,string tmpReg)
{
	// int base=(fun->isRelocated())?(Plat::stackBase_protect):(Plat::stackBase);
	int base=Plat::stackBase;
	if(fun->isRelocated())base=Plat::stackBase_protect;
	int off=fun->getMaxDep()-base;//计算栈帧大小
	if(Plat::constExpr(off))
		emit("sub","sp","sp",toStr(off));//sub sp,sp,#16
	else{
		ldr_imm(tmpReg,off);//ldr r8,=257
		emit("sub","sp","sp",tmpReg);//sub sp,sp,r8
	}
}

/*
	加载函数的参数到寄存器
*/
void ILoc::ldr_args(Fun*fun)
{
	vector<Var*> args=fun->getParaVar();//获取参数
	for(unsigned int i=0;i<args.size();i++){
		Var*arg=args[i];
		if(arg->regId!=-1){//参数在寄存器，需要加载,不用理会是否是字符，统一4字节加载
			ldr_base(Plat::regName[arg->regId],"fp",arg->getOffset(),false);//ldr r0,[fp,#4]
		}
	}
}

/*
	调用库函数:rsReg=fun(arg1,arg2)
*/
void ILoc::call_lib(string fun,string rsReg,string reg1,string reg2)
{
	emit("stmfd","sp!","{r0-r7}");//保护现场
	emit("mov","r0",reg1);//传递操作数
	emit("mov","r1",reg2);
	emit("bl",fun);
	emit("mov",rsReg,"r0");//取返回结果
	emit("ldmfd","sp!","{r0-r7}");//恢复现场
}

/*
	调用函数fun
*/
void ILoc::call_fun(Fun*fun,string tmpReg)
{
	string funName=fun->getName();
	emit("bl",funName);//函数返回值在r8,不需要保护
	int off=fun->getParaVar().size();//需要弹出参数的个数
	off*=4;//参数都是4字节
	if(Plat::constExpr(off))
		emit("add","sp","sp",toStr(off));//add sp,sp,#16
	else{
		ldr_imm(tmpReg,off);//ldr r8,=257
		emit("add","sp","sp",tmpReg);//add sp,sp,r8
	}
}

/*
	逻辑与
*/
void ILoc::logic_and(string rsReg,string reg1,string reg2)
{
	emit("cmp",reg1,"#0");//r8=0?
	emit("moveq",rsReg,"#0");//r8=0
	emit("movne",rsReg,"#1");//r8=1
	emit("cmpne",reg2,"#0");//r9=0?
	emit("moveq",rsReg,"#0");//r8=0
}

/*
	逻辑或
*/
void ILoc::logic_or(string rsReg,string reg1,string reg2)
{
	emit("cmp",reg1,"#0");//r8=0?
	emit("moveq",rsReg,"#0");//r8=0
	emit("movne",rsReg,"#1");//r8=1
	emit("cmpeq",reg2,"#0");//r9=0?
	emit("movne",rsReg,"#1");//r8=0
}

/*
	逻辑非
*/
void ILoc::logic_not(string rsReg,string reg1)
{
	emit("cmp",reg1,"#0");//r8=0
	emit("moveq",rsReg,"#0");//r8=1
	emit("movne",rsReg,"#1");//r8=0
}

/*
	比较
*/
void ILoc::cmp(string cmp,string cmpnot,string rsReg,string reg1,string reg2)
{
	emit("cmp",reg1,reg2);//r8 ? r9
	emit("mov"+cmp,rsReg,"#1");//r8=0
	emit("mov"+cmpnot,rsReg,"#0");//r8=1
}

/*
	占位
*/
void ILoc::nop()
{
	emit("");//无操作符
}


ILoc::~ILoc()
{
	for(list<Arm*>::iterator it=code.begin();it!=code.end();++it)
	{
		delete (*it);
	}
}

/*
	输出汇编
*/
void ILoc::outPut(FILE*file)
{
	for(list<Arm*>::iterator it=code.begin();it!=code.end();++it)
	{
		string s=(*it)->outPut();
		if(s!="")fprintf(file,"\t%s\n",s.c_str());
	}
}

/*
	获取代码
*/
list<Arm*>& ILoc::getCode()
{
	return code;
}
