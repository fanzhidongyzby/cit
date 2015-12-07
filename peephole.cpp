#include "peephole.h"
#include "platform.h"

/*******************************************************************************
                                   滑动窗口
*******************************************************************************/
/*
	初始化窗口
*/
Window::Window(int sz,list<Arm*>& cd):size(sz),code(cd)
{
	int i=0;
	for(pos=code.begin();pos!=code.end() && i<size;pos++,i++){//还有指令
		Arm*inst=*pos;//取出指令
		if(inst->dead)continue;//无效指令
		cont.push_back(inst);//移入新指令
	}
}

/*
	窗口移动函数，跳过无效指令
*/
bool Window::move()
{
	for(;pos!=code.end();pos++){//还有指令
		Arm*inst=*pos;//取出指令
		if(inst->dead)continue;//无效指令
		cont.pop_front();//移出前边的指令
		cont.push_back(inst);//移入新指令
		pos++;//移动到下个位置
		return true;//可以继续滑动
	}
	return false;//指令滑动完毕
}

/*
	指令模板匹配,若匹配执行替换动作
*/
bool Window::match()
{
	if(cont.size()!=size)return false;//窗口初始化失败，匹配一定失败
	if(size==1){//单指令匹配
		return match_single();
	}
	else if(size==2){//双指令匹配
		return match_double();
	}
	else if(size==3){//三指令匹配
		return match_triple();
	}
	return false;
}

/*
	单指令模板匹配,若匹配执行替换动作
*/
bool Window::match_single()
{
	Arm& inst=*cont.front();//取出指令
	string op=inst.opcode;//操作数
	string rs=inst.result;//结果
	string a1=inst.arg1;//参数1
	string a2=inst.arg2;//参数2
	string add=inst.addition;//附加信息
	bool flag=true;
	if((op=="sub" || op=="add") && a2=="#0"){//第二个参数是0,修改为mov
		inst.replace("mov",rs,a1);
	}
	else if(op=="mov" && rs==a1){//移动的目的寄存器和操作数相同，无效指令
		inst.setDead();
	}
	else flag=false;
	return flag;
}

/*
	双指令模板匹配,若匹配执行替换动作
*/
bool Window::match_double()
{
	//匹配成功后指令1设为无效，指令2为合并后的指令
	Arm& inst1=*cont.front();//取出指令1
	Arm& inst2=**(++cont.begin());//取出指令2
	string op1=inst1.opcode;//操作数
	string rs1=inst1.result;//结果
	string a11=inst1.arg1;//参数1
	string a12=inst1.arg2;//参数2
	string add1=inst1.addition;//附加信息
	string op2=inst2.opcode;//操作数
	string rs2=inst2.result;//结果
	string a21=inst2.arg1;//参数1
	string a22=inst2.arg2;//参数2
	string add2=inst2.addition;//附加信息
	bool flag=true;
	if(op1=="mov" && ((op2=="add" || op2=="sub" || op2=="rsb") 
					|| op2=="mul" && Plat::isReg(a11))
					&& rs1==a22){//mov合并到参数2
		inst1.setDead();
		inst2.replace(op2,rs2,a21,a11);
	}
	else if(op1=="mov" && ((op2=="add" || op2=="sub" || op2=="rsb") 
					|| op2=="mul" && Plat::isReg(a11))		
					&& rs1==a21 && Plat::isReg(a11)){//mov合并到参数1
		inst1.setDead();
		inst2.replace(op2,rs2,a11,a22);
	}
	else if(op2=="mov" && ((op1=="add" || op1=="sub" || op1=="rsb") && rs1==a21 ||
					op1=="mul" && rs1==a21 && rs2!=a11 && rs2!=a12)){//合并mov到结果
		inst1.setDead();
		inst2.replace(op1,rs2,a11,a12);
	}
	else if(op1=="mov" && op2=="mov" && rs1==a21){//双mov合并
		inst1.setDead();
		inst2.replace("mov",rs2,a11);
	}
	else if((op1=="ldr" || op1=="ldrb") && op2=="mov" && rs1==a21){//ldr mov合并
		inst1.setDead();
		inst2.replace(op1,rs2,a11);
	}
	else if((op2=="str" || op2=="strb") && op1=="mov" 
					&& Plat::isReg(a11) && rs1==rs2){//mov str合并
		inst1.setDead();
		inst2.replace(op2,a11,a21);
	}
	else if((op2=="ldr" || op2=="ldrb" || op2=="str" || op2=="strb") && op1=="mov"
					&& a21=="["+rs1+"]" && Plat::isReg(a11)){//mov合并到[r8]内存
		inst1.setDead();
		inst2.replace(op2,rs2,"["+a11+"]");
	}
	else if(op1=="mov" && op2=="cmp" && rs1==a21){//mov cmp合并比较数
		inst1.setDead();
		inst2.replace("cmp",rs2,a11);
	}
	else if(op1=="mov" && op2=="cmp" && rs1==rs2 && Plat::isReg(a11)){//mov cmp合并被比较数
		inst1.setDead();
		inst2.replace("cmp",a11,a21);
	}
	else if((op1=="ldr" && op2=="str" || op1=="ldrb" && op2=="strb")
					&& rs1==rs2 && a11==a21){//重复的ldr str
		inst1.setDead();
		inst2.replace(op1,rs1,a11);
	}
	else if(op1=="mov" && op2=="stmfd" && Plat::isReg(a11) && a21=="{"+rs1+"}"){//mov入栈合并
		inst1.setDead();
		inst2.replace(op2,rs2,"{"+a11+"}");
	}
	else if(op1=="b" && rs2==":" && rs1==op2){//跳转优化
		inst1.setDead();//删除当前跳转指令，不能删除标号，还可能有其他跳转到此
	}
	else flag=false;
	return flag;
}

/*
	三指令模板匹配,若匹配执行替换动作
*/
bool Window::match_triple()
{
	//匹配成功后指令1设为无效，指令2为合并后的指令
	Arm& inst1=*cont.front();//取出指令1
	Arm& inst2=**(++cont.begin());//取出指令2
	Arm& inst3=**(++++cont.begin());//取出指令2
	string op1=inst1.opcode;//操作数
	string rs1=inst1.result;//结果
	string a11=inst1.arg1;//参数1
	string a12=inst1.arg2;//参数2
	string add1=inst1.addition;//附加信息
	string op2=inst2.opcode;//操作数
	string rs2=inst2.result;//结果
	string a21=inst2.arg1;//参数1
	string a22=inst2.arg2;//参数2
	string add2=inst2.addition;//附加信息
	string op3=inst3.opcode;//操作数
	string rs3=inst3.result;//结果
	string a31=inst3.arg1;//参数1
	string a32=inst3.arg2;//参数2
	string add3=inst3.addition;//附加信息
	bool flag=true;
	if((op1=="str" && op3=="ldr" || op1=="strb" && op3=="ldrb") && op2==""
					&& rs1==rs3 && a11==a31){//重复的str ldr
		inst1.setDead();//消除ldr，指令后移
		inst2.replace(op1,rs1,a11);
		inst3.replace("");
	}
	else if(!op1.find("mov") && op1.size()>3 &&
					!op2.find("mov") && op2.size()>3 &&
					op3=="mov" && rs1==rs2 && rs2==a31){//比较结果合并mov
		inst1.setDead();//消除mov，指令后移
		inst2.replace(op1,rs3,a11);
		inst3.replace(op2,rs3,a21);
	}
	else flag=false;
	return flag;
}


/*******************************************************************************
                                   窥孔优化器
*******************************************************************************/
/*
	优化器初始化
*/
PeepHole::PeepHole(ILoc&il):iloc(il)
{}

/*
	内部过滤器，只处理一遍
*/
bool PeepHole::__filter(int sz)
{
	Window win(sz,iloc.getCode());//新建滑动窗口
	bool flag=false;//记录匹配成功出现标记
	do
	{
		bool ret=win.match();//执行匹配
		if(!flag&&ret)flag=ret;//尝试设定成功标记，不重复设定
	}while(win.move());//移动滑动窗口,直到不出现匹配成功为止
	return flag;//返回是否发生匹配标记
}

/*
	窥孔优化
*/
void PeepHole::filter()
{
	while(__filter(1) || __filter(2)||__filter(3));//开辟大小1,2,3的窗口，直到不能匹配为止
}
