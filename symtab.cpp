#include "symtab.h"
#include "error.h"
#include "symbol.h"
#include "genir.h"
#include "compiler.h"

//打印语义错误
#define SEMERROR(code,name) Error::semError(code,name)

/*******************************************************************************
                                   符号表
*******************************************************************************/

Var* SymTab::voidVar=NULL;//特殊变量的初始化
Var* SymTab::zero=NULL;//特殊变量的初始化
Var* SymTab::one=NULL;//特殊变量的初始化
Var* SymTab::four=NULL;//特殊变量的初始化

/*
	初始化符号表
*/
SymTab::SymTab()
{
	/*
		此处产生特殊的常量void，1,4
	*/
	voidVar=new Var();//void变量
	zero=new Var(1);//常量0
	one=new Var(1);//常量1
	four=new Var(4);//常量4
	addVar(voidVar);//让符号表管理这些特殊变量
	addVar(one);//让符号表管理这些特殊变量
	addVar(zero);//让符号表管理这些特殊变量
	addVar(four);//让符号表管理这些特殊变量

	scopeId=0;
	curFun=NULL;
	ir=NULL;
	scopePath.push_back(0);//全局作用域	
}

/*
	清除信息
*/
SymTab::~SymTab()
{
	//清除函数,保证变量信息在指令清除时可用
	hash_map<string, Fun*, string_hash>::iterator funIt,funEnd=funTab.end();
	for(funIt=funTab.begin();funIt!=funEnd;++funIt){
		delete funIt->second;
	}
	//清除变量
	hash_map<string, vector< Var* > *, string_hash>::iterator varIt,varEnd=varTab.end();
	for(varIt=varTab.begin();varIt!=varEnd;++varIt){
		vector<Var*>&list=*varIt->second;
		for(int i=0;i<list.size();i++)
			delete list[i];
		delete &list;
	}
	//清除串
	hash_map<string, Var*, string_hash>::iterator strIt,strEnd=strTab.end();
	for(strIt=strTab.begin();strIt!=strEnd;++strIt)
		delete strIt->second;	
}

/*
	设置中间代码生成器
*/
void SymTab::setIr(GenIR*ir)
{
	this->ir=ir;
}

/*
	获取scopePath
*/
vector<int>& SymTab::getScopePath()
{
	return scopePath;
}

/*
	添加一个变量到符号表
*/
void SymTab::addVar(Var* var)
{
	if(varTab.find(var->getName())==varTab.end()){ //没有该名字的变量
		varTab[var->getName()]=new vector<Var*>;//创建链表
		varTab[var->getName()]->push_back(var);//添加变量
		varList.push_back(var->getName());
	}
	else{
		//判断同名变量是否都不在一个作用域
		vector<Var*>&list=*varTab[var->getName()];
		int i;
		for(i=0;i<list.size();i++)
			if(list[i]->getPath().back()==var->getPath().back())//在一个作用域，冲突！
				break;
		if(i==list.size()||var->getName()[0]=='<')//没有冲突
			list.push_back(var);
		else{
			//同一作用域存在同名的变量的定义，extern是声明外部文件的变量，相当于定义了该全局变量
			SEMERROR(VAR_RE_DEF,var->getName());
			delete var;
			return;//无效变量，删除，不定位
		}
	}
	if(ir){
		int flag=ir->genVarInit(var);//产生变量初始化语句,常量返回0
		if(curFun&&flag)curFun->locate(var);//计算局部变量的栈帧偏移
	}
}
/*
	添加一个字符串常量
*/
void SymTab::addStr(Var* v)
{
	strTab[v->getName()]=v;
}

/*
	获取一个变量
*/
Var* SymTab::getVar(string name)
{
	Var*select=NULL;//最佳选择
	if(varTab.find(name)!=varTab.end()){
		vector<Var*>&list=*varTab[name];
		int pathLen=scopePath.size();//当前路径长度
		int maxLen=0;//已经匹配的最大长度
		for(int i=0;i<list.size();i++){
			int len=list[i]->getPath().size();
			//发现候选的变量,路径是自己的前缀
			if(len<=pathLen&&list[i]->getPath()[len-1]==scopePath[len-1]){
				if(len>maxLen){//选取最长匹配
					maxLen=len;
					select=list[i];
				}
			}
		}
	}
	if(!select)SEMERROR(VAR_UN_DEC,name);//变量未声明
	return select;
}

/*
	获取所有全局变量
*/
vector<Var*> SymTab::getGlbVars()
{
	vector<Var*> glbVars;
	for(int i=0;i<varList.size();i++){//遍历变量列表
		string varName=varList[i];
		if(varName[0]=='<')continue;//忽略常量
		vector<Var*>&list=*varTab[varName];
		for(int j=0;j<list.size();j++){
			if(list[j]->getPath().size()==1){//全局的变量
				glbVars.push_back(list[j]);
				break;//仅可能有一个同名全局变量
			}
		}
	}
	return glbVars;
}


/*
	根据实际参数，获取一个函数
*/
Fun* SymTab::getFun(string name,vector<Var*>& args)
{
	if(funTab.find(name)!=funTab.end()){
		Fun* last=funTab[name];
		if(!last->match(args)){
			SEMERROR(FUN_CALL_ERR,name);//行参实参不匹配
			return NULL;
		}
		return last;
	}
	SEMERROR(FUN_UN_DEC,name);//函数未声明
	return NULL;
}

/*
	获取当前分析的函数
*/
Fun* SymTab::getCurFun()
{
	return curFun;
}

/*
	声明一个函数
*/
void SymTab::decFun(Fun* fun)
{
	fun->setExtern(true);
	if(funTab.find(fun->getName())==funTab.end()){ //没有该名字的函数
		funTab[fun->getName()]=fun;//添加函数
		funList.push_back(fun->getName());
	}
	else{
		//判断是否是重复函数声明		
		Fun* last=funTab[fun->getName()];
		if(!last->match(fun)){
			SEMERROR(FUN_DEC_ERR,fun->getName());//函数声明与定义不匹配
		}
		delete fun;
	}
}

/*
	定义一个函数
*/
void SymTab::defFun(Fun* fun)
{
	if(fun->getExtern()){//extern不允许出现在定义
		SEMERROR(EXTERN_FUN_DEF,fun->getName());
		fun->setExtern(false);
	}
	if(funTab.find(fun->getName())==funTab.end()){ //没有该名字的函数
		funTab[fun->getName()]=fun;//添加函数
		funList.push_back(fun->getName());
	}
	else{//已经声明
		Fun*last=funTab[fun->getName()];
		if(last->getExtern()){
			//之前是声明
			if(!last->match(fun)){//匹配的声明
				SEMERROR(FUN_DEC_ERR,fun->getName());//函数声明与定义不匹配
			}
			last->define(fun);//将函数参数拷贝，设定externed为false
		}
		else{//重定义
			SEMERROR(FUN_RE_DEF,fun->getName());
		}
		delete fun;//删除当前函数对象
		fun=last;//公用函数结构体		
	}
	curFun=fun;//当前分析的函数
	ir->genFunHead(curFun);//产生函数入口
}

/*
	结束定义一个函数
*/
void SymTab::endDefFun()
{
	ir->genFunTail(curFun);//产生函数出口
	curFun=NULL;//当前分析的函数置空
}

/*
	添加一条中间代码
*/
void SymTab::addInst(InterInst*inst)
{
	if(curFun)curFun->addInst(inst);
	else delete inst;
}

/*
	进入局部作用域
*/
void SymTab::enter()
{
	scopeId++;
	scopePath.push_back(scopeId);
	if(curFun)curFun->enterScope();
}

/*
	离开局部作用域
*/
void SymTab::leave()
{
	scopePath.pop_back();//撤销更改
	if(curFun)curFun->leaveScope();
}

/*
	执行优化操作
*/
void SymTab::optimize()
{
	for(int i=0;i<funList.size();i++){
		funTab[funList[i]]->optimize(this);
	}
}

/*
	输出符号表信息
*/
void SymTab::toString()
{
	printf("----------变量表----------\n");
	for(int i=0;i<varList.size();i++){
		string varName=varList[i];
		vector<Var*>&list=*varTab[varName];
		printf("%s:\n",varName.c_str());
		for(int j=0;j<list.size();j++){
			printf("\t");
			list[j]->toString();
			printf("\n");
		}
	}
	printf("----------串表-----------\n");
	hash_map<string, Var*, string_hash>::iterator strIt,strEnd=strTab.end();
	for(strIt=strTab.begin();strIt!=strEnd;++strIt)
		printf("%s=%s\n",strIt->second->getName().c_str(),strIt->second->getStrVal().c_str());
	printf("----------函数表----------\n");
	for(int i=0;i<funList.size();i++){
		funTab[funList[i]]->toString();
	}
}

/*
	输出中间代码
*/
void SymTab::printInterCode()
{
	for(int i=0;i<funList.size();i++){
		funTab[funList[i]]->printInterCode();
	}
}

/*
	输出优化的中间代码
*/
void SymTab::printOptCode()
{
	for(int i=0;i<funList.size();i++){
		funTab[funList[i]]->printOptCode();
	}
}

void SymTab::genData(FILE*file)
{
	//生成常量字符串,.rodata段
	fprintf(file, ".section .rodata\n");
	hash_map<string, Var*, string_hash>::iterator strIt,strEnd=strTab.end();
	for(strIt=strTab.begin();strIt!=strEnd;++strIt){
		Var*str=strIt->second;//常量字符串变量
		fprintf(file, "%s:\n", str->getName().c_str());//var:
		fprintf(file, "\t.ascii \"%s\"\n", str->getRawStr().c_str());//.ascii "abc\000"
	}
	//生成数据段和bss段
	fprintf(file, ".data\n");
	vector<Var*> glbVars=getGlbVars();//获取所有全局变量
	for(unsigned int i=0;i<glbVars.size();i++)
	{
		Var*var=glbVars[i];
		fprintf(file, "\t.global %s\n",var->getName().c_str());//.global var
		if(!var->unInit()){//变量初始化了,放在数据段
			fprintf(file, "%s:\n", var->getName().c_str());//var:
			if(var->isBase()){//基本类型初始化 100 'a'
				const char* t=var->isChar()?".byte":".word";
				fprintf(file, "\t%s %d\n", t, var->getVal());//.byte 65  .word 100
			}
			else{//字符指针初始化
				fprintf(file, "\t.word %s\n",var->getPtrVal().c_str());//.word .L0
			}
		}
		else{//放在bss段
			fprintf(file, "\t.comm %s,%d\n", var->getName().c_str(), var->getSize());//.comm var,4
		}
	}
}

/*
	输出汇编文件
*/
void SymTab::genAsm(char*fileName)
{
	//将.c替换为.o，或者直接追加.o
	string newName=fileName;
	int pos=newName.rfind(".c");
	if(pos>0&&pos==newName.length()-2){
		newName.replace(pos,2,".s");
	}
	else newName=newName+".s";
	FILE* file=fopen(newName.c_str(),"w");//创建输出文件
	//生成数据段
	genData(file);
	//生成代码段
	if(Args::opt)fprintf(file,"#优化代码\n");
	else fprintf(file,"#未优化代码\n");
	fprintf(file,".text\n");
	for(int i=0;i<funList.size();i++){
		//printf("-------------生成函数<%s>--------------\n",funTab[funList[i]]->getName().c_str());
		funTab[funList[i]]->genAsm(file);
	}
	fclose(file);
}







