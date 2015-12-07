#include "constprop.h"
#include "dfg.h"
#include "symbol.h"
#include "token.h"
#include "symtab.h"

/*
	数据流分析初始化
*/
ConstPropagation::ConstPropagation(DFG*g,SymTab*t,vector<Var*>&paraVar):dfg(g),tab(t)
{
	//函数外变量：所有全局变量，函数参数变量
	vector<Var*>glbVars=tab->getGlbVars();//全局变量
	int index=0;
	//添加全局变量
	for(unsigned int i=0;i<glbVars.size();++i){
		Var*var=glbVars[i];//获取全局变量
		var->index=index++;//记录变量在列表内的位置
		vars.push_back(var);//将被赋值的变量加入值集合
		//不存在浮点变量，因此可以用特殊浮点数区分UNDEF与NAC
		double val=UNDEF;//默认变量都是UNDEF
		if(!var->isBase())val=NAC;//非基本类型
		else if(!var->unInit())val=var->getVal();//初始化的基本类型变量为const
		boundVals.push_back(val);//添加边界值
	}
	//参数变量,初始值应该是NAC！！！
	//（理论上参数应该是NAC，不过由于参数变量不影响交汇运算初始化为UNDEF也可以）错误！！！
	for(unsigned int i=0;i<paraVar.size();++i){
		Var*var=paraVar[i];//获取全局变量
		var->index=index++;//记录变量在列表内的位置
		vars.push_back(var);//将被赋值的变量加入值集合
		boundVals.push_back(NAC);//添加边界值
	}
	for(unsigned int i=0;i<dfg->codeList.size();++i){
		if(dfg->codeList[i]->isDec()){//局部变量DEC声明！
			Var*var=dfg->codeList[i]->getArg1();//获取声明的局部变量
			var->index=index++;//记录变量在列表内的位置
			vars.push_back(var);//将被赋值的变量加入值集合
			//不存在浮点变量，因此可以用特殊浮点数区分UNDEF与NAC
			double val=UNDEF;//默认变量都是UNDEF
			if(!var->isBase())val=NAC;//非基本类型
			else if(!var->unInit())val=var->getVal();;//初始化的基本类型变量为const
			boundVals.push_back(val);//添加边界值
		}
	}
	while(index--)initVals.push_back(UNDEF);//添加初始值,初始化index（变量个数）个UNDEF
}

/*
	元素交汇运算规则
*/
double ConstPropagation::join(double left,double right){
	if(left==NAC||right==NAC)return NAC;//与NAC交并结果必是NAC
	else if(left==UNDEF)return right;//与UNDEF交并结果不变
	else if(right==UNDEF)return left;
	else if(left==right)return left;//同值元素交并结果相同
	else return NAC;//异值元素交并结果是NAC
}

/*
	集合交汇运算:B.in= & B.prev[i].out 
*/
void ConstPropagation::join(Block*block)
{
	list<Block*>& prevs=block->prevs;//B.prev集合
	vector<double>& in=block->inVals;//B.in集合
	int prevCount=prevs.size();//前驱个数
	//for之前的判断是优化计算 ，可以删除
	if(prevCount==1){in=prevs.front()->outVals;return;}//唯一前驱，取出前驱集合初始化
	if(prevCount==0){in=initVals;return;}//没有前驱，直接初始化in集合
	for(unsigned int i=0;i<in.size();++i){//多个前驱，处理in集合每个元素
		double val=UNDEF;//记录交并结果
		for (list<Block*>::iterator j=prevs.begin();j!=prevs.end();++j){//处理每一个前驱基本块prev[j]
			val=join(val,(*j)->outVals[i]);//取出prev[j].out的每个元素与结果交并
		}
		in[i]=val;//更新in[i]
	}
}

/*
	单指令传递函数：out=f(in);
*/
void ConstPropagation::translate(InterInst*inst,vector<double>& in,vector<double>& out)
{
	out=in;//默认信息直接传递
	Operator op=inst->getOp();//获取运算符
	Var*result=inst->getResult();//结果变量
	Var*arg1=inst->getArg1();//参数1
	Var*arg2=inst->getArg2();//参数2
	if(inst->isExpr()){//基本运算表达式x=?，计算新发现的传播值
		double tmp;//存储临时值结果
		if(op==OP_AS||op==OP_NEG||op==OP_NOT){//一元运算x=y x=-y x=!y 
			if(arg1->index==-1){//参数不在值列表，必是常量
				if(arg1->isBase())tmp=arg1->getVal();//排除字符串类型常量
			}else tmp=in[arg1->index];//获取in集合值
			if(tmp!=UNDEF&&tmp!=NAC){//处理常量
				if(op==OP_NEG)tmp=-tmp;
				else if(op==OP_NOT)tmp=!tmp;
			}
		}
		else if(op>=OP_ADD&&op<=OP_OR){//二元运算x=y+z
			double lp,rp;//左右操作数
			if(arg1->index==-1){//参数不在值列表，必是常量
				if(arg1->isBase())lp=arg1->getVal();
			}else lp=in[arg1->index];//左操作数
			if(arg2->index==-1){//参数不在值列表，必是常量
				if(arg2->isBase())rp=arg2->getVal();//初始化的基本类型变量为const
			}else rp=in[arg2->index];//右操作数
			if(lp==NAC||rp==NAC)tmp=NAC;//有一个NAC结果必是NAC
			else if(lp==UNDEF||rp==UNDEF)tmp=UNDEF;//都不是NAC，有一个是UNDEF，结果UNDEF
			else{//都是常量，可以计算
				int left=lp,right=rp;
				if(op==OP_ADD)tmp=left+right;
				else if(op==OP_SUB)tmp=left-right;
				else if(op==OP_MUL)tmp=left*right;
				else if(op==OP_DIV){if(!right)tmp=NAC;else tmp=left/right;}//除数为0,特殊处理！
				else if(op==OP_MOD){if(!right)tmp=NAC;else tmp=left%right;}
				else if(op==OP_GT)tmp=left>right;
				else if(op==OP_GE)tmp=left>=right;
				else if(op==OP_LT)tmp=left<right;
				else if(op==OP_LE)tmp=left<=right;
				else if(op==OP_EQU)tmp=left==right;
				else if(op==OP_NE)tmp=left!=right;
				else if(op==OP_AND)tmp=left&&right;
				else if(op==OP_OR)tmp=left||right;
			}
		}
		else if(op==OP_GET){//破坏运算x=*y
			tmp=NAC;
		}
		out[result->index]=tmp;//更新out集合值
	}
	else if(op==OP_SET || op==OP_ARG && !arg1->isBase()){
		//破坏运算*x=y 或者 arg x没有影响，arg p->p是指针，破坏！！！
		for(unsigned int i=0;i<out.size();++i)out[i]=NAC;//out全部置为NAC
	}
	else if(op==OP_PROC){//破坏运算call f()
		for(unsigned int i=0;i<glbVars.size();++i)out[glbVars[i]->index]=NAC;//全局变量全部置为NAC
	}
	else if(op==OP_CALL){//破坏运算call f()
		for(unsigned int i=0;i<glbVars.size();++i)out[glbVars[i]->index]=NAC;//全局变量全部置为NAC
		out[result->index]=NAC;//函数返回值失去常量性质！——保守预测
	}
	//拷贝信息指令的in，out信息，方便代数化简，条件跳转优化和不可达代码消除
	inst->inVals=in;
	inst->outVals=out;
}

/*
	基本块传递函数:B.out=fB(B.in);
	out集合变化返回true
*/
bool ConstPropagation::translate(Block*block)
{
	vector<double>in=block->inVals;//输入集合
	vector<double>out=block->outVals;//输出集合，默认没有指令，直接传递
	for(list<InterInst*>::iterator i=block->insts.begin();i!=block->insts.end();++i){//处理基本块的每一个指令
		InterInst*inst=*i;//取出指令
		translate(inst,in,out);//单指令传递
		in=out;//下条指令的in为当前指令的out
	}
	bool flag=false;//是否变化
	for(unsigned int i=0;i<out.size();++i){
		if(block->outVals[i]!=out[i]){
			flag=true;//检测到变化
			break;
		}
	}
	block->outVals=out;//设定out集合
	return flag;
}

void ConstPropagation::analyse()
{
	dfg->blocks[0]->outVals=boundVals;//初始化边界集合Entry.out
	for(unsigned int i=1;i<dfg->blocks.size();++i){
		dfg->blocks[i]->outVals=initVals;//初始化基本块B.out
		dfg->blocks[i]->inVals=initVals;//初始化基本块B.in
	}
	bool outChange=true;
	while(outChange){//B.out集合发生变化
		outChange=false;//重新设定变化标记
		for(unsigned int i=1;i<dfg->blocks.size();++i){//任意B!=Entry
			join(dfg->blocks[i]);//交汇运算
			if(translate(dfg->blocks[i]))//传递函数执行时比较前后out集合是否有差别
				outChange=true;
		}
	}
}

/*
	执行常量传播
*/
void ConstPropagation::propagate()
{
	analyse();//常量传播分析
	algebraSimplify();//代数化简
	condJmpOpt();//条件跳转优化，不可达代码消除
}

/*
	代数化简算法，合并常量，利用代数公式
*/
void ConstPropagation::algebraSimplify()
{
	for (unsigned int j = 0; j < dfg->blocks.size(); ++j)
	{
		for(list<InterInst*>::iterator i=dfg->blocks[j]->insts.begin();
				i!=dfg->blocks[j]->insts.end();++i){//遍历可达块的所有指令
			InterInst*inst=*i;
			Operator op=inst->getOp();//获取运算符
			if(inst->isExpr()){//处理每一个可能常量传播的指令
				double rs;//存储临时值结果
				Var*result=inst->getResult();//结果变量
				Var*arg1=inst->getArg1();//参数1
				Var*arg2=inst->getArg2();//参数2
				rs=inst->outVals[result->index];//结果变量常量传播结果
				if(rs!=UNDEF&&rs!=NAC){//结果为常量，替换为result=c指令
					// if(op==OP_AS||op==OP_NEG||op==OP_NOT){//一元运算时处理参数1
					// 	if(arg1->index==-1)delete arg1;//参数1不在值列表，必是常量,删除！
					// }
					// else if(op>=OP_ADD&&op<=OP_OR){//二元运算时还要处理参数2
					// 	if(arg1->index==-1)delete arg1;//参数1不在值列表，必是常量,删除！
					// 	if(arg2->index==-1)delete arg2;//参数2不在值列表，必是常量,删除！
					// }
					Var*newVar=new Var((int)rs);//计算的表达式结果
					tab->addVar(newVar);//添加到符号表
					inst->replace(OP_AS,result,newVar);//替换新的操作符与常量操作数
				}
				else if(op>=OP_ADD&&op<=OP_OR&&!(op==OP_AS||op==OP_NEG||op==OP_NOT)){
					//常量传播不能处理的情况由代数化简完成
					double lp,rp;//左右操作数
					if(arg1->index==-1){//参数不在值列表，必是常量
						if(arg1->isBase())lp=arg1->getVal();
					}else lp=inst->inVals[arg1->index];//左操作数
					if(arg2->index==-1){//参数不在值列表，必是常量
						if(arg2->isBase())rp=arg2->getVal();
					}else rp=inst->inVals[arg2->index];//右操作数
					int left,right;//记录操作数值
					bool dol=false,dor=false;//处理哪一个操作数
					if(lp!=UNDEF&&lp!=NAC){left=lp;dol=true;}
					else if(rp!=UNDEF&&rp!=NAC){right=rp;dor=true;}
					else continue;//都不是常量不进行处理！
					Var* newArg1=NULL;//记录有效的操作数
					Var* newArg2=NULL;//可选的操作数
					Operator newOp=OP_AS;//化简成功后默认为赋值运算
					if(op==OP_ADD){//z=0+y z=x+0
						if(dol&&left==0){/*if(arg1->index==-1)delete arg1;*/newArg1=arg2;}
						if(dor&&right==0){newArg1=arg1;/*if(arg2->index==-1)delete arg2;*/}
					}
					else if(op==OP_SUB){//z=0-y z=x-0
						if(dol&&left==0){/*if(arg1->index==-1)delete arg1;*/newOp=OP_NEG;newArg1=arg2;}
						if(dor&&right==0){newArg1=arg1;/*if(arg2->index==-1)delete arg2;*/}
					}
					else if(op==OP_MUL){//z=0*y z=x*0 z=1*y z=x*1
						if(dol&&left==0||dor&&right==0){
							newArg1=new Var(0);
							// if(arg1->index==-1)delete arg1;
							// if(arg2->index==-1)delete arg2;
						}					
						if(dol&&left==1){/*if(arg1->index==-1)delete arg1;*/newArg1=arg2;}
						if(dor&&right==1){newArg1=arg1;/*if(arg2->index==-1)delete arg2;*/}
					}
					else if(op==OP_DIV){//z=0/y z=x/1
						if(dol&&left==0){/*if(arg1->index==-1)delete arg1;*/newArg1=SymTab::zero;}
						if(dor&&right==1){newArg1=arg1;/*if(arg2->index==-1)delete arg2;*/}
					}
					else if(op==OP_MOD){//z=0%y z=x%1
						if(dol&&left==0||dor&&right==1){
							newArg1=SymTab::zero;
							// if(arg1->index==-1)delete arg1;
							// if(arg2->index==-1)delete arg2;
						}
					}
					else if(op==OP_AND){//z=0&&y z=x&&0 z=1&&y z=x&&1
						if(dol&&left==0||dor&&right==0){
							newArg1=SymTab::zero;
							// if(arg1->index==-1)delete arg1;
							// if(arg2->index==-1)delete arg2;
						}
						if(dol&&left!=0){//z=y!=0
							// if(arg1->index==-1)delete arg1;
							newOp=OP_NE;
							newArg1=arg2;
							newArg2=SymTab::zero;
						}
						if(dor&&right!=0){//z=x!=0
							newOp=OP_NE;
							newArg1=arg1;
							newArg2=SymTab::zero;
							// if(arg2->index==-1)delete arg2;
						}
					}
					else if(op==OP_OR){//z=0||y z=x||0 z=1||y z=x||1
						if(dol&&left!=0||dor&&right!=0){
							newArg1=SymTab::one;
							// if(arg1->index==-1)delete arg1;
							// if(arg2->index==-1)delete arg2;
						}
						if(dol&&left==0){//z=y!=0
							// if(arg1->index==-1)delete arg1;
							newOp=OP_NE;
							newArg1=arg2;
							newArg2=SymTab::zero;						
						}
						if(dor&&right==0){//z=x!=0
							newOp=OP_NE;
							newArg1=arg1;
							newArg2=SymTab::zero;
							// if(arg2->index==-1)delete arg2;
						}
					}
					if(newArg1){//代数化简成功！
						inst->replace(newOp,result,newArg1,newArg2);//更换新的指令
					}
					else{//没法化简，正常传播
						if(dol){//传播左操作数
							newArg1=new Var(left);
							tab->addVar(newArg1);
							newArg2=arg2;
						}
						else if(dor){//传播右操作数
							newArg2=new Var(right);
							tab->addVar(newArg2);
							newArg1=arg1;
						}
						inst->replace(op,result,newArg1,newArg2);//更换传播的新的指令
					}
				}
			}
			else if(op==OP_ARG||op==OP_RETV)
			{
				Var*arg1=inst->getArg1();//参数1
				if(arg1->index!=-1){//不是常数
					double rs=inst->outVals[arg1->index];//结果变量常量传播结果
					if(rs!=UNDEF&&rs!=NAC){//为常量
						Var*newVar=new Var((int)rs);//计算的表达式结果
						tab->addVar(newVar);//添加到符号表
						inst->setArg1(newVar);//替换新的操作符与常量操作数
					}
				}
			}
		}
	}
}

/*
	条件跳转优化，同时进行不可达代码消除
	此时只需要处理JT JF 两种情况，Jcond指令会在短路规则优化内产生，不影响表达式常量条件的处理
*/
void ConstPropagation::condJmpOpt()
{
	//dfg->toString();
	for (unsigned int j = 0; j < dfg->blocks.size(); ++j)
	{
		/*if(!dfg->reachable(dfg->blocks[j])){//不可达
			dfg->release(dfg->blocks[j]);//不可达块消除后继
			//dfg->blocks[j]->toString();
			continue;//不进行优化
		}*/
		//dfg->blocks[j]->toString();
		for(list<InterInst*>::iterator i=dfg->blocks[j]->insts.begin(),k=i;
			i!=dfg->blocks[j]->insts.end();i=k){//遍历可达块的所有指令
			++k;//记录下一个迭代器，防止遍历时发生指令删除
			InterInst*inst=*i;
			if(inst->isJcond()){//处理每一个可能常量传播的条件跳转指令，无条件跳转指令不用关心
				Operator op=inst->getOp();//获取运算符
				InterInst*tar=inst->getTarget();//目标位置
				Var*arg1=inst->getArg1();//参数1
				double cond;//操作数
				if(arg1->index==-1){//参数不在值列表，必是常量
					if(arg1->isBase())cond=arg1->getVal();
				}else cond=inst->inVals[arg1->index];//条件值
				if(cond==NAC||cond==UNDEF)continue;//非常量条件不做处理
				if(op==OP_JT){//条件真跳转
					if(cond==0){//条件不满足
						inst->block->insts.remove(inst);//将该指令从基本块内删除
						if(dfg->blocks[j+1]!=tar->block)//目标块不是紧跟块
							dfg->delLink(inst->block,tar->block);//从DFG内递归解除指令所在块到目标块的关联
					}
					else{
						inst->replace(OP_JMP,tar);//无条件跳转指令
						if(dfg->blocks[j+1]!=tar->block)//目标块不是紧跟块
							dfg->delLink(inst->block,dfg->blocks[j+1]);//从DFG内递归解除指令所在块到紧跟块的关联
					}
				}
				else if(op==OP_JF){//条件假跳转
					if(cond==0){//条件满足
						inst->replace(OP_JMP,tar);//无条件跳转指令
						if(dfg->blocks[j+1]!=tar->block)//目标块不是紧跟块
							dfg->delLink(inst->block,dfg->blocks[j+1]);//从DFG内递归解除指令所在块到紧跟块的关联
					}
					else{
						inst->block->insts.remove(inst);//将该指令从基本块内删除
						if(dfg->blocks[j+1]!=tar->block)//目标块不是紧跟块
							dfg->delLink(inst->block,tar->block);//从DFG内递归解除指令所在块到目标块的关联
					}
				}
			}
		}
	}	
}
