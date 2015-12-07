#include "common.h"
#include "semantic.h"
/**  *****************************************************************************************************************************
                                                             ***语法分析***
   ********************************************************************************************************************************/
//--语法分析
void program();
void dec();
symbol type();
void dectail(symbol dec_type,string dec_name);
void varlist(symbol dec_type);
void para();
void paralist();
void funtail(symbol dec_type,string dec_name);

void block(int initvar_num,int &level,int lopId,int blockAddr);
void childprogram(int & num,int &level,int lopId,int blockAddr);
void localdec(int & num,int &level );
void localdectail(int & num,symbol local_type,int& level);

void statement(int & num,int &level,int lopId,int blockAddr);
void whilestat(int & num,int &level);
void ifstat(int & num,int &level,int lopId,int blockAddr);
void retstat(int & num,int &level);
void returntail(int & num,int &level);
var_record* idtail(string refname,int & num);
void realarg(string refname,int &var_num);
void arglist(int & num);

var_record* expr(int & num);
var_record* exptail(var_record*p_factor1,int & num);
void cmps();
var_record* aloexp(int & num);
var_record* itemtail(var_record*p_factor1,int & num);
void adds();
var_record* item(int & num);
var_record* factortail(var_record*p_factor1,int & var_num);
void muls();
var_record* factor(int & num);

//nextToken(),match(),语法分析
char symName[][30]=
{
  "null","ident","exception","number",//空，标识符，数字
  "+","-","*","/","=",//加，减，乘，除,赋值
  ">",">=","<","<=","=","!=",//>,>=,<,<=,==,!=
  ">>","<<",
  ",","chara","string",";","(",")","{","}",//界符 , ch str ; ( ) { } #
  "",
  "break","char","continue","else","extern","if","in","int","out","return","string","void","while"//保留字
  ,""
};
enum symbol oldtoken=null,token=null;//记录最近两个token
int wait=0;//指导nextToken是否继续取符号，每次设置只能作用一次
#define BACK wait=1;
int identinexpr=0;//指示标识符是否单独出现在表达式中

void synterror(enum errcode err,int pos);

/**
  读取下一个有效的符号，testSym[]作为测试观察使用
  返回值:编译结束返回-1，一般情况现返回0.
*/

int nextToken()
{
  if(wait==1)
  {
    wait=0;//还原
    return 0;
  }
  int flag=0;
  while(1)
  {
    flag=getSym();
    if(sym==null||sym==excep)//无效符号掠过
    {
      if(flag==-1)//文件结束
      {
	oldtoken=token;
	token=null;
	//int n=switchFile();//只处理一个源文件，关闭文件切换功能
	//if(n==0)//切换成功
	  //continue;
	return -1;
      }
    }
    else//get effective symbol
    {
      if(showLex)
      {
	if(showSyn)
	  printf("<------------------------------------------------------------------------------------------->\n");
	else
	  printf("<---------------------------------------------------->\n");

	if(sym==strings)
	  printf("字符串\t\t\"%s\"\n",str);
	else if(sym==ident)
	  printf("标识符\t\t(%s)\n",id);
	else if(sym==number)
	    printf("数字\t\t[%d]\n",num);
	else if(sym==chara)
	    printf("字符\t\t'%c'\n",letter);
	else if(sym>rsv_min&&sym<rsv_max)
	    printf("关键字\t\t<%s>\n",id);
	else
	    printf("界符\t\t%s\n",symName[sym]);
      }
      oldtoken=token;
      token=sym;
      return 0;
    }
  }
}
/**
  只做比较，不向下取符号
*/
int match(enum symbol s)
{
  return(token==s);
}
///输出语法分析信息
void p(const char* msg,int ex)
{
  if(!showSyn)
    return ;
  printf("\t%s",msg);
  if(ex==1)//输出tvar
  {
    printf("\t{类型:<%s>\t,名称:(%s)}\n",symName[tvar.type],tvar.name.c_str());
  }
  else if(ex==2)//输出tfun
  {
    printf("\t{类型:<%s>\t,名称:(%s)\t,参数列表:[",symName[tfun.type],tfun.name.c_str());
    int l=tfun.args->size();
    for(int i=0;i<l;i++)
    {
      if(i!=0)
      {
	printf(",");
      }
      printf("<%s>",symName[(*(tfun.args))[i]]);
    }
    printf("]}\n");
  }
  else
    printf("\n");
}
///输出语义分析信息
void sp(const char* msg)
{
  if(!showSem)
    return ;
  printf("\t\t%s\n",msg);
}
/**
  递归下降语法分析处理程序
*/
int compileOk=0;//编译成功标记
//<program>	->	<dec><program>|^
void program()//untest:调用之前是否提前测试了符号
{
  if(nextToken()==-1)//文件末尾 #
  {
    compileOk=1;
    //进行最终的校验以及静态数据的生成
    table.over();
    if(errorNum==0)
    	printf("编译完成！(");
    else
    	printf("编译失败！(");
    printf("错误=%d 警告=%d)\n",errorNum,warnNum);
    return ;
  }
  else
  {
    dec();
    program();
  }
}
//<dec>		->	<type>ident<dectail>|semicon|rsv_extern<type>ident semicon
void dec()//tested
{
  if(token==semicon)//空声明
  {
    return;
  }
  else if(token==rsv_extern)//外部变量声明
  {
    symbol dec_type;//临时记录声明的类型
    string dec_name="";//临时记录声明标识符的名称
    nextToken();
    dec_type=type();
    nextToken();
    if(!match(ident))//标识符不匹配，极有可能是没有标识符,回退
    {
      synterror(identlost,-1);
      BACK
    }
    else//声明标识符成功
    {
      dec_name+=id;
      tvar.init(dec_type,dec_name);//初始化变量记录
      tvar.externed=1;//表示外部变量
      if(dec_type==rsv_string)
      {
				tvar.strValId=-2;//全局的string
      }
      table.addvar();//添加变量记录
      p("全局变量声明",1);
    }
    nextToken();
    if(!match(semicon))
    {
      if(token==rsv_extern||token==rsv_void||token==rsv_int||token==rsv_char||token==rsv_string)//丢失分号
      {
	synterror(semiconlost,-1);
	BACK
      }
      else
      {
	synterror(semiconwrong,0);
      }
    }
  }
  else
  {
    symbol dec_type;//临时记录声明的类型
    string dec_name="";//临时记录声明标识符的名称
    dec_type=type();
    nextToken();
    if(!match(ident))//标识符不匹配，极有可能是没有标识符,回退
    {
      synterror(identlost,-1);
      BACK
    }
    else//声明标识符成功，还不能确定是变量还是函数,暂时记录作为参数传递
    {
      dec_name+=id;
    }
    dectail(dec_type,dec_name);
  }
}
//<type>		->	rsv_void|rsv_int|rsv_char
/**
  返回符号的类型，错误返回null
*/
symbol type()//tested
{
  switch(token)
  {
    case rsv_int:
      return rsv_int;
      break;
    case rsv_char:
      return rsv_char;
      break;
    case rsv_void:
      return rsv_void;
      break;
    case rsv_string:
      return rsv_string;
      break;
    case ident:
      synterror(typelost,-1);
      BACK
      break;
    default:
      synterror(typewrong,0);
  }
  return null;
}
//<dectail>	->	semicon|<varlist>semicon|lparen<para>rparen<block>
void dectail(symbol dec_type,string dec_name)//untest
{
  nextToken();
  switch(token)
  {
    case semicon:
      //单独的变量声明，添加到符号表，这是全局变量，代码生成的时候要生成静态数据
      tvar.init(dec_type,dec_name);//初始化变量记录
      if(dec_type==rsv_string)
      {
	tvar.strValId=-2;//全局的string
      }
      table.addvar();//添加变量记录
      p("全局变量定义",1);
      break;
    case lparen:

      tfun.init(dec_type,dec_name);//初始化函数记录，仅返回类型和函数名
      para();
      //match(rpren);
      funtail(dec_type,dec_name);
      //block();
      break;
    default:
      tvar.init(dec_type,dec_name);//初始化变量记录
      if(dec_type==rsv_string)
      {
	tvar.strValId=-2;
      }
      table.addvar();//添加变量记录
      p("全局变量定义",1);
      varlist(dec_type);//可能是^,会向下取符号，不需要重复取
      //match(semicon);
  }
}
//<funtail>	->	<block>|semicon
void funtail(symbol dec_type,string dec_name)
{
  static int level=0;//复合语句的层次
  nextToken();
  if(token==semicon)//函数声明
  {
    p("函数声明",2);
    //添加函数声明记录
    table.addfun();
    return;
  }
  else if(token==lbrac)//函数定义
  {
    p("函数定义",2);
    tfun.defined=1;//标记函数定义属性
    table.addfun();
    BACK
    block(0,level,0,0);
    level=0;//恢复
    tfun.poplocalvars(-1);//清除参数
    genFuntail();
    return;
  }
  else if(token==ident||token==rsv_if||token==rsv_while||token==rsv_return||token==rsv_break||token==rsv_continue
    ||token==rsv_in||token==rsv_out||token==rbrac)//必然是函数定义
  {
    BACK
    block(0,level,0,0);
    level=0;//恢复
    return ;
  }
  else if(token==rsv_void||token==rsv_int||token==rsv_char||token==rsv_string)//其他的作为声明处理
  {
    synterror(semiconlost,-1);
    BACK
    return ;
  }
  else
  {
    synterror(semiconwrong,0);
    return ;
  }
}
//<varlist>	->	comma ident<varlist>|^
void varlist(symbol dec_type)//tested
{
  if(token==comma)
  {
    nextToken();
    if(!match(ident))
    {
      BACK
      synterror(identlost,-1);
    }
    else
    {
      string dec_name="";
      dec_name+=id;
      tvar.init(dec_type,dec_name);//初始化变量记录
      if(dec_type==rsv_string)
      {
	tvar.strValId=-2;
      }
      table.addvar();//添加变量记录
      p("全局变量声明",1);
    }
    nextToken();
    varlist(dec_type);

  }
  else if(token==semicon)
  {
    return;
  }
  else if(token==ident)//极有可能忘记逗号
  {
    synterror(commalost,-1);
    nextToken();
    varlist(dec_type);
  }
  else if(token==rsv_int||token==rsv_void||token==rsv_char||token==rsv_string)//忘记分号
  {
    synterror(semiconlost,-1);
    BACK
  }
  else//其他符号，告知当作分号处理
  {
    synterror(semiconwrong,0);
  }
}
//<para>		->	<type>ident<paralist>|^
void para()//untest
{
  nextToken();
  switch(token)
  {
    case rparen://^匹配成功--follow
      break;
    default:
      symbol para_type;//记录临时参数类型
      para_type=type();
      string para_name="";//记录参数名字
      nextToken();
      if(!match(ident))//标识符不匹配，极有可能是没有参数名字,回退
      {
	synterror(paralost,-1);
	BACK
      }
      else
      {
	sp("对形式参数的声明进行语义检查");
	para_name+=id;
	int msg_back=table.hasname(para_name);
	if(msg_back==0)//忽略同名的参数和局部变量
	{
	  tvar.init(para_type,para_name);//初始化参数记录
	  tfun.addarg();//添加一个参数
	}
	else if(msg_back==1)
	{
	  //参数名字相同错误
	  semerror(para_redef);
	}
	//其他值比如-1是强制终止，就不处理了
      }
      paralist();
  }
}
//<paralist>	->	comma<type>ident<paralist>|^

void paralist()//untest
{
  nextToken();
  if(token==comma)
  {
    nextToken();
    symbol para_type;//记录临时参数类型
    para_type=type();
    string para_name="";//记录参数名字
    nextToken();
    if(!match(ident))//标识符不匹配，极有可能是没有参数名字,回退
    {
      synterror(paralost,-1);
      BACK
    }
    else
    {
	sp("对形式参数的声明进行语义检查");
	para_name+=id;
	int msg_back=table.hasname(para_name);
	if(msg_back==0)//忽略同名的参数和局部变量
	{
	  tvar.init(para_type,para_name);//初始化参数记录
	  tfun.addarg();//添加一个参数
	}
	else if(msg_back==1)
	{
	  //参数名字相同错误
	  semerror(para_redef);
	}
	//其他值比如-1是强制终止，就不处理了
    }
    paralist();
  }
  else if(token==lbrac||token==semicon)//定义或者声明时候缺少)
  {
    synterror(rparenlost,0);
    BACK
  }
  else if(token==rparen)//成功
  {
    return ;
  }
  else if(token==rsv_int||token==rsv_void||token==rsv_char||rsv_string)//忘记逗号
  {
    synterror(commalost,-1);
    nextToken();
    if(!match(ident))//标识符不匹配，极有可能是没有参数名字,回退
    {
      synterror(paralost,-1);
      BACK
    }
    paralist();
  }



}
//<block>		->	lbrac<childprogram>rbrac
void block(int initvar_num,int& level,int lopId,int blockAddr)
{
  nextToken();
  if(!match(lbrac))//丢失{
  {
    synterror(lbraclost,-1);
    BACK
  }
  int var_num=initvar_num;//复合语句里变量的个数,一般是0，但是在if和while语句就不一定了
  level++;//每次进入时加1

  childprogram(var_num,level,lopId,blockAddr);

  level--;
  //match(rbrac);
  //要清除局部变量名字表
  tfun.poplocalvars(var_num);
}
//<childprogram>	->	<localdec><childprogram>|<statements><childprogram>|^
int rbracislost=0;//}丢失异常，维护恢复,紧急恢复
void childprogram(int& var_num,int& level,int lopId,int blockAddr)
{
  nextToken();
  if(token==semicon||token==rsv_while||token==rsv_if||token==rsv_return||token==ident
    ||token==rsv_break||token==rsv_continue||token==rsv_in||token==rsv_out)//正常的语句
  {
    statement(var_num,level,lopId,blockAddr);
    childprogram(var_num,level,lopId,blockAddr);
  }
  else if(token==rsv_void||token==rsv_int||token==rsv_char||token==rsv_string)//局部变量声明
  {
    localdec(var_num,level);
    if(rbracislost==1)
    {
      rbracislost=0;
    }
    else
    {
      childprogram(var_num,level,lopId,blockAddr);
    }
  }
  else if(token==rbrac)//复合语句结尾
  {
    return;//复合语句解析结束
  }
  else if(token==null)//一直到文件最后没有}
  {
    synterror(rbraclost,-1);
  }
  else//无效的语句
  {
    synterror(statementexcp,0);
  }
}
//<localdec>	->	<type>ident<lcoaldectail>semicon
void localdec(int& var_num,int&level)
{
  symbol local_type;
  string local_name="";
  local_type=type();
  nextToken();
  if(!match(ident))//标识符不匹配，极有可能是没有标识符,回退
  {
    synterror(identlost,-1);
    BACK
  }
  else
  {
    sp("对局部变量的声明进行语义检查");
    local_name+=id;
    int msg_back=table.hasname(local_name);
    if(msg_back==0)//防止局部变量名字重复
    {
      tvar.init(local_type,local_name);//初始化局部变量记录
      tfun.pushlocalvar();//添加一个局部变量
      p("局部变量声明",1);
      var_num++;
    }
    else if(msg_back==1)
    {
      //局部变量定义重复错误
      semerror(localvar_redef);
    }
    //其他不处理

  }
  nextToken();
  localdectail(var_num,local_type,level);
  //match(semicon);
}
//<localvartail>	->	comma ident<localvartail>|^
void localdectail(int& var_num,symbol local_type,int&level)
{
  if(token==comma)
  {
    nextToken();
    if(!match(ident))
    {
      BACK
      synterror(localidentlost,-1);
    }
    else
    {
      sp("对局部变量的声明进行语义检查");
      string local_name="";
      local_name+=id;
      int msg_back=table.hasname(local_name);
      if(msg_back==0)//防止局部变量名字重复
      {
	tvar.init(local_type,local_name);//初始化局部变量记录
	tfun.pushlocalvar();//添加一个局部变量
	p("局部变量声明",1);
	var_num++;
      }
      else if(msg_back==1)
      {
	//局部变量定义重复错误
	semerror(localvar_redef);
      }
    }
    nextToken();
    localdectail(var_num,local_type,level);

  }
  else if(token==semicon)
  {
    return;
  }
  else if(token==ident)//极有可能忘记逗号
  {
    synterror(commalost,-1);
    nextToken();
    localdectail(var_num,local_type,level);
  }
  else if(token==rsv_int||token==rsv_void||token==rsv_char||token==rsv_string||token==rbrac)//忘记分号
  {
    synterror(semiconlost,-1);
    BACK
  }
  else if(token==lparen)//既有可能是丢失}导致后边的函数定义被解析为局部变量声明，此时应该报丢失}错误,--唯一一种识别}丢失的方式--，此时应该转到para()
  {
    rbracislost=1;//}丢失
    synterror(rbraclost,-1);
    para();
    block(0,level,0,0);
  }
  else//其他符号，告知当作分号处理
  {
    synterror(semiconwrong,0);
  }
}
//<statement>	->	ident<idtail>semicon|<whilestat>|<ifstat>|<retstat>|semicon|rsv_break semicon|rsv_continue semicon
void statement(int & var_num ,int& level,int lopId,int blockAddr)
{
  string refname="";

  switch(token)
  {
    case semicon:
      break;
    case rsv_while:
      p("while复合语句",0);
      whilestat(var_num,level);
      break;
    case rsv_if:
      p("if-else复合语句",0);
      ifstat(var_num,level,lopId,blockAddr);
      break;
    case rsv_break:
      p("break语句",0);
      nextToken();
      if(token==ident||token==rsv_while||token==rsv_if||token==rsv_return||token==rsv_break||token==rsv_continue
	||token==rsv_in||token==rsv_out||token==rbrac)
      {
	synterror(semiconlost,-1);
	BACK
      }else if(token!=semicon)
      {
	synterror(semiconwrong,0);
      }
      //生成break
      sp("对break语句的位置进行语义检查");
      if(lopId!=0)
      {
	genBlock(blockAddr);
	fprintf(fout,"\tjmp @while_%d_exit\n",lopId);
      }
      else
      {
	semerror(break_nin_while);
      }
      break;
    case rsv_continue:
      p("continue语句",0);
      nextToken();
      if(token==ident||token==rsv_while||token==rsv_if||token==rsv_return||token==rsv_break||token==rsv_continue
	||token==rsv_in||token==rsv_out||token==rbrac)
      {
	synterror(semiconlost,-1);
	BACK
      }else if(token!=semicon)
      {
	synterror(semiconwrong,0);
      }
      //生成continue
      sp("对continue语句的位置进行语义检查");
      if(lopId!=0)
      {
	genBlock(blockAddr);
	fprintf(fout,"\tjmp @while_%d_lop\n",lopId);
      }
      else
      {
	semerror(continue_nin_while);
      }
      break;
    case rsv_return:
      p("return语句",0);
      retstat(var_num,level);
      break;
    case rsv_in:
      p("in语句",0);
      nextToken();
      if(!match(input))
      {
	synterror(input_err,0);
      }
      nextToken();
      if(!match(ident))
      {
	synterror(na_input,0);
      }
      else
      {
	refname+=id;
	///input
	genInput(table.getVar(refname),var_num);
      }
      nextToken();
      if(!match(semicon))
      {
	synterror(semiconlost,-1);
	BACK
      }
      break;
    case rsv_out:
      p("out语句",0);
      nextToken();
      if(!match(output))
      {
	synterror(output_err,0);
      }
      ///output
      genOutput(expr(var_num),var_num);
      nextToken();
      if(!match(semicon))
      {
	synterror(semiconlost,-1);
	BACK
      }
      break;
    case ident:
      //错误检查
      refname+=id;
      idtail(refname,var_num);
      nextToken();
      if(!match(semicon))//赋值语句或者函数调用语句丢失分号，记得回退
      {
	synterror(semiconlost,-1);
	BACK
      }
      break;
  }
}
int lopID=0;
//<whilestat>	->	rsv_while lparen<expr>rparen<block>
void whilestat(int &var_num,int& level)
{
  lopID++;
  int id=lopID;
  nextToken();
  if(!match(lparen))
  {
    if(token==ident||token==number||token==chara||token==strings||token==lparen)
    {
      synterror(lparenlost,-1);
      BACK
    }
    else//无效字符
    {
      synterror(lparenwrong,0);
    }
  }
  if(showGen)
	cout<<"生成while循环框架"<<endl;
  fprintf(fout,"@while_%d_lop:\n",id);
  int blockAddr=genBlock(-1);
  int initvar_num_while=0;//把条件表达式放在块内部的处理
  var_record*cond=expr(initvar_num_while);
  genCondition(cond);//产生调价表达式的代码
  fprintf(fout,"\tje @while_%d_exit\n",id);

  nextToken();
  if(!match(rparen))
  {
    if(token==lbrac||
      token==semicon||token==rsv_while||token==rsv_if||token==rsv_return||token==rsv_break||token==rsv_continue||token==rsv_in||token==rsv_out||
      token==ident||token==rsv_void||token==rsv_int||token==rsv_char||token==rsv_string)
    {
      synterror(staterparenlost,-1);
      BACK
    }
    else//无效字符
    {
      synterror(rparenwrong,0);
    }
  }

  block(initvar_num_while,level,lopID,blockAddr);
  genBlock(blockAddr);
  fprintf(fout,"\tjmp @while_%d_lop\n",id);
  fprintf(fout,"@while_%d_exit:\n",id);
}
int ifID=0;
//<ifstat>	->	rsv_if lparen<expr>rparen<block>rsv_else<block>
void ifstat(int &var_num,int & level,int lopId,int blockAddr)
{
    ifID++;
    int id=ifID;
    nextToken();
    if(!match(lparen))
    {
      if(token==ident||token==number||token==chara||token==strings||token==lparen)
      {
	synterror(lparenlost,-1);
	BACK
      }
      else//无效字符
      {
	synterror(lparenwrong,0);
      }
    }
    if(showGen)
	cout<<"生成if-else条件框架"<<endl;
    int blockAddr1=genBlock(-1);
    int initvar_num_if=0;//把条件表达式放在块内部的处理
    var_record*cond=expr(initvar_num_if);
    genCondition(cond);//产生调价表达式的代码
    fprintf(fout,"\tje @if_%d_middle\n",id);
    nextToken();
    if(!match(rparen))
    {
      if(token==lbrac||
	token==semicon||token==rsv_while||token==rsv_if||token==rsv_return||token==rsv_break||token==rsv_continue
	||token==rsv_in||token==rsv_out||
	token==ident||token==rsv_void||token==rsv_int||token==rsv_char||token==rsv_string)
      {
	synterror(staterparenlost,-1);
	BACK
      }
      else//无效字符
      {
	synterror(rparenwrong,0);
      }
    }

    block(initvar_num_if,level,lopId,blockAddr);
    genBlock(blockAddr1);
    fprintf(fout,"\tjmp @if_%d_end\n",id);
    fprintf(fout,"@if_%d_middle:\n",id);
    genBlock(blockAddr1);//在转到else之后要有退出代码，保证条件的临时变量清除

    nextToken();
    if(!match(rsv_else))
    {
      if(token==lbrac||
	token==semicon||token==rsv_while||token==rsv_if||token==rsv_return||token==rsv_break||token==rsv_continue||token==rsv_in||token==rsv_out
	||token==rsv_void||token==rsv_int||token==rsv_char||token==rsv_string)//else 丢失
      {
	synterror(elselost,-1);
	BACK
      }
      else if(token==ident)//else 拼写错误
      {
	synterror(elsespelterr,0);
      }
      else
      {
	synterror(elsewrong,0);//意外字符
      }
    }
    else
    {
    }

    block(0,level,lopId,blockAddr);
    genBlock(blockAddr1);
    fprintf(fout,"@if_%d_end:\n",id);


}
//<retstat>	->	rsv_return<expr>semicon
void retstat(int &var_num,int & level)
{
  returntail(var_num,level);
  nextToken();
  if(!match(semicon))
  {
    if(token==rbrac)//丢失return 后边的 ;
    {
      synterror(semiconlost,-1);
      BACK
    }
  }
}
//<returntail>	->	<expr>|^
void returntail(int & var_num,int &level)
{
  if(level==1)
    tfun.hadret=1;


  nextToken();
  if(token==ident||token==number||token==chara||token==strings||token==lparen)
  {
    BACK
    var_record * ret=expr(var_num);
    sp("对return语句返回值的类型进行语义检查");
    if(ret!=NULL&&(ret->type!=tfun.type))
    {
      //返回值类型不兼容
      semerror(ret_type_err);
    }
    genReturn(ret,var_num);

  }
  else if(token==semicon)//return ;
  {
    BACK
    sp("对return语句返回值的类型进行语义检查");
    if(rsv_void!=tfun.type)
    {
      //返回值类型不兼容
      semerror(ret_type_err);
    }
    genReturn(NULL,var_num);
    return;
  }
  else if(token==rbrac)
  {
    BACK
    return;
  }
  else
  {
    synterror(returnwrong,0);
  }
}
//<idtail>	->	assign<expr>|lparen<realarg>rparen

var_record* idtail(string refname,int &var_num)
{
  nextToken();

  if(token==assign)
  {
    p("赋值语句",0);
    var_record *src=expr(var_num);
    var_record *des=table.getVar(refname);
    return genAssign(des,src,var_num);
  }
  else if(token==lparen)
  {
    realarg(refname,var_num);
    p("函数调用语句",0);
    //生成参数代码
    var_record*var_ret=table.genCall(refname,var_num);
    nextToken();
    if(!match(rparen))
    {
      synterror(rparenlost,-1);
      BACK
    }
    return var_ret;
  }
  else if(identinexpr==1)//表达式中可以单独出现标识符，基于此消除表达式中非a=b类型的错误
  {
    identinexpr=0;
    BACK
    return table.getVar(refname);
  }
  else
  {
    synterror(idtaillost,-1);
    BACK
  }
  return NULL;
}
//<realarg>	->	<expr><arglist>|^
void realarg(string refname,int &var_num)
{
  nextToken();
  if(token==ident||token==number||token==chara||token==strings||token==lparen)
  {
    BACK

    table.addrealarg(expr(var_num),var_num);
    arglist(var_num);
  }
  else if(token==rparen||token==semicon)//^
  {
    BACK
    return;
  }
  else if(token==comma)//参数丢失
  {
    synterror(arglost,-1);
    BACK
    arglist(var_num);
  }
  else//报错
  {
    synterror(argwrong,0);
  }
}
//<arglist>	->	comma<expr><arglist>|^
void arglist(int &var_num)
{
  nextToken();
  if(token==comma)
  {
    nextToken();
    if(token==ident||token==number||token==chara||token==strings||token==lparen)
    {
      BACK
      table.addrealarg(expr(var_num),var_num);
      arglist(var_num);
    }
    else if(token==comma)
    {
      synterror(arglost,-1);
      BACK
      arglist(var_num);
    }
    else if(token==semicon||token==rparen)
    {
      synterror(arglost,-1);
      BACK
      return;
    }
    else
    {
      synterror(argwrong,0);
    }

  }
  else if(token==rparen||token==semicon)
  {
    BACK
    return;
  }
  else if(token==ident||token==number||token==chara||token==strings||token==lparen)//极有可能忘记逗号
  {
    synterror(commalost,-1);
    BACK
    expr(var_num);
    arglist(var_num);
  }
  else//其他符号，告知当作分号处理
  {
    synterror(arglistwrong,0);
  }
}
//<exp>		->	<aloexp><exptail>
var_record* expr(int & var_num)
{
  var_record*p_factor1=aloexp(var_num);
  var_record*p_factor2=exptail(p_factor1,var_num);
  if(p_factor2==NULL)
    return p_factor1;
  else
    return p_factor2;
}

//<exptail>	->	<cmps><expr>|^
var_record* exptail(var_record*p_factor1,int & var_num)
{
  nextToken();
  if(token==gt||token==ge||token==lt||token==le||token==equ||token==nequ)//正常的算术表达式
  {
   //cmps();
   symbol t=token;

   var_record*p_factor2=expr(var_num);
   return genExp(p_factor1,t,p_factor2,var_num);//生成代码
  }
  else if(token==ident||token==number||token==chara||token==strings||token==lparen)//缺少运算符
  {
    synterror(opplost,-1);
    BACK
    expr(var_num);
  }
  else if(token==semicon||token==rparen||token==comma//一般结束
    ||token==rbrac||token==rsv_return||token==rsv_break||token==rsv_continue||token==rsv_in||token==rsv_out||token==rsv_while||token==rsv_if//意外结束
    ||token==rsv_int||token==rsv_void||token==rsv_char||token==rsv_string
    )
  {
    BACK
    return NULL;
  }
  else
  {
    nextToken();
    if(token==semicon||token==rparen||token==comma)//结束,意外符号
    {
      synterror(oppwrong,0);
      BACK
      return NULL;
    }
    else if(token==ident||token==number||token==chara||token==strings||token==lparen)//缺少运算符
    {
      synterror(oppwrong,0);
      BACK
      expr(var_num);
    }
    else
    {
      synterror(oppwrong,0);
      return NULL;
    }

  }
}
//<cmps>		->	gt|ge|ls|le|equ|nequ
void cmps()
{
  switch(token)
  {
    case gt:
      break;
    case ge:
      break;
    case lt:
      break;
    case le:
      break;
    case equ:
      break;
    case nequ:
      break;
  }
}

//<aloexp>	->	<item><itemtail>
var_record* aloexp(int & var_num)
{
  var_record*p_factor1=item(var_num);
  var_record*p_factor2=itemtail(p_factor1,var_num);
  if(p_factor2==NULL)
    return p_factor1;
  else
    return p_factor2;
}

//<itemtail>	->	<adds><aloexp>|^
var_record* itemtail(var_record*p_factor1,int & var_num)
{
  nextToken();
  if(token==addi||token==subs)//正常项
  {
    //adds();
    symbol t=token;

    var_record*p_factor2=aloexp(var_num);
    return genExp(p_factor1,t,p_factor2,var_num);//生成代码
  }
  else if(token==ident||token==number||token==chara||token==strings||token==lparen)//丢失项运算符
  {
    synterror(opplost,-1);
    BACK
    aloexp(var_num);
  }
  else//可能是项运算符错误，也可能是其他的低级运算符
  {
    //返回交给上级处理，实在不是运算符的话会报错，不需要提前处理
    BACK
    return NULL;
  }
  return NULL;
}
//<adds>		->	add|sub
void adds()
{
  switch(token)
  {
    case addi:
      break;
    case subs:
      break;
  }
}

//<item>		->	<factor><factortail>
var_record* item(int & var_num)
{
  var_record*p_factor1=factor(var_num);
  var_record*p_factor2=factortail(p_factor1,var_num);
  if(p_factor2==NULL)
    return p_factor1;
  else
    return p_factor2;
}

//<factortail>	->	<muls><item>|^
var_record* factortail(var_record*p_factor1,int & var_num)
{
  nextToken();
  if(token==mult||token==divi)//正常因子
  {
    //muls();
    symbol t=token;

    var_record*p_factor2=item(var_num);
    return genExp(p_factor1,t,p_factor2,var_num);//生成代码
  }
  else if(token==ident||token==number||token==chara||token==strings||token==lparen)//丢失因子运算符
  {
    synterror(opplost,-1);
    BACK
    item(var_num);
  }
  else//可能是因子运算符错误，也可能是其他的低级运算符
  {
    //返回交给上级处理，实在不是运算符的话会报错，不需要提前处理
    BACK
    return NULL;
  }
  return NULL;
}
//<muls>		->	mul|div
void muls()
{
  switch(token)
  {
    case mult:
      break;
    case divi:
      break;
  }
}

//<factor>		->	ident<idtail>|number|chara|lparen<expr>rparen|strings
var_record* factor(int & var_num)//临时变量替换方式,运算元素是地址！
{
  nextToken();
  var_record*p_tmpvar=NULL;//记录表达式计算中间结果的临时变量记录的指针
  string refname="";
  switch(token)
  {
    case ident:
      identinexpr=1;
      refname+=id;
      p_tmpvar=idtail(refname,var_num);
      break;
    case number:
      p_tmpvar=tfun.create_tmpvar(rsv_int,1,var_num);
      break;
    case chara:
      p_tmpvar=tfun.create_tmpvar(rsv_char,1,var_num);
      break;
    case lparen:
      p_tmpvar=expr(var_num);
      nextToken();
      if(!match(rparen))
      {
	synterror(exprparenlost,-1);
	BACK
      }
      break;
    case strings:
      p_tmpvar=tfun.create_tmpvar(rsv_string,1,var_num);
      break;
    default:
      if(token==rparen||token==semicon||token==comma||
	token==gt||token==ge||token==lt||token==le||token==equ||token==nequ||
	token==addi||token==subs||token==mult||token==divi
	)
      {
	synterror(exprlost,-1);//表达式丢失
	BACK
      }
      else
      {
	synterror(exprwrong,0);//无效的表达式
      }
  }
  return p_tmpvar;
}
