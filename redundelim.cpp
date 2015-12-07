#include <algorithm>
#include "redundelim.h"
#include "dfg.h"
#include "intercode.h"
#include "symbol.h"
#include "symtab.h"

/*******************************************************************************
                                   表达式对象
*******************************************************************************/
/*
	构造函数
*/
Expr::Expr(Var*r,Operator o,Var*a1,Var*a2)
	:rs(r),newRs(NULL),op(o),arg1(a1),arg2(a2),index(-1)
{}

/*
	表达式相同
*/
bool Expr::operator==(Expr&e)
{
	return op==e.op&&arg1==e.arg1&&arg2==e.arg2;
}

/*
	产生返回结果
*/
Var* Expr::genResult(SymTab*tab)
{
	if(newRs)return newRs;//不重复产生
	vector<int> glbPath;//函数内第一作用域
	glbPath.push_back(0);//全局
	glbPath.push_back(rs->getPath()[1]);//函数内
	newRs=new Var(glbPath,rs);//产生新的变量，作用域为函数第一作用域
	tab->addVar(newRs);//添加到符号表
}

/*
	获取表达式的临时结果变量
*/
Var* Expr::getNewRs()
{
	return newRs;
}


/*
	产生插入指令
*/
InterInst* Expr::genInst(SymTab*tab,DFG*dfg)
{
	genResult(tab);//创建返回变量
	InterInst*inst=new InterInst(op,newRs,arg1,arg2);//创建指令
	dfg->newCode.push_back(inst);//保存到新代码列表内
	return inst;
}

/*
	产生声明指令
*/
InterInst* Expr::genDec(DFG*dfg)
{
	if(!newRs)return NULL;//没有计算插入点
	InterInst*inst=new InterInst(OP_DEC,newRs);//创建指令
	dfg->newCode.push_back(inst);
	return inst;
}


/*
	表达式使用了变量v
*/
bool Expr::use(Var*v)
{
	return arg1==v||arg2==v;
}

/*******************************************************************************
                                   局部冗余消除
*******************************************************************************/
/*
	查询表达式位置
*/
int RedundElim::findExpr(Expr&e)
{
	for(unsigned int i=0;i<exprList.size();++i)
		if(exprList[i]==e)return i;
	return -1;
}

/*
	构造
*/
RedundElim::RedundElim(DFG*g,SymTab*tab):dfg(g),symtab(tab)
{
	dfg->toCode(optCode);//获取上阶段优化后的代码
	int j=0;
	//统计所有的表达式
	for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
		InterInst*inst=*i;//遍历指令
		Var* rs=inst->getResult();//获取结果
		Operator op=inst->getOp();//获取操作符
		Var*arg1=inst->getArg1();//获取操作数1
		Var*arg2=inst->getArg2();//获取操作数2
		if(op>=OP_ADD&&op<=OP_GET&&op!=OP_SET){//所有的可能使用的表达式
			Expr tmp(rs,op,arg1,arg2);//创建表达式对象
			if(findExpr(tmp)!=-1)continue;//表达式已经存在
			tmp.index=j++;//记录索引
			exprList.push_back(tmp);//保存到表达式列表
		}
	}
	U.init(exprList.size(),1);//初始化基本集合
	E.init(exprList.size(),0);
	//初始化指令的指令的e_use和e_kill集合
	for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
		InterInst*inst=*i;//遍历指令
		inst->e_use=E;//初始化使用集合与杀死集合
		inst->e_kill=E;
		Var*rs=inst->getResult();//获取结果
		Operator op=inst->getOp();//获取操作符
		Var*arg1=inst->getArg1();//获取操作数1
		Var*arg2=inst->getArg2();//获取操作数2
		if(op>=OP_AS&&op<=OP_GET&&op!=OP_SET){//所有的可能使用的表达式
			if(op==OP_AS){//赋值语句特殊处理
				for(unsigned int i=0;i<exprList.size();++i){
					if(exprList[i].use(rs))inst->e_kill.set(i);//设定杀死集合
				}
				continue;
			}
			Expr tmp(rs,op,arg1,arg2);
			int index=findExpr(tmp);//表达式必存在
			inst->e_use.set(index);//设定使用集合
			for(unsigned int i=0;i<exprList.size();++i){
				if(exprList[i].use(rs))inst->e_kill.set(i);//设定杀死集合
			}
		}
		else if(inst->unknown())//不可预测操作
			inst->e_kill=U;//杀死全部表达式
	}
	//初始化基本块索引，用于分析支配节点
	for(unsigned int i=0;i<dfg->blocks.size();++i){
		dfg->blocks[i]->info.index=i;
	}
	BE.init(dfg->blocks.size(),0);//初始化块基本集合
	BU.init(dfg->blocks.size(),1);
}

/*
	预期执行表达式传递函数:f(x)=e_use(B) & (x-e_kill(B))
*/
bool RedundElim::translate_anticipated(Block*block)
{
	Set tmp=block->info.anticipated.out;//记录B.out作为最后一个指令的输入
	//逆序处理基本块的每一个指令
	for(list<InterInst*>::reverse_iterator i=block->insts.rbegin();i!=block->insts.rend();++i){
		InterInst*inst=*i;//取出指令
		Set& in=inst->info.anticipated.in;//指令的in
		Set& out=inst->info.anticipated.out;//指令的out
		out=tmp;//设置指令的out
		// if(inst->e_kill.empty())
		// {
		// 	int i=0;
		// 	i++;
		// }
		in=inst->e_use | (out-inst->e_kill);//指令级传递函数in=f(out)
		tmp=in;//用当前指令的in设置下次使用的out
	}
	bool flag=tmp!=block->info.anticipated.in;//是否变化
	block->info.anticipated.in=tmp;//设定B.in集合
	return flag;
}

/*
	预期执行表达式数据流分析函数
	direct : 数据流方向,逆向
	init : 初始化集合,B.in=U
	bound : 边界集合，Exit.in=E
	join : 交汇运算，交集
	translate : 传递函数,f(x)=e_use(B) & (x-e_kill(B))
*/
void RedundElim::analyse_anticipated()
{
	/*
		Exit特殊初始化
		由于在分析anticipated前向数据流问题，因此Exit节点的out集合不能得到计算，
		而后期计算latest[Exit]时，需要earliest[Exit]，间接使用了anticipated[Exit],
		故初始化这个集合为空！
	*/
	dfg->blocks[dfg->blocks.size()-1]->info.anticipated.out=E;// 初始化
	translate_anticipated(dfg->blocks[dfg->blocks.size()-1]);//将信息传递到指令上
	//正常的数据流分析
	dfg->blocks[dfg->blocks.size()-1]->info.anticipated.in=E;//初始化边界集合Exit.in=E
	for(unsigned int i=0;i<dfg->blocks.size()-1;++i){
		dfg->blocks[i]->info.anticipated.in=U;//初始化其他基本块B.in=U
	}
	bool change=true;//集合变化标记
	while(change){//B.in集合发生变化
		change=false;//重新设定变化标记
		for(int i=dfg->blocks.size()-2;i>=0;--i){//逆序，任意B!=Exit
			if(!dfg->blocks[i]->canReach)continue;//块不可达不处理
			Set tmp=U;//保存交汇运算结果
			for(list<Block*>::iterator j=dfg->blocks[i]->succs.begin();
				j!=dfg->blocks[i]->succs.end();++j){//succ[i]
				tmp=tmp & (*j)->info.anticipated.in;
			}
			//B.out=& succ[j].in
			dfg->blocks[i]->info.anticipated.out=tmp;
			if(translate_anticipated(dfg->blocks[i]))//传递函数执行时比较前后集合是否有差别
				change=true;
		}
	}
// 	/*
// 		调试输出
// 	*/
// 	for(unsigned int i=0;i<dfg->blocks.size();++i){
// 		printf("%d  ", i);
// 		dfg->blocks[i]->info.anticipated.in.p();
// 		printf("  ");
// 		dfg->blocks[i]->info.anticipated.out.p();
// 		printf("\n");
// 	}
// 	for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
// 		InterInst*inst=*i;
// 		if(inst->info.anticipated.in!=E){
// 			inst->toString();
// 		}
// 	}
}

/*
	可用表达式传递函数：f(x)=(anticipated(B).in | x)-e_kill(B)
*/
bool RedundElim::translate_available(Block*block)
{
	Set tmp=block->info.available.in;//记录B.in作为第一个指令的输入
	//正序处理基本块的每一个指令
	for(list<InterInst*>::iterator i=block->insts.begin();i!=block->insts.end();++i){
		InterInst*inst=*i;//取出指令
		Set& in=inst->info.available.in;//指令的in
		Set& out=inst->info.available.out;//指令的out
		in=tmp;//设置指令的in
		out=(inst->info.anticipated.in | in)-inst->e_kill;//指令级传递函数out=f(in)
		tmp=out;//用当前指令的out设置下次使用的in
	}
	bool flag=tmp!=block->info.available.out;//是否变化
	block->info.available.out=tmp;//设定B.out集合
	return flag;
}

/*
	可用表达式数据流分析函数
	direct : 数据流方向,前向
	init : 初始化集合,B.out=U
	bound : 边界集合，Entry.out=E
	join : 交汇运算，交集
	translate : 传递函数,f(x)=(anticipated(B).in | x)-e_kill(B)
*/
void RedundElim::analyse_available()
{
	/*
		Entry特殊初始化
		由于在分析available前向数据流问题，因此Entry节点的in集合不能得到计算，
		而分析used集合时，需要earliset[Entry]=anticipated[Entry]-available[Entry]，
		故初始化这个集合为空！
	*/
	dfg->blocks[0]->info.available.in=E;//初始化in集合
	translate_available(dfg->blocks[0]);//将信息传递到指令上
	//正常的数据流分析
	dfg->blocks[0]->info.available.out=E;//初始化边界集合Entry.out=E
	for(unsigned int i=1;i<dfg->blocks.size();++i){
		dfg->blocks[i]->info.available.out=U;//初始化基本块B.out=U
	}	
	bool change=true;//集合变化标记
	while(change){//B.out集合发生变化
		change=false;//重新设定变化标记
		for(unsigned int i=1;i<dfg->blocks.size();++i){//任意B!=Entry
			if(!dfg->blocks[i]->canReach)continue;//块不可达不处理
			Set tmp=U;//保存交汇运算结果
			for(list<Block*>::iterator j=dfg->blocks[i]->prevs.begin();
				j!=dfg->blocks[i]->prevs.end();++j){//prev[i]
				tmp=tmp & (*j)->info.available.out;
			}
			//B.in=& prev[j].out
			dfg->blocks[i]->info.available.in=tmp;
			if(translate_available(dfg->blocks[i]))//传递函数执行时比较前后集合是否有差别
				change=true;
		}
	}
// 	/*
// 		调试输出
// 	*/
// 	for(unsigned int i=0;i<dfg->blocks.size();++i){
// 		printf("%d  ", i);
// 		dfg->blocks[i]->info.available.in.p();
// 		printf("  ");
// 		dfg->blocks[i]->info.available.out.p();
// 		printf("\n");
// 	}
// 	for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
// 		InterInst*inst=*i;
// 		if(inst->info.available.in!=E){
// 			inst->toString();
// 		}
// 	}
}

/*
	可后延表达式传递函数：f(x)=(earliest(B) | x)-e_use(B)
		其中，earliest(B)=anticipated(B).in-available(B).in
*/
bool RedundElim::translate_postponable(Block*block)
{
	Set tmp=block->info.postponable.in;//记录B.in作为第一个指令的输入
	//正序处理基本块的每一个指令
	for(list<InterInst*>::iterator i=block->insts.begin();i!=block->insts.end();++i){
		InterInst*inst=*i;//取出指令
		Set& in=inst->info.postponable.in;//指令的in
		Set& out=inst->info.postponable.out;//指令的out
		in=tmp;//设置指令的in
		//计算earliest
		inst->info.earliest=inst->info.anticipated.in-inst->info.available.in;
		out=(inst->info.earliest | in)-inst->e_use;//指令级传递函数out=f(in)
		tmp=out;//用当前指令的out设置下次使用的in
	}
	bool flag=tmp!=block->info.postponable.out;//是否变化
	block->info.postponable.out=tmp;//设定B.out集合
	block->info.earliest=block->info.anticipated.in-block->info.available.in;//计算块的earliest
	return flag;
}

/*
	可后延表达式数据流分析函数
	direct : 数据流方向,前向
	init : 初始化集合,B.out=U
	bound : 边界集合，Entry.out=E
	join : 交汇运算，交集
	translate : 传递函数,f(x)=(earliest(B) | x)-e_use(B)
		其中，earliest(B)=anticipated(B).in-available(B).in
*/
void RedundElim::analyse_postponable()
{
	/*
		Entry特殊初始化
		由于在分析postponable前向数据流问题，因此Entry节点的in集合不能得到计算，
		而分析used集合时，需要postponable[Entry],故初始化这个集合为空！
	*/
	dfg->blocks[0]->info.postponable.in=E;// 初始化
	translate_postponable(dfg->blocks[0]);//将信息传递到指令上
	//正常的数据流分析
	dfg->blocks[0]->info.postponable.out=E;//初始化边界集合Entry.out=E
	for(unsigned int i=1;i<dfg->blocks.size();++i){
		dfg->blocks[i]->info.postponable.out=U;//初始化基本块B.out=U
	}	
	bool change=true;//集合变化标记
	while(change){//B.out集合发生变化
		change=false;//重新设定变化标记
		for(unsigned int i=1;i<dfg->blocks.size();++i){//任意B!=Entry
			if(!dfg->blocks[i]->canReach)continue;//块不可达不处理
			Set tmp=U;//保存交汇运算结果
			for(list<Block*>::iterator j=dfg->blocks[i]->prevs.begin();
				j!=dfg->blocks[i]->prevs.end();++j){//prev[i]
				tmp=tmp & (*j)->info.postponable.out;
			}
			//B.in=& prev[j].out
			dfg->blocks[i]->info.postponable.in=tmp;
			if(translate_postponable(dfg->blocks[i]))//传递函数执行时比较前后集合是否有差别
				change=true;
		}
	}
// 	/*
// 		调试输出
// 	*/
// 	for(unsigned int i=0;i<dfg->blocks.size();++i){
// 		printf("%d  ", i);
// 		dfg->blocks[i]->info.postponable.in.p();
// 		printf("  ");
// 		dfg->blocks[i]->info.postponable.out.p();
// 		printf("\n");
// 	}
// 	for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
// 		InterInst*inst=*i;
// 		if(inst->info.postponable.in!=E){
// 			inst->toString();
// 		}
// 	}
}

/*
	被使用表达式传递函数:f(x)=(e_use(B) | x)-latest(B)
		其中，latest(B)=(earliest(B) | postponable(B).in) &
			(e_use(B) | ~(&( earliest(B.succ[i]) | postponable(B.succ[i]).in )))
*/
bool RedundElim::translate_used(Block*block)
{
	Set tmp=block->info.used.out;//记录B.out作为最后一个指令的输入
	//记录后继的交集集合&( earliest(B.succ[i]) | postponable(B.succ[i]).in )
	//如果不是最后一条指令，则上式简化为earliest(B.succ[i]) | postponable(B.succ[i]).in
	Set succJoin=U;
	for(list<Block*>::iterator i=block->succs.begin();i!=block->succs.end();++i){
		succJoin=succJoin & ((*i)->info.earliest | (*i)->info.postponable.in);
	}
	//逆序处理基本块的每一个指令
	for(list<InterInst*>::reverse_iterator i=block->insts.rbegin();i!=block->insts.rend();++i){
		InterInst*inst=*i;//取出指令
		Set& in=inst->info.used.in;//指令的in
		Set& out=inst->info.used.out;//指令的out
		out=tmp;//设置指令的out
		//计算latest
		inst->info.latest=(inst->info.earliest | inst->info.postponable.in) &
			(inst->e_use | ~succJoin);
		in=(inst->e_use | out)-inst->info.latest;//指令级传递函数in=f(out)
		//记录succJoin=earliest(B) | postponable(B).in，作为下次指令的后继交集集合
		succJoin=inst->info.earliest | inst->info.postponable.in;
		tmp=in;//用当前指令的in设置下次使用的out
	}
	bool flag=tmp!=block->info.used.in;//是否变化
	block->info.used.in=tmp;//设定B.in集合
	return flag;
}

/*
	预期执行表达式数据流分析函数
	direct : 数据流方向,逆向
	init : 初始化集合,B.in=E
	bound : 边界集合，Exit.in=E
	join : 交汇运算，交集
	translate : 传递函数,f(x)=(e_use(B) | x)-latest(B)
		其中，latest(B)=(earliest(B) | postponable(B).in) &
			(e_use(B) | ~(&( earliest(B.succ[i]) | postponable(B.succ[i]).in )))
*/
void RedundElim::analyse_used()
{
	/*
		Exit特殊初始化
		由于在分析used前向数据流问题，因此Exit节点的out集合不能得到计算，
		而计算表达式插入点和替换点时，需要used[Exit],故初始化这个集合为空！
	*/
	dfg->blocks[dfg->blocks.size()-1]->info.used.out=E;// 初始化
	translate_used(dfg->blocks[dfg->blocks.size()-1]);//将信息传递到指令上
	//正常的数据流分析
	dfg->blocks[dfg->blocks.size()-1]->info.used.in=E;//初始化边界集合Exit.in=E
	for(unsigned int i=0;i<dfg->blocks.size()-1;++i){
		dfg->blocks[i]->info.used.in=E;//初始化基本块B.in=E
	}
	bool change=true;//集合变化标记
	while(change){//B.in集合发生变化
		change=false;//重新设定变化标记
		for(int i=dfg->blocks.size()-2;i>=0;--i){//任意B!=Exit
			if(!dfg->blocks[i]->canReach)continue;//块不可达不处理
			Set tmp=E;//保存交汇运算结果
			for(list<Block*>::iterator j=dfg->blocks[i]->succs.begin();
				j!=dfg->blocks[i]->succs.end();++j){//succ[i]
				tmp=tmp | (*j)->info.used.in;
			}
			//B.out=& succ[j].in
			dfg->blocks[i]->info.used.out=tmp;
			if(translate_used(dfg->blocks[i]))//传递函数执行时比较前后集合是否有差别
				change=true;
		}
	}
	// /*
	// 	调试输出
	// */
	// for(unsigned int i=0;i<dfg->blocks.size();++i){
	// 	printf("%d  ", i);
	// 	dfg->blocks[i]->info.used.in.p();
	// 	printf("  ");
	// 	dfg->blocks[i]->info.used.out.p();
	// 	printf("\n");
	// }
	// for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
	// 	InterInst*inst=*i;
	// 	//x+y 在 latest[B] & used[B].out，插入t = x+y
	// 	if((inst->info.latest & inst->info.used.out)!=E){
	// 		inst->toString();
	// 		printf("  t=x+y\n");
	// 	}
	// }
}

/*
	支配节点传递函数：f(x)=x | B
*/
bool RedundElim::translate_dom(Block*block)
{
	Set tmp=block->info.dom.in;//记录B.in
	tmp.set(block->info.index);//并入基本块B
	bool flag=tmp!=block->info.dom.out;//是否变化
	block->info.dom.out=tmp;//设定B.out集合
	return flag;
}

/*
	支配节点数据流分析函数
	direct : 数据流方向,前向
	init : 初始化集合,B.out=BE
	bound : 边界集合，Entry.out=Entry
	join : 交汇运算，交集
	translate : 传递函数,f(x)=x | B
*/
void RedundElim::analyse_dom()
{
	//正常的数据流分析
	dfg->blocks[0]->info.dom.out=BE;//初始化边界集合Entry.out=Entry
	dfg->blocks[0]->info.dom.out.set(dfg->blocks[0]->info.index);//加入自身
	for(unsigned int i=1;i<dfg->blocks.size();++i){
		dfg->blocks[i]->info.dom.out=BU;//初始化基本块B.out=BU
	}	
	bool change=true;//集合变化标记
	while(change){//B.out集合发生变化
		change=false;//重新设定变化标记
		for(unsigned int i=1;i<dfg->blocks.size();++i){//任意B!=Entry
			if(!dfg->blocks[i]->canReach)continue;//块不可达不处理
			Set tmp=BU;//保存交汇运算结果
			for(list<Block*>::iterator j=dfg->blocks[i]->prevs.begin();
				j!=dfg->blocks[i]->prevs.end();++j){//prev[i]
				tmp=tmp & (*j)->info.dom.out;
			}
			//B.in=& prev[j].out
			dfg->blocks[i]->info.dom.in=tmp;
			if(translate_dom(dfg->blocks[i]))//传递函数执行时比较前后集合是否有差别
				change=true;
		}
	}
	// /*
	// 	调试输出
	// */
	// for(unsigned int i=0;i<dfg->blocks.size();++i){
	// 	printf("%d  ", i);
	// 	dfg->blocks[i]->info.dom.in.p();
	// 	printf("  ");
	// 	dfg->blocks[i]->info.dom.out.p();
	// 	printf("\n");
	// }
}

/*
	冗余消除
*/
void RedundElim::elimate()
{
	//分析四个数据流问题
	analyse_anticipated();
	analyse_available();
	analyse_postponable();
	analyse_used();

	//----------------------支配节点计算声明点位置------------------------------------
	// //方法取消！因为循环内插入声明点无法使得声明外部可见！！！
	// analyse_dom();//无法准确确定声明点的位置（支配节点也不行），因此将声明插入entry！
	// //冗余删除操作
	// vector<Set> delarList(exprList.size(),BU);//存储表达式插入点支配节点集合交集
	// for(list<InterInst*>::iterator j=optCode.begin();j!=optCode.end();++j){
	// 	InterInst*inst=*j;
	// 	//x+y 在 latest[B] & used[B].out，插入t = x+y
	// 	Set insert=inst->info.latest & inst->info.used.out;//计算插入集合
	// 	if(insert==E)continue;
	// 	for(int i=0;i<exprList.size();++i){//处理所有的表达式
	// 		if(insert.get(i)){//表达式i前需要插入t=x+y
	// 			//求被插入的表达式所在基本块的前驱集合交集
	// 			delarList[i]=delarList[i] & inst->block->info.dom.out;//插入点基本块支配节点集合交集
	// 		}
	// 	}
	// }
	// //公共前驱算法——最近公共支配节点
	// vector<Block*> declarPoint(exprList.size(),NULL);//存储表达式插入点基本块
	// for(unsigned int i=0;i<declarPoint.size();++i){
	// 	int index=delarList[i].max();//求解最高位1索引
	// 	//最高位1标识最近的公共前驱，即声明点所在的基本块
	// 	declarPoint[i]=(index!=-1)?dfg->blocks[index]:NULL;
	// }
	//--------------------以上为支配节点计算和声明点计算代码-----------------------------
	//计算插入点并插入指令
	for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
		InterInst*inst=*i;
		//x+y 在 latest[B] & used[B].out，插入t = x+y
		Set insert=inst->info.latest & inst->info.used.out;//计算插入集合		
		if(insert==E)continue;
		for(int i=0;i<exprList.size();++i){//处理所有的表达式
			if(insert.get(i)){//表达式i需要插入
				//插入点
				InterInst*newInst=exprList[i].genInst(symtab,dfg);//创建插入指令
				list<InterInst*>&insts=inst->block->insts;//指令所在基本块的指令序列
				//在block->insts内，将newInst插入到inst之前
				insts.insert(find(insts.begin(),insts.end(),inst),newInst);
				// printf("插入");
				// newInst->toString();
				// printf("在");
				// inst->toString();
				// printf("之前\n");
			}
		}
	}
	//声明点全部插入到Entry块
	for(int i=0;i<exprList.size();++i){//处理所有的表达式
		InterInst*decInst=exprList[i].genDec(dfg);//创建声明指令
		if(!decInst)continue;//没有生成插入点，则不声明！
		//declarPoint[i]->insts.push_front(decInst);//插入声明点到最近支配节点块首部
		dfg->blocks[0]->insts.push_back(decInst);//插入声明点到Entry块尾部
		//dfg->blocks[1]->insts.push_front(decInst);//插入声明点到紧跟Entry块的块首部
		// printf("插入");
		// decInst->toString();
		//printf("在");
		//declarPoint[i]->toString();
	}
	for(list<InterInst*>::iterator i=optCode.begin();i!=optCode.end();++i){
		InterInst*inst=*i;

		//x+y 在 e_use(B) & (~latest[B] | used[B].out)，替换x+y <= t
		Set replace=inst->e_use & (~inst->info.latest | inst->info.used.out);//计算替换集合

		if(replace!=E){
			// printf("替换");
			// inst->toString();
			// printf(" -> ");
			inst->replace(OP_AS,inst->getResult(),exprList[replace.max()].getNewRs());
			// inst->toString();
		}

		// if((inst->e_use & (~inst->info.latest | inst->info.used.out))!=E){
		// 	Var* rs=inst->getResult();//获取结果
		// 	Operator op=inst->getOp();//获取操作符
		// 	Var*arg1=inst->getArg1();//获取操作数1
		// 	Var*arg2=inst->getArg2();//获取操作数2
		// 	if(op>=OP_ADD&&op<=OP_GET&&op!=OP_SET){//所有的可能使用的表达式
		// 		Expr tmp(rs,op,arg1,arg2);//创建表达式对象
		// 		if(findExpr(tmp)!=-1){//表达式已经存在
		// 			inst->toString();
		// 			printf("  x+y<=t\n");
		// 		}
		// 	}
		// }
	}

}