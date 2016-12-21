#include "common.h"
#include "semantic.h"
#include <iomanip>
int convert_buffer=0;//标志是不是对缓冲区的数据进行操作
/**  *****************************************************************************************************************************
                                                             ***代码生成*** ********************************************************************************************************************************/
/**
 * 产生目标代码中的唯一的标签名
 * head——标题：tmp,var,str,fun,lab
 * type——类型: rsv_char,rsv_int,rsv_string
 * name——名称
 */
int ID=0;
string genName(string head,symbol type,string name)
{
  ID++;
  string retStr="@"+head;
  if(type!=null)
  {
    retStr+="_";
    retStr+=symName[type];
  }
  if(name!="")
  {
    retStr+="_";
    retStr+=name;
  }
  if(head!="str"&&head!="fun"&&head!="var")
  {
    retStr+="_";
    char strID[10];
    sprintf(strID, "%d",ID);
    retStr+=strID;
  }
  return retStr;
}
void sp(const char* msg);
/**
 * 产生表达式代码
 * 运算规则，字符可以当作整数进行运算，string只能加法运算
 * 有string类型的操作数结果是string，否则就是int
 * 参数说明：
 * 	p_factor1——左操作数
 * 	token——运算符
 * 	p_factor2——右操作数
 * 	var_num——复合语句里的变量个数
 */
var_record* genExp(var_record*p_factor1,symbol opp,var_record*p_factor2,int &var_num)
{
  if(errorNum!=0)//有错误，不生成
    return NULL;
  if(p_factor1==NULL||p_factor2==NULL)
    return NULL;
  sp("对表达式操作数类型进行语义检查");
  //单独处理void
    if(p_factor1->type==rsv_void||p_factor2->type==rsv_void)
     {
      //void不能参加运算
    	semerror(void_ncal);
      	return NULL;
     }
  int rsl_type=rsv_int;//默认int
  /*if(opp==gt||opp==ge||opp==lt||opp==le||opp==equ||opp==nequ)
  {
  	if(p_factor1->type==rsv_string^p_factor2->type==rsv_string)
  	{
  		//字符串不能和非字符串比较的运算
    		semerror(str_ncmp);
    		return NULL;
  	}
  	rsl_type=rsv_char;
  }
  else
  {
  	if((p_factor1->type==rsv_string||p_factor2->type==rsv_string)&&opp!=addi)
  	{
  		//字符串不能进行加法，比较以外的运算
    		semerror(str_nadd);
    		return NULL;
  	}
  	if((p_factor1->type==rsv_string||p_factor2->type==rsv_string)&&opp==addi)
  	{
  		rsl_type=rsv_string;
  	}
  	else
  	{
  		rsl_type=rsv_int;
  	}
  }*/
    if(p_factor1->type==rsv_string||p_factor2->type==rsv_string)
    {
      if(opp==addi)
      {
	rsl_type=rsv_string;
      }
      else
      {
	semerror(str_nadd);
	return NULL;
      }
    }
    else
    {
      if(opp==gt||opp==ge||opp==lt||opp==le||opp==equ||opp==nequ)
      {
	rsl_type=rsv_char;
      }
    }
    if(showGen)
      cout<<"生成表达式<"<<symName[p_factor1->type]<<"> "<<symName[opp]<<" <"<<symName[p_factor2->type]<<"> => <"<<symName[rsl_type]<<">"<<endl;
    var_record * pRec=tfun.create_tmpvar(symbol(rsl_type),0,var_num);//创建临时变量\t
    string labLop,labExt;
    switch(rsl_type)
    {
      case rsv_string://字符串连接运算
      	//cout<<"字符串链接运算"<<endl;
	if(p_factor2->type==rsv_string)
	{
	  labLop=genName("lab",null,"cpystr2");
	  labExt=genName("lab",null,"cpystr2_exit");
	  if(p_factor2->strValId==-1)//动态string
	  {
	    fprintf(fout,";----------生成动态string%s的代码----------\n",p_factor2->name.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	    if(p_factor2->localAddr<0)
	      fprintf(fout,"\tmov ebx,[ebp%d]\n\tmov eax,0\n\tmov al,[ebx]\n",p_factor2->localAddr);
	    else
	      fprintf(fout,"\tmov ebx,[ebp+%d]\n\tmov eax,0\n\tmov al,[ebx]\n",p_factor2->localAddr);
	    fprintf(fout,"\tsub esp,1\n\tmov [esp],al;长度压入后再压入数据栈\n");
	    fprintf(fout,"\tmov [ebp%d],esp\n",pRec->localAddr);//存入数据指针

	    fprintf(fout,"\tcmp eax,0\n");
	    fprintf(fout,"\tje %s\n",labExt.c_str());
	    fprintf(fout,"\tmov ecx,0\n");
	    fprintf(fout,"\tmov esi,ebx\n\tsub esi,1\n");
	    fprintf(fout,"\tneg eax\n");
	    fprintf(fout,"%s:\n",labLop.c_str());
	    fprintf(fout,"\tcmp ecx,eax\n");
	    fprintf(fout,"\tje %s\n",labExt.c_str());
	    fprintf(fout,"\tmov dl,[esi+ecx]\n");
	    fprintf(fout,"\tsub esp,1\n\tmov [esp],dl\n");
	    fprintf(fout,"\tdec ecx\n");
	    fprintf(fout,"\tjmp %s\n",labLop.c_str());
	    fprintf(fout,"%s:\n",labExt.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	  }
	  else if(p_factor2->strValId>0)//常量string
	  {
	    fprintf(fout,";----------生成常量string%s的代码----------\n",p_factor2->name.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	    fprintf(fout,"\tmov eax,@str_%d_len\n\tsub esp,1\n\tmov [esp],al;长度压入后再压入数据栈\n",p_factor2->strValId);
	    fprintf(fout,"\tmov [ebp%d],esp\n",pRec->localAddr);//存入数据指针

	    fprintf(fout,"\tcmp eax,0\n");//测试长度是否是0
	    fprintf(fout,"\tje %s\n",labExt.c_str());
	    fprintf(fout,"\tmov ecx,@str_%d_len\n\tdec ecx\n",p_factor2->strValId);//
	    fprintf(fout,"\tmov esi,@str_%d\n",p_factor2->strValId);//取得首地址
	    fprintf(fout,"%s:\n",labLop.c_str());
	    fprintf(fout,"\tcmp ecx,-1\n");
	    fprintf(fout,"\tje %s\n",labExt.c_str());
	    fprintf(fout,"\tmov al,[esi+ecx]\n");
	    fprintf(fout,"\tsub esp,1\n\tmov [esp],al\n");
	    fprintf(fout,"\tdec ecx\n");
	    fprintf(fout,"\tjmp %s\n",labLop.c_str());
	    fprintf(fout,"%s:\n",labExt.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	  }
	  else if(p_factor2->strValId==-2)//全局string
	  {
	    fprintf(fout,";----------生成全局string%s的代码----------\n",p_factor2->name.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	    if(convert_buffer==0)
	      fprintf(fout,"\tmov eax,0\n\tmov al,[@str_%s_len]\n\tsub esp,1\n\tmov [esp],al;长度压入后再压入数据栈\n",p_factor2->name.c_str());
	    else
	      fprintf(fout,"\tmov eax,0\n\tmov al,[%s_len]\n\tsub esp,1\n\tmov [esp],al;长度压入后再压入数据栈\n",p_factor2->name.c_str());
	    fprintf(fout,"\tmov [ebp%d],esp\n",pRec->localAddr);//存入数据指针

	    fprintf(fout,"\tcmp eax,0\n");//测试长度是否是0
	    fprintf(fout,"\tje %s\n",labExt.c_str());
	    fprintf(fout,"\tsub eax,1\n\tmov ecx,eax\n");
	    if(convert_buffer==0)
	      fprintf(fout,"\tmov esi,@str_%s\n",p_factor2->name.c_str());//取得首地址
	    else
	      fprintf(fout,"\tmov esi,%s\n",p_factor2->name.c_str());//取得首地址
	    fprintf(fout,"%s:\n",labLop.c_str());
	    fprintf(fout,"\tcmp ecx,-1\n");
	    fprintf(fout,"\tje %s\n",labExt.c_str());
	    fprintf(fout,"\tmov al,[esi+ecx]\n");
	    fprintf(fout,"\tsub esp,1\n\tmov [esp],al\n");
	    fprintf(fout,"\tdec ecx\n");
	    fprintf(fout,"\tjmp %s\n",labLop.c_str());
	    fprintf(fout,"%s:\n",labExt.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	  }
	  else if(p_factor2->strValId==0)
	  {
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	    fprintf(fout,"\tmov eax,0\n\tsub esp,1\n\tmov [esp],al;长度压入后再压入数据栈\n");
	    fprintf(fout,"\tmov [ebp%d],esp\n",pRec->localAddr);//存入数据指针
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	  }
	}
	else if(p_factor2->type==rsv_int) //数字
	{
	  labLop=genName("lab",null,"numtostr2");
	  labExt=genName("lab",null,"numtostr2_exit");
	  string labNumSign=genName("lab",null,"numsign2");
	  string labNumSignExt=genName("lab",null,"numsign2_exit");
	  fprintf(fout,";----------生成number%s的string代码----------\n",p_factor2->name.c_str());
	  fprintf(fout,"\
			    \tmov eax,[@s_esp]\n\
			    \tmov [@s_esp],esp\n\
			    \tmov esp,eax\n");
	  if(p_factor2->localAddr==0)//全局的
	  {
	    fprintf(fout,"\tmov eax,[@var_%s]\n",p_factor2->name.c_str());
	  }
	  else//局部的
	  {
	    if(p_factor2->localAddr<0)
	      fprintf(fout,"\tmov eax,[ebp%d]\n",p_factor2->localAddr);
	    else
	      fprintf(fout,"\tmov eax,[ebp+%d]\n",p_factor2->localAddr);
	  }
	  fprintf(fout,"\
			    \tsub esp,1;先把数字的长度位置空出来\n\
			    \tmov ecx,0\n\tmov [esp],cl\n\
			    \tmov esi,esp\n");
	  fprintf(fout,"\tmov [ebp%d],esp\n",pRec->localAddr);//存入数据指针
	  //确定数字的正负
	  fprintf(fout,"\tmov edi,0\n");//保存eax符号：0+ 1-
	  fprintf(fout,"\tcmp eax,0\n");
	  fprintf(fout,"\tjge %s\n",labNumSignExt.c_str());
	  fprintf(fout,"%s:\n",labNumSign.c_str());
	  fprintf(fout,"\tneg eax\n");
	  fprintf(fout,"\tmov edi,1\n");
	  fprintf(fout,"%s:\n",labNumSignExt.c_str());

	  fprintf(fout,"\tmov ebx,10\n");
	  fprintf(fout,"%s:\n",labLop.c_str());
	  fprintf(fout,"\
			    \tmov edx,0\n\
			    \tidiv ebx\n\
			    \tmov cl,[esi]\n\
			    \tinc cl\n\
			    \tmov [esi],cl\n\
			    \tsub esp,1\n\
			    \tadd dl,48\n\
			    \tmov [esp],dl\n\
			    \tcmp eax,0\n");
	  fprintf(fout,"\tjne %s\n",labLop.c_str());

	  fprintf(fout,"\tcmp edi,0\n");
	  fprintf(fout,"\tje %s\n",labExt.c_str());
	  fprintf(fout,"\tsub esp,1\n\tmov ecx,%d\n\tmov [esp],cl\n",'-');
	  fprintf(fout,"\tmov cl,[esi]\n\tinc cl\n\tmov [esi],cl\n");
	  fprintf(fout,"%s:\n",labExt.c_str());
	  fprintf(fout,"\
			    \tmov eax,[@s_esp]\n\
			    \tmov [@s_esp],esp\n\
			    \tmov esp,eax\n");
	}
	else if(p_factor2->type==rsv_char)
	{
	  fprintf(fout,";----------生成char%s的string代码----------\n",p_factor2->name.c_str());
	  fprintf(fout,"\
			    \tmov eax,[@s_esp]\n\
			    \tmov [@s_esp],esp\n\
			    \tmov esp,eax\n");
	  if(p_factor2->localAddr==0)//全局的
	  {
	    fprintf(fout,"\tmov eax,[@var_%s]\n",p_factor2->name.c_str());
	  }
	  else//局部的
	  {
	    if(p_factor2->localAddr<0)
	      fprintf(fout,"\tmov eax,[ebp%d]\n",p_factor2->localAddr);
	    else
	      fprintf(fout,"\tmov eax,[ebp+%d]\n",p_factor2->localAddr);
	  }
	  fprintf(fout,"\tsub esp,1\n\tmov bl,1\n\tmov [esp],bl\n\tmov [ebp%d],esp\n",pRec->localAddr);//存入数据指针
	  fprintf(fout,"\tsub esp,1\n\tmov [esp],al\n");
	  fprintf(fout,"\
			    \tmov eax,[@s_esp]\n\
			    \tmov [@s_esp],esp\n\
			    \tmov esp,eax\n");
	}
	fprintf(fout,";--------------------------------------------------\n");

	if(p_factor1->type==rsv_string)
	{
	  labLop=genName("lab",null,"cpystr1");
	  labExt=genName("lab",null,"cpystr1_exit");
	  if(p_factor1->strValId==-1)//动态string
	  {
	    fprintf(fout,";----------生成动态string%s的代码----------\n",p_factor1->name.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	    if(p_factor1->localAddr<0)
		    fprintf(fout,"\tmov ebx,[ebp%d]\n\tmov eax,0\n\tmov al,[ebx]\n",p_factor1->localAddr);
		  else
		  	fprintf(fout,"\tmov ebx,[ebp+%d]\n\tmov eax,0\n\tmov al,[ebx]\n",p_factor1->localAddr);
	    fprintf(fout,"\tcmp eax,0\n");
	    fprintf(fout,"\tje %s\n",labExt.c_str());

	    fprintf(fout,"\tmov ebx,[ebp%d];\n",pRec->localAddr);//将结果字符串的长度追加

	    fprintf(fout,"\tmov edx,0\n\tmov dl,[ebx]\n");
	    fprintf(fout,"\tadd edx,eax\n");
	    fprintf(fout,"\tmov [ebx],dl\n");

	    fprintf(fout,"\tmov ecx,0\n");
	    if(p_factor1->localAddr<0)
	      fprintf(fout,"\tmov esi,[ebp%d]\n\tsub esi,1\n",p_factor1->localAddr);//消除偏移
	    else
	      fprintf(fout,"\tmov esi,[ebp+%d]\n\tsub esi,1\n",p_factor1->localAddr);//消除偏移
	    fprintf(fout,"\tneg eax\n");
	    //仅仅是测试字符串总长是否超过255，超出部分忽略
	    fprintf(fout,"\tcmp edx,255\n");
	    fprintf(fout,"\tjna %s\n",labLop.c_str());
	    fprintf(fout,"\tcall @str2long\n");

	    fprintf(fout,"%s:\n",labLop.c_str());
	    fprintf(fout,"\tcmp ecx,eax\n");
	    fprintf(fout,"\tje %s\n",labExt.c_str());
	    fprintf(fout,"\tmov dl,[esi+ecx]\n");
	    fprintf(fout,"\tsub esp,1\n\tmov [esp],dl\n");
	    fprintf(fout,"\tdec ecx\n");
	    fprintf(fout,"\tjmp %s\n",labLop.c_str());
	    fprintf(fout,"%s:\n",labExt.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	  }
	  else if(p_factor1->strValId>0)//常量string
	  {
	    fprintf(fout,";----------生成常量string%s的代码----------\n",p_factor1->name.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	    fprintf(fout,"\tmov eax,@str_%d_len\n",p_factor1->strValId);
	    fprintf(fout,"\tcmp eax,0\n");

	    fprintf(fout,"\tje %s\n",labExt.c_str());
	    fprintf(fout,"\tmov ebx,[ebp%d];\n",pRec->localAddr);//将结果字符串的长度追加
	    fprintf(fout,"\tmov edx,0\n\tmov dl,[ebx]\n");
	    fprintf(fout,"\tadd edx,eax\n");
	    fprintf(fout,"\tmov [ebx],dl\n");

	    fprintf(fout,"\tmov ecx,@str_%d_len\n\tdec ecx\n",p_factor1->strValId);
	    fprintf(fout,"\tmov esi,@str_%d\n",p_factor1->strValId);
	    //仅仅是测试字符串总长是否超过255，超出报错
	    fprintf(fout,"\tcmp edx,255\n");
	    fprintf(fout,"\tjna %s\n",labLop.c_str());
	    fprintf(fout,"\tcall @str2long\n");

	    fprintf(fout,"%s:\n",labLop.c_str());
	    fprintf(fout,"\tcmp ecx,-1\n");
	    fprintf(fout,"\tje %s\n",labExt.c_str());
	    fprintf(fout,"\tmov al,[esi+ecx]\n");
	    fprintf(fout,"\tsub esp,1\n\tmov [esp],al\n");
	    fprintf(fout,"\tdec ecx\n");
	    fprintf(fout,"\tjmp %s\n",labLop.c_str());
	    fprintf(fout,"%s:\n",labExt.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	  }
	  else if(p_factor1->strValId==-2)//全局string
	  {
	    fprintf(fout,";----------生成全局string%s的代码----------\n",p_factor1->name.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	    if(convert_buffer==0)
	      fprintf(fout,"\tmov eax,0\n\tmov al,[@str_%s_len]\n",p_factor1->name.c_str());
	    else
	      fprintf(fout,"\tmov eax,0\n\tmov al,[%s_len]\n",p_factor1->name.c_str());
	    fprintf(fout,"\tcmp eax,0\n");

	    fprintf(fout,"\tje %s\n",labExt.c_str());
	    fprintf(fout,"\tmov ebx,[ebp%d];\n",pRec->localAddr);//将结果字符串的长度追加
	    fprintf(fout,"\tmov edx,0\n\tmov dl,[ebx]\n");
	    fprintf(fout,"\tadd edx,eax\n");
	    fprintf(fout,"\tmov [ebx],dl\n");

	    fprintf(fout,"\tsub eax,1\n\tmov ecx,eax\n");
	    if(convert_buffer==0)
	      fprintf(fout,"\tmov esi,@str_%s\n",p_factor1->name.c_str());
	    else
	      fprintf(fout,"\tmov esi,%s\n",p_factor1->name.c_str());
	    //仅仅是测试字符串总长是否超过255，超出报错
	    fprintf(fout,"\tcmp edx,255\n");
	    fprintf(fout,"\tjna %s\n",labLop.c_str());
	    fprintf(fout,"\tcall @str2long\n");

	    fprintf(fout,"%s:\n",labLop.c_str());
	    fprintf(fout,"\tcmp ecx,-1\n");
	    fprintf(fout,"\tje %s\n",labExt.c_str());
	    fprintf(fout,"\tmov al,[esi+ecx]\n");
	    fprintf(fout,"\tsub esp,1\n\tmov [esp],al\n");
	    fprintf(fout,"\tdec ecx\n");
	    fprintf(fout,"\tjmp %s\n",labLop.c_str());
	    fprintf(fout,"%s:\n",labExt.c_str());
	    fprintf(fout,"\tmov eax,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,eax\n");//esp<=>[@s_esp]
	  }
	}
	else if(p_factor1->type==rsv_int) //数字
	{
	  labLop=genName("lab",null,"numtostr1");
	  labExt=genName("lab",null,"numtostr1_exit");
	  string labNumSign=genName("lab",null,"numsign1");
	  string labNumSignExt=genName("lab",null,"numsign1_exit");
	  string lab2long=genName("lab",null,"numsign1_add");
	  fprintf(fout,";----------生成number%s的string代码----------\n",p_factor1->name.c_str());
	  fprintf(fout,"\
			    \tmov eax,[@s_esp]\n\
			    \tmov [@s_esp],esp\n\
			    \tmov esp,eax\n");
	  if(p_factor1->localAddr==0)//全局的
	  {
	    fprintf(fout,"\tmov eax,[@var_%s]\n",p_factor1->name.c_str());
	  }
	  else//局部的
	  {
	    if(p_factor1->localAddr<0)
	      fprintf(fout,"\tmov eax,[ebp%d]\n",p_factor1->localAddr);
	    else
	      fprintf(fout,"\tmov eax,[ebp+%d]\n",p_factor1->localAddr);
	  }
	  fprintf(fout,"\tmov esi,[ebp%d];\n",pRec->localAddr);//将临时字符串的长度地址记录下来

	  //确定数字的正负
	  fprintf(fout,"\tmov edi,0\n");//保存eax符号：0+ 1-
	  fprintf(fout,"\tcmp eax,0\n");
	  fprintf(fout,"\tjge %s\n",labNumSignExt.c_str());
	  fprintf(fout,"%s:\n",labNumSign.c_str());
	  fprintf(fout,"\tneg eax\n");
	  fprintf(fout,"\tmov edi,1\n");
	  fprintf(fout,"%s:\n",labNumSignExt.c_str());

	  //累加长度，压入数据
	  fprintf(fout,"\tmov ebx,10\n");
	  fprintf(fout,"%s:\n",labLop.c_str());
	  fprintf(fout,"\
			    \tmov edx,0\n\
			    \tidiv ebx\n\
			    \tmov cl,[esi]\n\
			    \tinc cl\n\
			    \tmov [esi],cl\n\
			    \tsub esp,1\n\
			    \tadd dl,48\n\
			    \tmov [esp],dl\n\
			    \tcmp eax,0\n");
	  fprintf(fout,"\tjne %s\n",labLop.c_str());

	  //添加符号
	  fprintf(fout,"\tcmp edi,0\n");
	  fprintf(fout,"\tje %s\n",lab2long.c_str());
	  fprintf(fout,"\tsub esp,1\n\tmov ecx,%d\n\tmov [esp],cl\n",'-');
	  fprintf(fout,"\tmov cl,[esi]\n\tinc cl\n\tmov [esi],cl\n");

	  fprintf(fout,"%s:\n",lab2long.c_str());
	  //仅仅是测试字符串总长是否超过255，超出报错
	  fprintf(fout,"\tcmp cl,255\n");
	  fprintf(fout,"\tjna %s\n",labExt.c_str());
	  fprintf(fout,"\tcall @str2long\n");
	  fprintf(fout,"%s:\n",labExt.c_str());
	  fprintf(fout,"\
			    \tmov eax,[@s_esp]\n\
			    \tmov [@s_esp],esp\n\
			    \tmov esp,eax\n");
	}
	else if(p_factor1->type==rsv_char)
	{
	  labExt=genName("lab",null,"chtostr2_exit");
	  fprintf(fout,";----------生成char%s的string代码----------\n",p_factor1->name.c_str());
	  fprintf(fout,"\
			    \tmov eax,[@s_esp]\n\
			    \tmov [@s_esp],esp\n\
			    \tmov esp,eax\n");
	  if(p_factor1->localAddr==0)//全局的
	  {
	    fprintf(fout,"\tmov eax,[@var_%s]\n",p_factor1->name.c_str());
	  }
	  else//局部的
	  {
	    if(p_factor1->localAddr<0)
	      fprintf(fout,"\tmov eax,[ebp%d]\n",p_factor1->localAddr);
	    else
	      fprintf(fout,"\tmov eax,[ebp+%d]\n",p_factor1->localAddr);
	  }
	  fprintf(fout,"\tmov esi,[ebp%d];\n",pRec->localAddr);//将临时字符串的长度地址记录下来

	  //累加长度，压入数据
	  fprintf(fout,"\
			    \tmov cl,[esi]\n\
			    \tinc cl\n\
			    \tmov [esi],cl\n\
			    \tsub esp,1\n\
			    \tmov [esp],al\n");

	  //仅仅是测试字符串总长是否超过255，超出报错
	  fprintf(fout,"\tcmp cl,255\n");
	  fprintf(fout,"\tjna %s\n",labExt.c_str());
	  fprintf(fout,"\tcall @str2long\n");
	  fprintf(fout,"%s:\n",labExt.c_str());
	  fprintf(fout,"\
			    \tmov eax,[@s_esp]\n\
			    \tmov [@s_esp],esp\n\
			    \tmov esp,eax\n");
	}
	fprintf(fout,";--------------------------------------------------\n");


	break;
      case rsv_int://需要考虑+ - * / 类型：int 算术运算
            	//cout<<"算术运算"<<endl;
	if(p_factor1->localAddr==0)//全局的
	  fprintf(fout,"\tmov eax,[@var_%s]\n",p_factor1->name.c_str());
	else//局部的
	{
	    if(p_factor1->localAddr<0)
	      fprintf(fout,"\tmov eax,[ebp%d]\n",p_factor1->localAddr);
	    else
	      fprintf(fout,"\tmov eax,[ebp+%d]\n",p_factor1->localAddr);
	}
	if(p_factor2->localAddr==0)//全局的
	  fprintf(fout,"\tmov ebx,[@var_%s]\n",p_factor2->name.c_str());
	else//局部的
	{
	  if(p_factor2->localAddr<0)
	    fprintf(fout,"\tmov ebx,[ebp%d]\n",p_factor2->localAddr);
	  else
	    fprintf(fout,"\tmov ebx,[ebp+%d]\n",p_factor2->localAddr);
	}
	switch(opp)
	{
	  case addi:
	    fprintf(fout,"\tadd eax,ebx\n");
	    break;
	  case subs:
	    fprintf(fout,"\tsub eax,ebx\n");
	    break;
	  case mult:
	    fprintf(fout,"\timul ebx\n");
	    break;
	  case divi:
	    fprintf(fout,"\tmov edx,0\n");
	    fprintf(fout,"\tidiv ebx\n");
	    break;
	}
	fprintf(fout,"\tmov [ebp%d],eax\n",pRec->localAddr);
	break;
     case rsv_char://比较运算
     	if(p_factor1->type==rsv_string)
     	{
	  //cout<<"字符串比较运算"<<endl;

     	}
     	else
     	{
	  //cout<<"基本比较运算"<<endl;
	  labLop=genName("lab",null,"base_cmp");
	  labExt=genName("lab",null,"base_cmp_exit");
	  if(p_factor1->localAddr==0)//全局的
	    fprintf(fout,"\tmov eax,[@var_%s]\n",p_factor1->name.c_str());
	  else//局部的
	  {
	    if(p_factor1->localAddr<0)
	      fprintf(fout,"\tmov eax,[ebp%d]\n",p_factor1->localAddr);
	    else
	      fprintf(fout,"\tmov eax,[ebp+%d]\n",p_factor1->localAddr);
	  }
	  if(p_factor2->localAddr==0)//全局的
	    fprintf(fout,"\tmov ebx,[@var_%s]\n",p_factor2->name.c_str());
	  else//局部的
	  {
	    if(p_factor2->localAddr<0)
	      fprintf(fout,"\tmov ebx,[ebp%d]\n",p_factor2->localAddr);
	    else
	      fprintf(fout,"\tmov ebx,[ebp+%d]\n",p_factor2->localAddr);
	  }
	  fprintf(fout,"\tcmp eax,ebx\n");
	  switch(opp)
	  {
	    case gt:
	      fprintf(fout,"\tjg %s\n",labLop.c_str());
	      break;
	    case ge:
	      fprintf(fout,"\tjge %s\n",labLop.c_str());
	      break;
	    case lt:
	      fprintf(fout,"\tjl %s\n",labLop.c_str());
	      break;
	    case le:
	      fprintf(fout,"\tjle %s\n",labLop.c_str());
	      break;
	    case equ:
	      fprintf(fout,"\tje %s\n",labLop.c_str());
	      break;
	    case nequ:
	      fprintf(fout,"\tjne %s\n",labLop.c_str());
	      break;
	  }

	  fprintf(fout,"\tmov eax,0\n");
	  fprintf(fout,"\tjmp %s\n",labExt.c_str());
	  fprintf(fout,"%s:\n",labLop.c_str());
	  fprintf(fout,"\tmov eax,1\n");
	  fprintf(fout,"%s:\n",labExt.c_str());
	  fprintf(fout,"\tmov [ebp%d],eax\n",pRec->localAddr);
     	}
	break;
    }
    return pRec;
}
/**
 * 产生赋值表达式的代码
 */
var_record* genAssign(var_record*des,var_record*src,int &var_num)
{
  if(errorNum!=0)//有错误，不生成
    return NULL;
  sp("对赋值运算的对象类型进行语义检查");
  if(des->type==rsv_void)//空类型，没有意义
  {
     semerror(void_nassi);
     return NULL;
  }
  if(des->type==rsv_string)
  {}
  else if(des->type==rsv_char&&src->type==rsv_int)//char <- int
  {
    //警告
    //warn(int2ch);
  }
  else if(des->type==rsv_int&&src->type==rsv_char)//int <- char
  {}
  else if(des->type!=src->type)//des != src
  {
    semerror(assi_ncomtype);
    return NULL;
  }

  if(des->type==rsv_string)//字符串赋值特殊处理
  {
    if(src->strValId!=-1)
    {
      var_record empstr;
      string empname="";
      empstr.init(rsv_string,empname);
      //src=genExp(src,addi,&empstr,var_num);
      src=genExp(&empstr,addi,src,var_num);
    }
    if(showGen)
      cout<<"生成赋值语句<"<<symName[des->type]<<"> = <"<<symName[src->type]<<">"<<endl;
    if(des->strValId==-2)//全局string
    {
      string labLop=genName("lab",null,"cpy2gstr");
      string labExt=genName("lab",null,"cpy2gstr_exit");
      //将栈中的数据拷贝到静态存储区
      if(src->localAddr<0)
      	fprintf(fout,"\tmov ecx,0\n\tmov esi,[ebp%d]\n\tmov cl,[esi]\n",src->localAddr);
      else
      	fprintf(fout,"\tmov ecx,0\n\tmov esi,[ebp+%d]\n\tmov cl,[esi]\n",src->localAddr);
      fprintf(fout,"\tcmp ecx,0\n\tje %s\n",labExt.c_str());
      fprintf(fout,"\tmov [@str_%s_len],cl\n",des->name.c_str());//先复制长度
      fprintf(fout,"\tsub esi,ecx\n");
      fprintf(fout,"\tmov edi,@str_%s\n",des->name.c_str());
      fprintf(fout,"\tmov edx,0\n");
      fprintf(fout,"%s:\n",labLop.c_str());
      fprintf(fout,"\tmov al,[esi+edx]\n\tmov [edi+edx],al\n");
      fprintf(fout,"\tinc edx\n\tcmp edx,ecx\n\tje %s\n\tjmp %s\n",labExt.c_str(),labLop.c_str());
      fprintf(fout,"%s:\n",labExt.c_str());
    }
    else //局部str
    {
      des->strValId=-1;
      ///不能修改地址，可以重新赋值//des->localAddr=src->localAddr;
      if(src->localAddr<0)
	      fprintf(fout,"\tmov eax,[ebp%d]\n",src->localAddr);
	    else
	    	fprintf(fout,"\tmov eax,[ebp+%d]\n",src->localAddr);
      if(des->localAddr<0)
	fprintf(fout,"\tmov [ebp%d],eax\n",des->localAddr);
      else
	fprintf(fout,"\tmov [ebp+%d],eax\n",des->localAddr);
    }
  }
  else//int char 默认处理
  {
    if(showGen)
      cout<<"生成赋值语句<"<<symName[des->type]<<"> = <"<<symName[src->type]<<">"<<endl;
    if(des->localAddr==0)//全局的
      fprintf(fout,"\tmov eax,@var_%s\n",des->name.c_str());
    else//局部的
    {
      if(des->localAddr<0)
	fprintf(fout,"\tlea eax,[ebp%d]\n",des->localAddr);
      else
	fprintf(fout,"\tlea eax,[ebp+%d]\n",des->localAddr);
    }
    if(src->localAddr==0)//全局的
      fprintf(fout,"\tmov ebx,[@var_%s]\n",src->name.c_str());
    else//局部的
    {
      if(src->localAddr<0)
				fprintf(fout,"\tmov ebx,[ebp%d]\n",src->localAddr);
      else
				fprintf(fout,"\tmov ebx,[ebp+%d]\n",src->localAddr);
    }
    fprintf(fout,"\tmov [eax],ebx\n");
  }
  return des;
}

/**
 * 产生返回语句的代码——返回值是内容的复制，即使它返回的是string类型《参数传递的string也是副本》
 */
void genReturn(var_record*ret,int &var_num)
{
  if(errorNum!=0)//有错误，不生成
    return ;
  if(ret!=NULL)
  {
    if(ret->type==rsv_string)
    {
      //if(ret->strValId!=-1)//不在栈中
      {//强制产生副本
				var_record empstr;
				string empname="";
				empstr.init(rsv_string,empname);
				ret=genExp(&empstr,addi,ret,var_num);
      }
      if(showGen)
				cout<<"生成return <"<<symName[ret->type]<<"> 语句"<<endl;
			if(ret->localAddr<0)
			  fprintf(fout,"\tmov eax,[ebp%d]\n",ret->localAddr);//将副本字符串的地址放在eax中
			else
				fprintf(fout,"\tmov eax,[ebp+%d]\n",ret->localAddr);//将副本字符串的地址放在eax中
    }
    else
    {
      if(showGen)
	cout<<"生成return <"<<symName[ret->type]<<"> 语句"<<endl;
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

  }
  //函数末尾清理
  fprintf(fout,"\tmov ebx,[@s_ebp]\n\tmov [@s_esp],ebx\n");//s_leave
  fprintf(fout,"\tmov ebx,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,ebx\n");//esp<=>[@s_esp]
  fprintf(fout,"\tpop ebx\n\tmov [@s_ebp],ebx\n");//s_ebp
  fprintf(fout,"\tmov ebx,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,ebx\n");//esp<=>[@s_esp]
  fprintf(fout,"\tmov esp,ebp\n\tpop ebp\n\tret\n");//leave
}
/**
 * 产生函数头的代码，因为是在函数解析的过程中进行代码生成，所有默认是用的是tfun的信息opp!=addi
 */
void genFunhead()
{
  if(errorNum!=0)//有错误，不生成
    return;
  if(showGen)
      cout<<"生成函数"<<tfun.name<<"()的首部"<<endl;
//change the main function calling style,handle the main as normal function,call it by start 2012.4.24
  /*if(tfun.name=="main")
  {
    //主函数
    fprintf(fout,"global _start\n_start:\n");//main函数头
  }
  else*/
  {
    fprintf(fout,"%s:\n",tfun.name.c_str());//函数头
  }
  fprintf(fout,"\tpush ebp\n\tmov ebp,esp\n");//enter
  fprintf(fout,"\tmov ebx,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,ebx\n");//esp<=>[@s_esp]
  fprintf(fout,"\tmov ebx,[@s_ebp]\n\tpush ebx\n\tmov [@s_ebp],esp\n");//s_enter
  fprintf(fout,"\tmov ebx,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,ebx\n");//esp<=>[@s_esp]
  fprintf(fout,"\t;函数头\n");
}
void genFuntail()
{
  if(errorNum!=0)//有错误，不生成
    return;
  if(tfun.hadret)
    return;
  if(showGen)
      cout<<"生成函数"<<tfun.name<<"()的尾部"<<endl;
  fprintf(fout,"\t;函数尾\n");
//change the main function calling style,handle the main as normal function,exit call it after called by start 2012.4.24
/*
  if(tfun.name=="main")
  {
    fprintf(fout,"\tmov ebx, 0\n\tmov eax, 1\n\tint 128\n");
  }
  else*/
  {
    fprintf(fout,"\tmov ebx,[@s_ebp]\n\tmov [@s_esp],ebx\n");//s_leave
    fprintf(fout,"\tmov ebx,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,ebx\n");//esp<=>[@s_esp]
    fprintf(fout,"\tpop ebx\n\tmov [@s_ebp],ebx\n");//s_ebp
    fprintf(fout,"\tmov ebx,[@s_esp]\n\tmov [@s_esp],esp\n\tmov esp,ebx\n");//esp<=>[@s_esp]
    fprintf(fout,"\tmov esp,ebp\n\tpop ebp\n\tret\n");//leave
  }
}
/**
 * 为局部变量开辟新的空间，包括临时变量，但不包含参数变量，参数变量的空间一般在调用函数值前申请入栈的
 */
void genLocvar(int val)
{
  if(errorNum!=0)//有语义错误，不生成
    return;
  fprintf(fout,"\tpush %d\n",val);
}

/**
 * 产生条件的汇编代码，为if和while的语句生成服务
 */
void genCondition(var_record*cond)
{
  if(errorNum!=0)//有语义错误，不生成
    return;
  if(cond==NULL)
     return ;
  sp("对条件表达式类型进行语义检查");
  if(cond->type==rsv_string)
  {
    semerror(str_nb_cond);//字符串不能作为条件表达式
    return ;
  }
  else if(cond->type==rsv_void)
  {
    semerror(void_nb_cond);//void不能作为条件表达式
    return ;
  }
  else//char 或者是 int
  {
    if(showGen)
      cout<<"生成复合语句条件"<<endl;
    if(cond->localAddr==0)//全局的
      fprintf(fout,"\tmov eax,[@var_%s]\n",cond->name.c_str());
    else//局部的
    {
      if(cond->localAddr<0)
				fprintf(fout,"\tmov eax,[ebp%d]\n",cond->localAddr);
      else
				fprintf(fout,"\tmov eax,[ebp+%d]\n",cond->localAddr);
    }
    fprintf(fout,"\tcmp eax,0\n");
  }
}

/**
 * 产生block的边界代码
 * 参数isIn：-1-进入block；0..n-退出block
 */
int genBlock(int n)
{
  if(errorNum!=0)//有语义错误，不生成
    return -1;
  if(n==-1)
    return tfun.getCurAddr();
  else
  {
    if(n!=0)
      fprintf(fout,"\tlea esp,[ebp%d]\n",n);
    else
      fprintf(fout,"\tmov esp,ebp\n");
    return -2;
  }
}

void genInput(var_record*p_i,int& var_num)
{
  if(errorNum!=0)//有语义错误，不生成
    return ;
  if(p_i==NULL)
    return;
  sp("对输入对象类型进行语义检查");
  if(p_i->type==rsv_void)
  {
    semerror(void_nin);
    return ;
  }
  if(showGen)
      cout<<"生成对象<"<<symName[p_i->type]<<">("<<p_i->name<<")的输入操作代码"<<endl;
  fprintf(fout,"\t;为%s产生输入代码\n",p_i->name.c_str());
  fprintf(fout,"\tmov ecx,@buffer\n\tmov edx,255\n\tmov ebx,0\n\tmov eax,3\n\tint 128\n");//输入到缓冲区
  ///还没有更好的解决方式～～～2010-5-17 21：18
  //计算缓冲区的具体字符个数，第一个\n之前的所有字符,同时计算如是数字的值，放在eax
  fprintf(fout,"\tcall @procBuf\n");
  //eax-数字的值,bl-字符，ecx-串长度
  if(p_i->type==rsv_string)
  {
    //将buffer临时作为全局string变量输入到指定p_i
    var_record gBuf;
    string bname="";
    bname+="@buffer";
    gBuf.init(rsv_string,bname);
    gBuf.strValId=-2;
    convert_buffer=1;
    genAssign(p_i,&gBuf,var_num);
    convert_buffer=0;
  }
  else if(p_i->type==rsv_int)
  {
    if(p_i->localAddr==0)//全局的
      fprintf(fout,"\tmov [@var_%s],eax\n",p_i->name.c_str());
    else//局部的
    {
      if(p_i->localAddr<0)
				fprintf(fout,"\tmov [ebp%d],eax\n",p_i->localAddr);
      else
				fprintf(fout,"\tmov [ebp+%d],eax\n",p_i->localAddr);
    }
  }
  else
  {
    if(p_i->localAddr==0)//全局的
      fprintf(fout,"\tmov [@var_%s],bl\n",p_i->name.c_str());
    else//局部的
    {
      if(p_i->localAddr<0)
	fprintf(fout,"\tmov [ebp%d],bl\n",p_i->localAddr);
      else
	fprintf(fout,"\tmov [ebp+%d],bl\n",p_i->localAddr);
    }
  }
}

void genOutput(var_record*p_o,int&var_num)
{
  if(errorNum!=0)//有语义错误，不生成
    return ;
  if(p_o==NULL)
    return;
  fprintf(fout,"\t;为%s产生输出代码\n",p_o->name.c_str());
  //强制产生副本
  var_record empstr;
  string empname="";
  empstr.init(rsv_string,empname);
  p_o=genExp(&empstr,addi,p_o,var_num);
  if(showGen)
      cout<<"生成对象<"<<symName[p_o->type]<<">的输出操作代码"<<endl;
  fprintf(fout,"\tmov ecx,[ebp%d]\n\tmov edx,0\n\tmov dl,[ecx]\n\tsub ecx,edx\n\tmov ebx,1\n\tmov eax,4\n\tint 128\n",p_o->localAddr);
}

