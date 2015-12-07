#include "dfg.h"
#include "intercode.h"
#include "constprop.h"

/*******************************************************************************
                                   基本块
*******************************************************************************/

/*
	构造与初始化
*/
Block::Block(vector<InterInst*>&codes):visited(false),canReach(true)
{
	for(unsigned int i=0;i<codes.size();++i){
		codes[i]->block=this;//记录指令所在的基本块
		insts.push_back(codes[i]);//转换为list
	}
}

/*
	输出基本块指令
*/
void Block::toString()
{
	printf("-----------%.8x----------\n",this);
	printf("前驱：");
	for (list<Block*>::iterator i = prevs.begin(); i != prevs.end(); ++i)
	{
		printf("%.8x ",*i);
	}
	printf("\n");
	printf("后继：");
	for (list<Block*>::iterator i = succs.begin(); i != succs.end(); ++i)
	{
		printf("%.8x ",*i);
	}
	printf("\n");
	for (list<InterInst*>::iterator i = insts.begin(); i != insts.end(); ++i)
	{
		(*i)->toString();
	}
	printf("-----------------------------\n");
}


/*******************************************************************************
                                   数据流图
*******************************************************************************/

/*
	初始化
*/
DFG::DFG(InterCode& code)
{
	code.markFirst();//标识首指令
	codeList=code.getCode();//获取代码序列
	createBlocks();//创建基本块
	linkBlocks();//链接基本块关系
}

/*
	创建基本块
*/
void DFG::createBlocks()
{
	vector<InterInst*>tmpList;//记录一个基本块的指令临时列表
	for(unsigned int i=0;i<codeList.size();++i){
		if(tmpList.empty()&&codeList[i]->isFirst()){
			tmpList.push_back(codeList[i]);//添加第一条首指令
			continue;
		}
		if(!tmpList.empty()){
			if(codeList[i]->isFirst()){//新的首指令
				blocks.push_back(new Block(tmpList));//发现添加基本块
				tmpList.clear();//清除上个临时列表
			}
			tmpList.push_back(codeList[i]);//添加新的首指令或者基本块后继的指令
		}
	}
	blocks.push_back(new Block(tmpList));//添加最后一个基本块
}

/*
	链接基本块
*/
void DFG::linkBlocks()
{
	//链接基本块顺序关系
	for (unsigned int i = 0; i < blocks.size()-1; ++i){//后继关系
		InterInst*last=blocks[i]->insts.back();//当前基本块的最后一条指令
		if(!last->isJmp())//不是直接跳转，可能顺序执行
			blocks[i]->succs.push_back(blocks[i+1]);
	}
	for (unsigned int i = 1; i < blocks.size(); ++i){//前驱关系
		InterInst*last=blocks[i-1]->insts.back();//前个基本块的最后一条指令
		if(!last->isJmp())//不是直接跳转，可能顺序执行
			blocks[i]->prevs.push_back(blocks[i-1]);
	}
	for (unsigned int i = 0; i < blocks.size(); ++i){//跳转关系
		InterInst*last=blocks[i]->insts.back();//基本块的最后一条指令
		if(last->isJmp()||last->isJcond()){//（直接/条件）跳转
			blocks[i]->succs.push_back(last->getTarget()->block);//跳转目标块为后继
			last->getTarget()->block->prevs.push_back(blocks[i]);//相反为前驱
		}
	}
}


/*
	清除所有基本块
*/
DFG::~DFG()
{
	for (unsigned int i = 0; i < blocks.size(); ++i)
	{
		delete blocks[i];
	}
	for (unsigned int i = 0; i < newCode.size(); ++i)
	{
		delete newCode[i];
	}
}


/*
	测试块是否可达
*/
bool DFG::reachable(Block*block)
{
	if(block==blocks[0])return true;//到达入口
	else if(block->visited)return false;//访问过了
	block->visited=true;//设定访问标记
	bool flag=false;//可达入口标记
	//若有前驱,测试每个前驱
	for(list<Block*>::iterator i=block->prevs.begin();i!=block->prevs.end();++i){
		Block*prev=*i;//每个前驱
		flag=reachable(prev);//递归测试
		if(flag)break;//可到达入口终止测试，否则继续测试其他前驱
	}	
	return flag;//返回标记
}

/*
	如果块不可达，则删除所有后继，并继续处理所有后继
*/
void DFG::release(Block*block)
{	
	//测试block是否还能入口点可达	
	if(!reachable(block)){//确定为不可达代码
		list<Block*> delList;
		for(list<Block*>::iterator i=block->succs.begin();i!=block->succs.end();++i){
			delList.push_back(*i);//记录所有后继
		}
		//先删除后继与当前块的关系
		for(list<Block*>::iterator i=delList.begin();i!=delList.end();++i){
			block->succs.remove(*i);
			(*i)->prevs.remove(block);
		}
		//再递归处理所有后继
		for(list<Block*>::iterator i=delList.begin();i!=delList.end();++i){
			release(*i);//递归删除end的后继
		}
	}
}


/*
	删除块间联系，如果块不可达，则删除所有后继联系
*/
void DFG::delLink(Block*begin,Block*end)
{
	resetVisit();
	//解除begin与end的关联
	if(begin){//end没有前驱的情况
		begin->succs.remove(end);
		end->prevs.remove(begin);
	}
	release(end);//递归解除关联，需要测试后继块是否可达
}

/*
	重置访问标记
*/
void DFG::resetVisit()
{
	//重置访问标记
	for (unsigned int i = 0; i < blocks.size(); ++i)
	{
		blocks[i]->visited=false;
	}
}


/*
	导出数据流图为中间代码
*/
void DFG::toCode(list<InterInst*>& opt)
{
	opt.clear();
	for (unsigned int i = 0; i < blocks.size(); ++i)
	{
		resetVisit();
		if(reachable(blocks[i])){//有效的基本块
			list<InterInst*> tmpInsts;//复制一份指令，防止意外删除
			for(list<InterInst*>::iterator it=blocks[i]->insts.begin();
				it!=blocks[i]->insts.end();++it){
				if((*it)->isDead)continue;//跳过死代码
				tmpInsts.push_back(*it);//抽取有效指令
			}
			opt.splice(opt.end(),tmpInsts);//合并有效基本快
		}
		else
			blocks[i]->canReach=false;//记录块不可达
	}
}

/*
	输出基本块
*/
void DFG::toString()
{
	for (unsigned int i = 0; i < blocks.size(); ++i)
	{
		blocks[i]->toString();
	}
}
