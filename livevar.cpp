#include "livevar.h"
#include "dfg.h"
#include "intercode.h"
#include "symbol.h"
#include "symtab.h"

LiveVar::LiveVar(DFG*g,SymTab*t,vector<Var*>&paraVar):dfg(g),tab(t)
{
	//函数外变量：所有全局变量，函数参数变量
	varList=tab->getGlbVars();//全局变量
	int glbNum=varList.size();//全局变量个数
	for(unsigned int i=0;i<paraVar.size();++i)
		varList.push_back(paraVar[i]);//将参数放入列表
	dfg->toCode(optCode);//提取中间代码
	//统计所有的局部变量
	for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
		InterInst*inst=*i;//遍历指令
		Operator op=inst->getOp();//获取操作符
		if(op==OP_DEC){//局部变量声明
			varList.push_back(inst->getArg1());//记录变量
		}
	}
	U.init(varList.size(),1);//初始化基本集合,全集
	E.init(varList.size(),0);//初始化基本集合,空集
	G=E;//初始化
	for(int i=0;i<glbNum;i++)G.set(i);//初始化全局变量集合
	for(unsigned int i=0;i<varList.size();i++)//遍历所有变量
	{
		varList[i]->index=i;//记录变量的列表索引
	}
	//初始化指令的指令的use和def集合
	for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
		InterInst*inst=*i;//遍历指令
		inst->liveInfo.use=E;//初始化使用集合与杀死集合
		inst->liveInfo.def=E;
		Var*rs=inst->getResult();//获取结果
		Operator op=inst->getOp();//获取操作符
		Var*arg1=inst->getArg1();//获取操作数1
		Var*arg2=inst->getArg2();//获取操作数2
		if(op>=OP_AS&&op<=OP_LEA){//常规运算
			inst->liveInfo.use.set(arg1->index);//使用arg1
			if(arg2)
				inst->liveInfo.use.set(arg2->index);//使用arg2
			if(rs!=arg1 && rs!=arg2)
				inst->liveInfo.def.set(rs->index);//定值result
		}
		else if(op==OP_DEC){
			//inst->liveInfo.def.set(arg1->index);//声明语句定值arg1
		}
		else if(op==OP_SET){//*arg1=result
			inst->liveInfo.use.set(rs->index);//必然使用了result
			//inst->liveInfo.def=E;//可能定值了哪个变量，不能消除其他变量的活跃信息
			//inst->liveInfo.def.reset(rs->index);//但是绝对不可能定值result！
		}
		else if(op==OP_GET){//result=*arg1
			inst->liveInfo.use=U;//可能使用了任何变量，保守认为使用了所有变量
			//inst->liveInfo.def=E;//不确定是否对result定值，不消除活跃信息，arg可能指向result
		}
		else if(op==OP_RETV){//return arg1
			inst->liveInfo.use.set(arg1->index);
		}
		else if(op==OP_ARG){//arg arg1
			if(arg1->isBase())
				inst->liveInfo.use.set(arg1->index);//基本类型为使用
			else{
				inst->liveInfo.use=U;//可能使用全部的变量
				//保守认为没有杀死所有变量
			}
		}
		else if(op==OP_CALL||op==OP_PROC){//[result]=arg1()
			inst->liveInfo.use=G;//可能使用了所有的全局变量
			//保守认为没有杀死所有变量
			if(rs){
				if(rs->getPath().size()>1){//有返回值,且返回值不是全局变量
					inst->liveInfo.def.set(rs->index);//定值了返回值result
				}
			}
		}
		else if(op==OP_JF||op==OP_JT){//if([!]arg1)goto result
			inst->liveInfo.use.set(arg1->index);//使用条件arg1
		}
	}
}

/*
	活跃变量传递函数：f(x)=(x - def(B)) | use(B)
*/
bool LiveVar::translate(Block*block)
{
	Set tmp=block->liveInfo.out;//记录B.out作为最后一个指令的输入
	//逆序处理基本块的每一个指令
	for(list<InterInst*>::reverse_iterator i=block->insts.rbegin();i!=block->insts.rend();++i){
		InterInst*inst=*i;//取出指令
		if(inst->isDead)continue;//跳过死代码
		Set& in=inst->liveInfo.in;//指令的in
		Set& out=inst->liveInfo.out;//指令的out
		out=tmp;//设置指令的out
		in=inst->liveInfo.use | (out-inst->liveInfo.def);//指令级传递函数in=f(out)
		tmp=in;//用当前指令的in设置下次使用的out
	}
	bool flag=tmp!=block->liveInfo.in;//是否变化
	block->liveInfo.in=tmp;//设定B.in集合
	return flag;
}

/*
	活跃变量数据流分析函数
	direct : 数据流方向,逆向
	init : 初始化集合,B.in=E
	bound : 边界集合，Exit.in=E
	join : 交汇运算，并集
	translate : 传递函数,f(x)=(x - def(B)) | use(B)
*/
void LiveVar::analyse()
{
	//解除寄存器分配时随意取变量liveout集合的bug
	dfg->blocks[dfg->blocks.size()-1]->liveInfo.out=E;//初始化边界集合Exit.out=E
	translate(dfg->blocks[dfg->blocks.size()-1]);//传播给内部指令——其实就是exit
	//正常的数据流分析
	dfg->blocks[dfg->blocks.size()-1]->liveInfo.in=E;//初始化边界集合Exit.in=E
	for(unsigned int i=0;i<dfg->blocks.size()-1;++i){
		dfg->blocks[i]->liveInfo.in=E;//初始化其他基本块B.in=E
	}
	bool change=true;//集合变化标记
	while(change){//B.in集合发生变化
		change=false;//重新设定变化标记
		for(int i=dfg->blocks.size()-2;i>=0;--i){//逆序，任意B!=Exit
			if(!dfg->blocks[i]->canReach)continue;//块不可达不处理
			Set tmp=E;//保存交汇运算结果
			for(list<Block*>::iterator j=dfg->blocks[i]->succs.begin();
				j!=dfg->blocks[i]->succs.end();++j){//succ[i]
				tmp=tmp | (*j)->liveInfo.in;
			}
			//B.out=| succ[j].in
			dfg->blocks[i]->liveInfo.out=tmp;
			if(translate(dfg->blocks[i]))//传递函数执行时比较前后集合是否有差别
				change=true;
		}
	}
	// /*
	// 	调试输出
	// */
	// for(unsigned int i=0;i<dfg->blocks.size();++i){
	// 	printf("<%d>", i);
	// 	dfg->blocks[i]->liveInfo.in.p();
	// 	printf("\t");
	// 	dfg->blocks[i]->liveInfo.out.p();
	// 	printf("\n");
	// }
}

/*
	根据提供的liveout集合提取优化后的变量集合——冲突变量
*/
vector<Var*> LiveVar::getCoVar(Set liveout)
{
	vector<Var*> coVar;
	for(unsigned int i=0;i<varList.size();i++){
		if(liveout.get(i)){
			coVar.push_back(varList[i]);//将活跃的变量保存
		}
	}
	return coVar;
}

/*
	返回空集对象
*/
Set& LiveVar::getE()
{
	return E;
}

/*
	死代码消除
*/
void LiveVar::elimateDeadCode(int stop)
{
	if(stop){//没有新的死代码生成,清理多余的声明
		//计算哪些变量存在非死代码中，记录变量活跃性
		for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
			InterInst*inst=*i;//遍历指令
			Operator op=inst->getOp();//获取操作符
			if(inst->isDead||op==OP_DEC)continue;//跳过死代码和声明指令
			Var*rs=inst->getResult();//获取结果
			Var*arg1=inst->getArg1();//获取操作数1
			Var*arg2=inst->getArg2();//获取操作数2
			if(rs)rs->live=true;
			if(arg1)arg1->live=true;
			if(arg2)arg2->live=true;
		}
		//删除无效的DEC命令
		for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
			InterInst*inst=*i;//遍历指令
			Operator op=inst->getOp();//获取操作符
			if(op==OP_DEC){
				Var*arg1=inst->getArg1();//获取操作数1
				if(!arg1->live)inst->isDead=true;//变量不是活跃的了，DEC命令为死代码
			}
		}
		return ;
	}

	// for (list<InterInst*>::iterator i = optCode.begin(); i != optCode.end(); ++i)
	// {
	// 	if(!(*i)->isDead)(*i)->toString();
	// }

	stop=true;//希望停止
	analyse();
	
	// for(int i=0;i<varList.size();i++)
	// 	printf("%s ",varList[i]->getName().c_str());
	// printf("\n");
	
	//清洗一次代码
	for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
		InterInst*inst=*i;//遍历指令
		if(inst->isDead)continue;//不对死代码重复处理
		Var*rs=inst->getResult();//获取结果
		Operator op=inst->getOp();//获取操作符
		Var*arg1=inst->getArg1();//获取操作数1
		Var*arg2=inst->getArg2();//获取操作数2
		if(op>=OP_AS&&op<=OP_LEA||op==OP_GET){//定值变量运算
			if(rs->getPath().size()==1)continue;//全局变量不作处理
			//出口不活跃或者是a=a;的形式《来源于复写传播的处理结果》
			if(!inst->liveInfo.out.get(rs->index)
				||op==OP_AS&&rs==arg1){
				inst->isDead=true;
				stop=false;//识别新的死代码
			}
		}
		// else if(op==OP_DEC){
		// 	//出口不活跃
		// 	if(!inst->liveInfo.out.get(arg1->index))
		// 		inst->isDead=true;
		// 		stop=true;//识别新的死代码
		// }
		// else if(op==OP_SET){//*arg1=result
		// 	//不是死代码
		// }
		// else if(op==OP_RETV){//return arg1
		// 	//不是死代码
		// }
		// else if(op==OP_ARG){//arg arg1
		// 	//不是死代码
		// }
		// else if(op==OP_PROC){
		// 	//不是死代码
		// }
		else if(op==OP_CALL){
			//处理函数返回值,若函数返回值不活跃，则替换为函数CALL指令
			if(!inst->liveInfo.out.get(rs->index)){
				inst->callToProc();//将call指令替换为proc指令
			}
		}
	}
	elimateDeadCode(stop);
}
