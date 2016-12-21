#include "semantic.h"
/**  *****************************************************************************************************************************
							    ***语义处理***
  ********************************************************************************************************************************/
string genName(string head,symbol type,string name);
var_record::var_record()//默认构造函数
{
  this->type=null;
  this->name="";
  this->intVal=0;
  this->localAddr=0;
  this->externed=0;
}
void var_record::init(symbol dec_type,string dec_name)//初始化函数
{
  if(synerr!=0)//有语法错误，不处理
    return;
  this->type=dec_type;
  this->name=dec_name;
  this->intVal=0;
  this->localAddr=0;
  this->externed=0;
}
void var_record::copy(const var_record*src)
{
  this->type=src->type;
  this->name=src->name;
  this->intVal=src->intVal;
  this->localAddr=src->localAddr;
  this->externed=src->externed;
}
var_record::var_record(const var_record& v)//拷贝构造函数
{
  this->type=v.type;
  this->name=v.name;
  this->intVal=v.intVal;
  this->localAddr=v.localAddr;
  this->externed=v.externed;
}
var_record::~var_record()
{
  //printf("var was deleted .\n");
}

fun_record::fun_record()//默认构造函数
{
  this->type=null;
  this->name="";
  this->args=new vector<symbol>();
  this->args->clear();
  this->localvars=new vector<var_record*>();
  this->defined=0;
  this->flushed=0;
  this->hadret=0;
}
fun_record::fun_record(const fun_record& f)//拷贝构造函数，仅仅拷贝函数声明信息，其他的信息忽略，代码生成时通过对tfun的管理就能生成局部代码
{
  this->type=f.type;
  this->name=f.name;
  this->args=new vector<symbol>();
  for(int i=0;i<f.args->size();i++)
  {
    args->push_back((*f.args)[i]);
  }
  this->defined=f.defined;
  this->localvars=NULL;
}
void fun_record::init(symbol dec_type,string dec_name)//初始化函数
{
  if(synerr!=0)//有语法错误，不处理
    return;
  this->type=dec_type;
  this->name=dec_name;
  this->args->clear();
  this->localvars->clear();
  this->defined=0;
  this->flushed=0;
  this->hadret=0;
}
void fun_record::addarg()//添加一个参数
{
  if(synerr!=0)//有语法错误，不处理
    return;
  this->args->push_back(tvar.type);
  this->pushlocalvar();//把参数当作变量临时存储在局部列表里边
}
void fun_record::pushlocalvar()//添加一个局部变量
{
  if(synerr!=0)//有语法错误，不处理
    return;
  //局部变量定义的缓冲机制，defined==1之前，是不写入符号表的，因为此时还不能确定是不是函数定义
  if(defined==0)//还是参数声明
  {
    this->localvars->push_back(new var_record(tvar));
//     cout<<"缓冲"<<this->name<<"函数的参数变量"<<symName[tvar.type]<<" "<<tvar.name<<endl;
  }
  else
  {
    if(showTab)
      cout<<"\t\t\t函数 "<<symName[this->type]<<" "<<this->name<<"()\t局部变量 <"<<symName[tvar.type]<<">("<<tvar.name<<")\t进入-->符号表"<<endl;
    var_record *pRec=new var_record(tvar);
    //记录当前的局部变量
    this->localvars->push_back(pRec);
    //把局部变量写入名字表
    table.addvar(pRec);
    //计算地址
    int argslen=this->args->size();
    int localvarlen=this->localvars->size();
    pRec->localAddr=-4*(localvarlen-argslen);//局部变量的地址按照ebp-4*count的方式变化,修改
    //代码中为局部变量开辟临时空间
    genLocvar(0);
  }
}
int fun_record::hasname(string id_name)//防止参数的名字在写入之前重复
{
  if(synerr!=0)//有语法错误，不处理
    return -1;
  int len=this->localvars->size();
  for(int i=0;i<len;i++)
  {
    if((*localvars)[i]->name==id_name)
      return 1;
  }
  return 0;
}
void fun_record::flushargs()//将参数写到符号表
{
  if(synerr!=0)//有语法错误，不处理
    return;
  int argslen=this->args->size();
  //把参数写入名字表
  for(int i=argslen-1;i>=0;i--)
  {
    var_record *pRec=(*localvars)[i];
    pRec->localAddr=4*(i+2);//修改参数地址，参数的地址按照ebp+4*count+4的方式变化
    if(pRec->type==rsv_string)
      pRec->strValId=-1;//参数的类型统一为动态string
    table.addvar(pRec);
    if(showTab)
      cout<<"\t\t\t函数 "<<symName[this->type]<<" "<<this->name<<"()\t参数变量<"<<symName[pRec->type]<<">("<<pRec->name<<")\t进入-->符号表"<<endl;
  }
  flushed=1;
}
void fun_record::poplocalvars(int num)//弹出多个局部变量
{
  if(synerr!=0)//有语法错误，不处理
    return;
  for(int i=0;i<num;i++)//删除记录
  {
    //删除符号表的变量信息
    if((*localvars)[localvars->size()-1]->name[0]!='@')
    {
      if(showTab)
	cout<<"\t\t\t函数 "<<symName[this->type]<<" "<<this->name<<"()\t局部变量<"<<symName[(*localvars)[localvars->size()-1]->type]<<">("<<(*localvars)[localvars->size()-1]->name<<")\t退出<--符号表"<<endl;
    }
    table.delvar((*localvars)[localvars->size()-1]->name);
    this->localvars->pop_back();
  }
  if(num==-1)//函数定义结束
  {
    int argslen=this->args->size();
    int localvarlen=this->localvars->size();
    //删除参数变量
    for(int i=0;i<argslen;i++)
    {
      if(showTab)
	cout<<"\t\t\t函数 "<<symName[this->type]<<" "<<this->name<<"()\t参数变量<"<<symName[(*localvars)[i]->type]<<">("<<(*localvars)[i]->name<<")\t退出<--符号表"<<endl;
      table.delvar((*localvars)[i]->name);
    }
    localvars->clear();
  }
}
int fun_record::equal(fun_record&f)
{
  if(synerr!=0)//有语法错误，不处理
    return -1;
  int flag=1;
  if(args->size()==f.args->size())
  {
   for(int i=0;i<f.args->size();i++)//参数不匹配
   {
    if((*f.args)[i]!=(*args)[i])
    {
      flag=0;
      break;
    }
   }
  }
  else
  	flag=0;
  return(type==f.type&&name==f.name&&flag==1);
}
/**
 * 创建临时变量记录
 * type——变量类型
 * noVal——是否对常量封装
 * var_num——局部变量个数
 */
var_record*fun_record::create_tmpvar(symbol type,int hasVal,int &var_num)
{
  if(synerr!=0)//有错误，不处理./*即使有语意错误也要执行 2012-4-30*/
    return NULL;
  //创建临时变量记录
  var_record*p_tmpvar=new var_record();
  switch(type)
  {
    case rsv_int:
      if(hasVal)
      {
	p_tmpvar->intVal=num;
      }
      break;
    case rsv_char:
      if(hasVal)
      {
	p_tmpvar->charVal=letter;
      }
      break;
    case rsv_string:
      if(hasVal)
      {
	p_tmpvar->strValId=table.addstring();
      }
      else
      {
	p_tmpvar->strValId=-1;//是临时string
      }
      break;
  }
  p_tmpvar->name=genName("tmp",type,"");
  p_tmpvar->type=type;
  //局部变量计数
  var_num++;
  //添加临时变量入栈
  //记录当前的局部变量
  this->localvars->push_back(p_tmpvar);
  //把局部变量写入名字表
  table.addvar(p_tmpvar);//应该可以不写入名字表，但是为了保持变量清除的一致性，作此操作
  //计算地址
  int argslen=this->args->size();
  int localvarlen=this->localvars->size();
  p_tmpvar->localAddr=-4*(localvarlen-argslen);//局部变量的地址按照ebp-4*count的方式变化,修改
//   cout<<"临时变量"<<p_tmpvar->name<<"[地址=ebp"<<p_tmpvar->localAddr<<",类型="<<symName[p_tmpvar->type]<<"]入栈"<<endl;
  //代码中为局部变量开辟临时空间
  genLocvar(p_tmpvar->strValId);
  //返回变量记录的指针
  return p_tmpvar;
}
/**
 * 取得上一个进栈的变量的地址（esp相对于ebp偏移）
 */
int fun_record::getCurAddr()
{
  //计算地址
  int argslen=this->args->size();
  int localvarlen=this->localvars->size();
  return -4*(localvarlen-argslen);
}
fun_record::~fun_record()
{
  //printf("fun was deleted .\n");
  this->type=null;
  this->name="";

  if(localvars!=NULL)
  {
    if(flushed==0)//参数还在缓冲区，必须清除
    {
      int len=args->size();
      for(int i=0;i<len;i++)
      {
// 	cout<<"参数"<<(*localvars)[i]->name<<"被紧急删除"<<endl;
	delete (*localvars)[i];
      }
    }
    //其他的就不管理了，在map里边清除
    this->localvars->clear();
    delete localvars;
  }
  this->args->clear();
  delete this->args;
  this->defined=0;
}

Table::Table()
{
  var_map.clear();
  fun_map.clear();
  stringTable.clear();
  real_args_list.clear();
}
/**
 * 为当前的函数调用代码参加一个实际参数的记录，不检验合法性，最后一块检查
 */
void Table::addrealarg(var_record*arg,int&var_num)
{
  if(synerr!=0)//有语法错误，不处理
    return ;
  if(arg->type==rsv_string)
  {
    //if(ret->strValId!=-1)//不在栈中
    {//强制产生副本
      var_record empstr;
      string empname="";
      empstr.init(rsv_string,empname);
      arg=genExp(&empstr,addi,arg,var_num);
    }
  }
  real_args_list.push_back(arg);
}
void sp(const char* msg);
var_record* Table::genCall(string fname,int& var_num)
{
  var_record*pRec=NULL;
  if(errorNum!=0)//有错误，不处理
     return NULL;
  sp("对函数调用规则进行语义检查");
  if(fun_map.find(fname)!=fun_map.end())//有函数声明，就可以调用
  {
    fun_record*pfun=fun_map[fname];
    //匹配函数的参数
    //2012-6-16 ：实参列表是共用的，因此需要动态维护
    if(real_args_list.size()>=pfun->args->size())//实参个数足够时
    {
      int l=real_args_list.size();
      int m=pfun->args->size();
      for(int i=l-1,j=m-1;j>=0;i--,j--)
      {
				if(real_args_list[i]->type!=(*(pfun->args))[j])
				{
					semerror(real_args_err);
					break;
				}
				else
				{
					//产生参数进栈代码
					var_record*ret=real_args_list[i];
					if(semerr!=0)//有错误，不生成
						break ;
					if(ret->type==rsv_string)
					{
						fprintf(fout,"\tmov eax,[ebp%d]\n",ret->localAddr);//将副本字符串的地址放在eax中
					}
					else
					{
						if(ret->localAddr==0)//全局的
							fprintf(fout,"\tmov eax,[@var_%s]\n",ret->name.c_str());
						else//局部的
						{
							if(ret->localAddr<0)
								fprintf(fout,"\tmov eax,[ebp%d]\n",ret->localAddr);
							else
								fprintf(fout,"\tmov eax,[ebp+%d]\n",ret->localAddr);
						}
					}
					fprintf(fout,"\tpush eax\n");
				}
      }
      if(showGen)
				cout<<"产生函数"<<fname<<"()的调用代码"<<endl;
      //产生函数调用代码
      fprintf(fout,"\tcall %s\n",fname.c_str());
      fprintf(fout,"\tadd esp,%d\n",4*l);
      //产生函数返回代码
      if(pfun->type!=rsv_void)//非void函数在函数返回的时候将eax的数据放到临时变量中，为调用代码使用
      {
				pRec=tfun.create_tmpvar(pfun->type,0,var_num);//创建临时变量
				fprintf(fout,"\tmov [ebp%d],eax\n",pRec->localAddr);
				if(pfun->type==rsv_string)//返回的是临时string，必须拷贝
				{
					var_record empstr;
					string empname="";
					empstr.init(rsv_string,empname);
					//src=genExp(src,addi,&empstr,var_num);
					pRec=genExp(&empstr,addi,pRec,var_num);
				}
      }
      //清除实际参数
      while(m--)
      	real_args_list.pop_back();
    }
    else
    {
      semerror(real_args_err);
    }
  }
  else
  {
    semerror(fun_undec);
  }
  
  return pRec;
}
int stringId=0;//串空间标志串的Id——唯一
int Table::addstring()//返回串空间的索引,仅仅是记录串值其他符号串自动存储
{
  if(synerr!=0)//有语法错误，不处理
    return 0;
  stringId++;
  string * ps=new string();
  *ps+=str;
  stringTable.push_back(ps);
  if(showTab)
    cout<<"\t\t\t串 "<<"\""<<str<<"\""<<"进入-->串空间,id="<<stringId<<endl;
  return stringId;
}
string Table::getstring(int index)
{
  if(synerr!=0)//有语法错误，不处理
    return "";
  if(index>0&&index<=stringTable.size())
  {
    return *(stringTable[index-1]);
  }
  else
    return "";
}
//添加变量声明记录,默认使用tvar
void Table::addvar()
{
  if(synerr!=0)//有语法错误，不处理
    return;
  if(var_map.find(tvar.name)==var_map.end())//不存在重复记录
  {
    var_record * pvar=new var_record(tvar);
    var_map[tvar.name]=pvar;//插入tvar 信息到堆中
    if(showTab)
      cout<<"\t\t\t全局变量 <"<<symName[tvar.type]<<">("<<tvar.name<<")\t进入-->符号表"<<endl;
  }
  else//存在记录，看看是不是已经声明的外部变量
  {
    sp("对全局变量的定义行进行语义检查");
    var_record * pvar=var_map[tvar.name];
    //刷新变量记录信息
    delete var_map[tvar.name];
    var_map[tvar.name]=pvar;//插入tvar 信息到堆中
    semerror(var_redef);
  }
}
//添加变量声明记录,重载——为添加局部变量和参数变量提供的
void Table::addvar(var_record*v_r)
{
  if(synerr!=0)//有语法错误，不处理
    return;
  if(var_map.find(v_r->name)==var_map.end())//不存在重复记录
  {
    var_map[v_r->name]=v_r;//插入v_r到堆中
  }
  else//一般不会执行，因为全局变量添加用tvar,其他的类型错误都被hasname()屏蔽掉了
  {
     semerror(var_redef);
     delete v_r;//删除重复信息
  }
}
var_record * Table::getVar(string name)
{
  if(synerr!=0)//有语法错误，不处理
    return NULL;
  sp("对引用全局变量进行语义检查");
  if(table.hasname(name))//有函数声明，就可以调用
  {
    return var_map[name];
  }
  else
  {
    semerror(var_undec);
    return NULL;
  }
}
int Table::hasname(string id_name)//测试局部变量，参数的名字是否重复，主要应对变量的重复定义，全局变量和函数不需要调用他
{
  if(synerr!=0)//有语法错误，不处理
    return -1;
  return (var_map.find(id_name)!=var_map.end())||(tfun.hasname(id_name));
}
void Table::delvar(string var_name)//删除变量记录
{
  if(synerr!=0)//有语法错误，不处理
    return;
  if(var_map.find(var_name)!=var_map.end())//有记录
  {
    var_record * pvar=var_map[var_name];
    delete pvar;
    var_map.erase(var_name);
  }
  else
  {
    //cout<<"删除的变量名成不存在！"<<endl;
  }
}
//添加函数声明记录,默认使用tfun
void Table::addfun()
{
  if(synerr!=0)//有语法错误，不处理
    return;
   sp("对函数的声明定义行进行语义检查");
  if(fun_map.find(tfun.name)==fun_map.end())//不存在记录《不管是否定义都插入，要是函数定义则defined字段已经设置过了》
  {
    fun_record * pfun=new fun_record(tfun);
    fun_map[tfun.name]=pfun;//插入tfun 信息到堆中
    if(showTab)
      cout<<"\t\t\t函数 "<<symName[tfun.type]<<" "<<tfun.name<<"()\t进入-->符号表"<<endl;
    //函数定义生成代码
    if(pfun->defined==1)
    {
      tfun.flushargs();
      genFunhead();
    }
  }
  else//函数声明过了
  {
    fun_record * pfun=fun_map[tfun.name];//取得之前声明的函数信息
    //验证函数声明
    if(pfun->equal(tfun))
    {
      //参数匹配,正常声明
      if(tfun.defined==1)//添加函数定义
      {
      	if(pfun->defined==1)//已经定义了
				{
					//重复定义错误,覆盖原来的定义，防止后边的逻辑错误
					semerror(fun_redef);
					//2012-6-16:由于函数形式完全相同，因此不需要更改函数记录内容，只需要刷新参数即可					
					tfun.flushargs();
				}
      	else
      	{
      	 //正式的函数定义
      	 pfun->defined=1;//标记函数定义
      	 tfun.flushargs();
      	 //函数定义生成函数头代码
      	 genFunhead();
      	}
      }
      return ;
    }
    else
    {
      //插入新的定义声明
      fun_record * pfun=new fun_record(tfun);
      delete fun_map[tfun.name];//删除旧的函数记录
      fun_map[tfun.name]=pfun;//插入tfun 信息到堆中
      //参数声明不一致++
      if(tfun.defined==1)//定义和声明不一致
      {
				semerror(fun_def_err);
				tfun.flushargs();
      }
      else//多次声明不一致
      {
				semerror(fun_dec_err);
      }
    }
  }
}
void Table::over()//进行最后的处理
{
  if(errorNum!=0)
    return;
  if(showGen)
    cout<<"生成数据段中的静态数据区、文字池和辅助栈"<<endl;
  hash_map<string, var_record*, string_hash>::iterator var_i,var_iend=var_map.end();
  //生成静态数据-全局变量
  fprintf(fout,"section .data\n");
  for(var_i=var_map.begin();var_i!=var_iend;var_i++)
  {
    var_record *p_v=var_i->second;
    int isEx=0;
    if(p_v->externed)
    {
    	isEx=1;//标示外部变量，此处不进行输出extern
    }
    else//对extern变量不输出
    {
		  if(p_v->type==rsv_string)
		  {
		    fprintf(fout,"\t%s times 255 db %d\n",genName("str",null,p_v->name).c_str(),isEx);//字符串变量初始化为全0,长度255B
		    fprintf(fout,"\t%s_len db %d\n",genName("str",null,p_v->name).c_str(),isEx);//记录实际长度0
		  }
		  else
		  {
		    fprintf(fout,"\t%s dd %d\n",genName("var",null,p_v->name).c_str(),isEx);//普通变量初始化为0
		  }
    }
  }
  //生成文字池
  char strbuf[255];
  int l;
  for(int i=0;i<stringTable.size();i++)
  {
     strcpy(strbuf,stringTable[i]->c_str());
     l=stringTable[i]->length();
     fprintf(fout,"\t@str_%d db ",i+1);
     int chpass=0;
     for(int j=0;j<l;j++)
     {
	if(strbuf[j]==10||strbuf[j]==9||strbuf[j]=='\"')//\n \t
	{
	    if(chpass==0)
	    {
	      if(j!=0)//不是第一个
		fprintf(fout,",");
	      fprintf(fout,"%d",strbuf[j]);
	    }
	    else
	      fprintf(fout,"\",%d",strbuf[j]);
	    chpass=0;
	}
       else
       {
	 if(chpass==0)
	 {
	   if(j!=0)
	     fprintf(fout,",");
	   fprintf(fout,"\"%c",strbuf[j]);
	   if(j==l-1)
	      fprintf(fout,"\"");
	 }
	 else
	 {
	   fprintf(fout,"%c",strbuf[j]);
	   if(j==l-1)
	     fprintf(fout,"\"");
	 }
	 chpass=1;
       }
     }
     if(l==0)
     	fprintf(fout,"\"\"");
     fprintf(fout,"\n");
    //fprintf(fout,"\t@str_%d db \"%s\"\n",i+1,stringTable[i]->c_str());
    fprintf(fout,"\t@str_%d_len equ %d\n",i+1,(int)stringTable[i]->length());
  }
  

}
void Table::clear()//清空所有符号表
{
  //删除var记录
  hash_map<string, var_record*, string_hash>::iterator var_i,var_iend;
  var_iend=var_map.end();
  for(var_i=var_map.begin();var_i!=var_iend;var_i++)
  {
//     cout<<"删除变量记录"<<var_i->second->name<<endl;
    delete var_i->second;//删除记录对象
  }
  var_map.clear();
  //删除fun记录
  hash_map<string, fun_record*, string_hash>::iterator fun_i,fun_iend;
  fun_iend=fun_map.end();
  for(fun_i=fun_map.begin();fun_i!=fun_iend;fun_i++)
  {
//     cout<<"删除函数记录"<<fun_i->second->name<<endl;
    delete fun_i->second;//删除记录对象
  }
  fun_map.clear();
  //删除串空间
  for(int i=0;i<stringTable.size();i++)
  {
//     cout<<"删除串"<<*stringTable[i]<<endl;
    delete stringTable[i];
  }
  stringTable.clear();
//   cout<<"清除成功！"<<endl;
  //删除实参列表
  real_args_list.clear();
}
Table::~Table()//注销所有空间
{
  this->clear();
}

var_record tvar;//临时变量声明临时记录
fun_record tfun;//临时函数声明临时记录
Table table;//符号表全局对象


