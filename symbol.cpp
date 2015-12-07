#include "symbol.h"
#include "error.h"
#include "token.h"
#include "genir.h"
#include "symtab.h"
#include "compiler.h"
#include "dfg.h"
#include "constprop.h"
#include "redundelim.h"
#include "copyprop.h"
#include "livevar.h"
#include "alloc.h"
#include "platform.h"
#include "selector.h"
#include "platform.h"
#include "peephole.h"

//打印语义错误
#define SEMERROR(code,name) Error::semError(code,name)

/*******************************************************************************
                                   变量结构
*******************************************************************************/

/*
	获取VOID特殊变量
*/
Var* Var::getVoid()
{
	return SymTab::voidVar;
}

/*
	void变量
*/
Var::Var()
{
	clear();
	setName("<void>");//特殊变量名字
	setLeft(false);
	intVal=0;//记录数字数值
	literal=false;//消除字面量标志
	type=KW_VOID;//hack类型
	isPtr=true;//消除基本类型标志
}


/*
	获取true变量
*/
Var* Var::getTrue()
{
	return SymTab::one;
}

/*
	获取步长变量
*/
Var* Var::getStep(Var*v)
{
	if(v->isBase())return SymTab::one;
	else if(v->type==KW_CHAR)return SymTab::one;
	else if(v->type==KW_INT)return SymTab::four;
	else return NULL;
}

/*
	关键信息初始化
*/
void Var::clear()
{
	scopePath.push_back(-1);//默认全局作用域
	externed=false;
	isPtr=false;
	isArray=false;
	isLeft=true;//默认变量是可以作为左值的
	inited=false;
	literal=false;
	size=0;
	offset=0;
	ptr=NULL;//没有指向当前变量的指针变量
	index=-1;//无效索引
	initData=NULL;
	live=false;
	regId=-1;//默认放在内存
	inMem=false;
}

/*
	临时变量
*/
Var::Var(vector<int>&sp,Tag t,bool ptr)
{
	clear();
	scopePath=sp;//初始化路径
	setType(t);
	setPtr(ptr);
	setName("");
	setLeft(false);
}

/*
	拷贝出一个临时变量
*/
Var::Var(vector<int>&sp,Var*v)
{
	clear();
	scopePath=sp;//初始化路径
	setType(v->type);
	setPtr(v->isPtr||v->isArray);//数组 指针都是指针
	setName("");//新建名字
	setLeft(false);
}

/*
	变量，指针
*/
Var::Var(vector<int>&sp,bool ext,Tag t,bool ptr,string name,Var*init)
{
	clear();
	scopePath=sp;//初始化路径
	setExtern(ext);
	setType(t);
	setPtr(ptr);
	setName(name);
	initData=init;
}

/*
	数组
*/
Var::Var(vector<int>&sp,bool ext,Tag t,string name,int len)
{
	clear();
	scopePath=sp;//初始化路径
	setExtern(ext);
	setType(t);
	setName(name);
	setArray(len);
}

/*
	整数变量
*/
Var::Var(int val)
{
	clear();
	setName("<int>");//特殊变量名字
	literal=true;
	setLeft(false);
	setType(KW_INT);
	intVal=val;//记录数字数值
}

	
/*
	常量,不涉及作用域的变化，字符串存储在字符串表，其他常量作为初始值(使用完删除)
*/
Var::Var(Token*lt)
{
	clear();
	literal=true;
	setLeft(false);
	switch(lt->tag){
		case NUM:
			setType(KW_INT);
			name="<int>";//类型作为名字
			intVal=((Num*)lt)->val;//记录数字数值
			break;
		case CH:
			setType(KW_CHAR);
			name="<char>";//类型作为名字
			intVal=0;//高位置0
			charVal=((Char*)lt)->ch;//记录字符值
			break;
		case STR:
			setType(KW_CHAR);
			name=GenIR::genLb();//产生一个新的名字
			strVal=((Str*)lt)->str;//记录字符串值
			setArray(strVal.size()+1);//字符串作为字符数组存储
			break;
	}
}

/*
	设置extern
*/
void Var::setExtern(bool ext)
{
	externed=ext;
	size=0;
}

/*
	设置类型
*/
void Var::setType(Tag t)
{
	type=t;
	if(type==KW_VOID){//无void变量
		SEMERROR(VOID_VAR,"");//不允许使用void变量
		type=KW_INT;//默认为int
	}
	if(!externed&&type==KW_INT)size=4;//整数4字节
	else if(!externed&&type==KW_CHAR)size=1;//字符1字节
}

/*
	设置指针
*/
void Var::setPtr(bool ptr)
{
	if(!ptr)return;
	isPtr=true;
	if(!externed)size=4;//指针都是4字节
}

/*
	设置名称
*/
void Var::setName(string n)
{
	if(n=="")
		n=GenIR::genLb();
	name=n;
}

/*
	设置数组
*/
void Var::setArray(int len)
{
	if(len<=0){
		SEMERROR(ARRAY_LEN_INVALID,name);//数组长度小于等于0错误
		return ;
	}
	else{
		isArray=true;
		isLeft=false;//数组不能作为左值
		arraySize=len;
		if(!externed)size*=len;//变量大小乘以数组长度
	}
}

/*
	初始化检查与信息记录，数组不允许初始化
	局部变量初始化如果使用中间代码生成器返回true
*/
bool Var::setInit()
{
	Var*init=initData;//取出初值数据
	if(!init)return false;//没有初始化表达式
	inited=false;//默认初始化失败
	if(externed)
		SEMERROR(DEC_INIT_DENY,name);//声明不允许初始化
	else if(!GenIR::typeCheck(this,init))
		SEMERROR(VAR_INIT_ERR,name);//类型检查不兼容
	else if(init->literal){//使用常量初始化
		inited=true;//初始化成功！
		if(init->isArray)//初始化常量是数组，必是字符串
			ptrVal=init->name;//字符指针变量初始值=常量字符串的名字
		else//基本类型
			intVal=init->intVal;//拷贝数值数据
	}
	else{//初始值不是常量
		if(scopePath.size()==1)//被初始化变量是全局变量
			SEMERROR(GLB_INIT_ERR,name);//全局变量初始化必须是常量
		else//被初始化变量是局部变量
			return true;
	}
	// if(init->literal && !(init->isArray))//删除整数，字符初始化常量
	// 	delete init;//清除临时的字面变量，字符串和临时变量不清除（已经在符号表内）
	return false;
}

/*
	获取初始化变量数据
*/
Var* Var::getInitData()
{
	return initData;
}


/*
	获取extern
*/
bool Var::getExtern()
{
	return externed;
}

/*
	获取作用域路径
*/
vector<int>& Var::getPath()
{
	return scopePath;
}

/*
	获取类型
*/
Tag Var::getType()
{
	return type;
}

/*
	判断是否是字符变量
*/
bool Var::isChar()
{
	return (type==KW_CHAR) && isBase();//是基本的字符类型
}

/*
	判断字符指针
*/
bool Var::isCharPtr()
{
	return (type==KW_CHAR) && !isBase();//字符指针或者字符数组
}

/*
	获取指针
*/
bool Var::getPtr()
{
	return isPtr;
}

/*
	获取名字
*/
string Var::getName()
{
	return name;
}

/*
	获取数组
*/
bool Var::getArray()
{
	return isArray;
}

/*
	设置指针变量
*/
void Var::setPointer(Var* p)
{
	ptr=p;
}

/*
	获取指针变量
*/
Var* Var::getPointer()
{
	return ptr;
}

/*
	获取字符指针内容
*/
string Var::getPtrVal()
{
	return ptrVal;
}

/*
	获取字符串常量内容
*/
string Var::getStrVal()
{
	return strVal;
}

/*
	获取字符串常量原始内容，将特殊字符转义
*/
string Var::getRawStr()
{
	string raw;
	for(int i=0;i<strVal.size();i++){
		switch(strVal[i])
		{
			case '\n':raw.append("\\n");break;
			case '\t':raw.append("\\t");break;
			case '\0':raw.append("\\000");break;
			case '\\':raw.append("\\\\");break;
			case '\"':raw.append("\\\"");break;
			default:raw.push_back(strVal[i]);
		}
	}
	raw.append("\\000");//结束标记
	return raw;
}

/*
	设置变量的左值
*/
void Var::setLeft(bool lf)
{
	isLeft=lf;
}

/*
	获取变量的左值
*/
bool Var::getLeft()
{
	return isLeft;
}

/*
	设置栈帧偏移
*/
void Var::setOffset(int off)
{
	offset=off;
}

/*
	获取栈帧偏移
*/
int Var::getOffset()
{
	return offset;
}


/*
	获取变量大小
*/
int Var::getSize()
{
	return size;
}

/*
	是数字
*/
bool Var::isVoid()
{
	return type==KW_VOID;
}

/*
	是基本类型
*/
bool Var::isBase()
{
	return !isArray && !isPtr;
}

/*
	是引用类型
*/
bool Var::isRef()
{
	return !!ptr;
}

/*
	是否初始化
*/
bool Var::unInit()
{
	return !inited;
}

/*
	是否是常量
*/
bool Var::notConst()
{
	return !literal;
}

/*
	获取常量值
*/
int Var::getVal()
{
	return intVal;
}

/*
		是基本类型常量（字符串除外），没有存储在符号表，需要单独内存管理
*/	
bool Var::isLiteral()
{
	return this->literal&&isBase();
}



/*
	输出变量的中间代码形式
*/
void Var::value()
{
	if(literal){//是字面量
		if(type==KW_INT)
			printf("%d",intVal);
		else if(type==KW_CHAR){
			if(isArray)
				printf("%s",name.c_str());
			else
				printf("%d",charVal);
		}
	}
	else
		printf("%s",name.c_str());
}

/*
	输出变量信息
*/
void Var::toString()
{
	if(externed)printf("externed ");
	//输出type
	printf("%s",tokenName[type]);
	//输出指针
	if(isPtr)printf("*");
	//输出名字
	printf(" %s",name.c_str());
	//输出数组
	if(isArray)printf("[%d]",arraySize);
	//输出初始值
	if(inited){
		printf(" = ");
		switch(type){
			case KW_INT:printf("%d",intVal);break;
			case KW_CHAR:
				if(isPtr)printf("<%s>",ptrVal.c_str());
				else printf("%c",charVal);
				break;
		}
	}
	printf("; size=%d scope=\"",size);	
	for(int i=0;i<scopePath.size();i++){
		printf("/%d",scopePath[i]);
	}
	printf("\" ");
	if(offset>0)
		printf("addr=[ebp+%d]",offset);
	else if(offset<0)
		printf("addr=[ebp%d]",offset);
	else if(name[0]!='<')
		printf("addr=<%s>",name.c_str());
	else
		printf("value='%d'",getVal());
}	
	
/*******************************************************************************
                                   函数结构
*******************************************************************************/
/*
	构造函数声明，返回值+名称+参数列表
*/
Fun::Fun(bool ext,Tag t,string n,vector<Var*>&paraList)
{
	externed=ext;
	type=t;
	name=n;
	paraVar=paraList;
	curEsp=Plat::stackBase;//没有执行寄存器分配前，不需要保存现场，栈帧基址不需要修正。
	maxDepth=Plat::stackBase;//防止没有定义局部变量导致最大值出错。
	//保存现场和恢复现场有函数内部解决，因此参数偏移不需要修正！
	for(int i=0,argOff=4;i<paraVar.size();i++,argOff+=4){//初始化参数变量地址从左到右，参数进栈从右到左
		paraVar[i]->setOffset(argOff);
	}
	dfg=NULL;
	relocated=false;
}

Fun::~Fun()
{
	if(dfg)delete dfg;//清理数据流图
}


/*
	定位局部变了栈帧偏移
*/
void Fun::locate(Var*var)
{
	int size=var->getSize();
	size+=(4-size%4)%4;//按照4字节的大小整数倍分配局部变量
	scopeEsp.back()+=size;//累计作用域大小
	curEsp+=size;//累计当前作用域大小
	var->setOffset(-curEsp);//局部变量偏移为负数
}

/*
	声明定义匹配
*/
#define SEMWARN(code,name) Error::semWarn(code,name)
bool Fun::match(Fun*f)
{
	//区分函数的返回值
	if(name!=f->name)
		return false;
	if(paraVar.size()!=f->paraVar.size())
		return false;
	int len=paraVar.size();
	for(int i=0;i<len;i++){
		if(GenIR::typeCheck(paraVar[i],f->paraVar[i])){//类型兼容
			if(paraVar[i]->getType()!=f->paraVar[i]->getType()){//但是不完全匹配
				SEMWARN(FUN_DEC_CONFLICT,name);//函数声明冲突——警告
			}
		}
		else
			return false;
	}
	//匹配成功后再验证返回类型
	if(type!=f->type){
		SEMWARN(FUN_RET_CONFLICT,name);//函数返回值冲突——警告
	}
	return true;
}

/*
	行参实参匹配
*/
bool Fun::match(vector<Var*>&args)
{
	if(paraVar.size()!=args.size())
		return false;
	int len=paraVar.size();
	for(int i=0;i<len;i++){
		if(!GenIR::typeCheck(paraVar[i],args[i]))//类型检查不兼容
			return false;
	}
	return true;		
}

/*
	将函数声明转换为定义，需要拷贝参数列表，设定extern
*/
void Fun::define(Fun*def)
{
	externed=false;//定义
	paraVar=def->paraVar;//拷贝参数
}

/*
	添加一条中间代码
*/
void Fun::addInst(InterInst*inst)
{
	interCode.addInst(inst);
}

/*
	设置函数返回点
*/
void Fun::setReturnPoint(InterInst*inst)
{
	returnPoint=inst;
}

/*
	获取函数返回点
*/
InterInst* Fun::getReturnPoint()
{
	return returnPoint;
}

/*
	进入一个新的作用域
*/
void Fun::enterScope()
{
	scopeEsp.push_back(0);
}

/*
	离开当前作用域
*/
void Fun::leaveScope()
{
	maxDepth=(curEsp>maxDepth)?curEsp:maxDepth;//计算最大深度
	curEsp-=scopeEsp.back();
	scopeEsp.pop_back();
}

/*
	设置extern
*/
void Fun::setExtern(bool ext)
{
	externed=ext;
}

/*
	获取extern
*/
bool Fun::getExtern()
{
	return externed;
}

Tag Fun::getType()
{
	return type;
}

/*
	获取名字
*/
string& Fun::getName()
{
	return name;
}

/*
	获取参数列表，用于为参数生成加载代码
*/
vector<Var*>& Fun::getParaVar()
{
	return paraVar;
}


/*
	输出信息
*/
void Fun::toString()
{
	//输出type
	printf("%s",tokenName[type]);
	//输出名字
	printf(" %s",name.c_str());
	//输出参数列表
	printf("(");
	for(int i=0;i<paraVar.size();i++){
		printf("<%s>",paraVar[i]->getName().c_str());
		if(i!=paraVar.size()-1)printf(",");
	}
	printf(")");
	if(externed)printf(";\n");
	else{
		printf(":\n");
		printf("\t\tmaxDepth=%d\n",maxDepth);
	}
}

/*
	执行优化操作
*/
void Fun::optimize(SymTab*tab)
{
	if(externed)return;//函数声明不处理
	//数据流图
	dfg=new DFG(interCode);//创建数据流图
	if(Args::showBlock)dfg->toString();//输出基本块和流图关系
	if(!Args::opt)return;//不执行优化

	//常量传播：代数化简，条件跳转优化，不可达代码消除
	ConstPropagation conPro(dfg,tab,paraVar);//常量传播
#ifdef CONST
	conPro.propagate();//常量传播
#endif

	//冗余消除
	RedundElim re(dfg,tab);	
#ifdef RED
	re.elimate();
#endif

	//复写传播
	CopyPropagation cp(dfg);
#ifdef DEAD
	cp.propagate();
#endif
	
	//活跃变量
	LiveVar lv(dfg,tab,paraVar);
#ifdef DEAD
	lv.elimateDeadCode();
#else
	#ifdef REG
		lv.analyse();
	#endif
#endif

	//优化结果存储在optCode
	dfg->toCode(optCode);//导出数据流图为中间代码

	//寄存器分配和局部变量栈地址重新计算
	CoGraph cg(optCode,paraVar,&lv,this);//初始化冲突图
	cg.alloc();//重新分配变量的寄存器和栈帧地址
}

/*
	输出中间代码
*/
void Fun::printInterCode()
{
	if(externed)return;
	printf("-------------<%s>Start--------------\n",name.c_str());
	interCode.toString();
	printf("--------------<%s>End---------------\n",name.c_str());
}

/*
	输出优化后的中间代码
*/
void Fun::printOptCode()
{
	if(externed)return;
	printf("-------------<%s>Start--------------\n",name.c_str());
	for (list<InterInst*>::iterator i = optCode.begin(); i != optCode.end(); ++i)
	{
		(*i)->toString();
	}
	printf("--------------<%s>End---------------\n",name.c_str());
}

/*
	输出汇编代码
*/
void Fun::genAsm(FILE*file)
{
	if(externed)return;
	//导出最终的代码,如果优化则是优化后的中间代码，否则就是普通的中间代码
	vector<InterInst*> code;
	if(Args::opt){//经过优化
		for(list<InterInst*>::iterator it=optCode.begin();it!=optCode.end();++it){
			code.push_back(*it);
		}
	}
	else{//未优化，将中间代码导出
		code=interCode.getCode();
	}
	const char* pname=name.c_str();
	fprintf(file,"#函数%s代码\n",pname);
	fprintf(file,"\t.global %s\n",pname);//.global fun\n
	fprintf(file,"%s:\n",pname);//fun:\n
	ILoc il;//ILOC代码
	//将最终的中间代码转化为ILOC代码
	Selector sl(code,il);//指令选择器
	sl.select();
	//对ILOC代码进行窥孔优化
	PeepHole ph(il);//窥孔优化器
#ifdef PEEP
	if(Args::opt)ph.filter();//优化过滤代码
#endif

	//将优化后的ILOC代码输出为汇编代码
	il.outPut(file);
}

/*
	获取最大栈帧深度
*/
int Fun::getMaxDep()
{
	return maxDepth;
}

/*
	设置最大栈帧深度
*/
void Fun::setMaxDep(int dep)
{
	maxDepth=dep;
	//设置函数栈帧被重定位标记，用于生成不同的栈帧保护代码
	relocated=true;
}

/*
	函数栈帧被重新定位了？
*/
bool Fun::isRelocated()
{
	return relocated;
}




