#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdio.h>
#include <iostream>
using namespace std;
/**  *****************************************************************************************************************************
                                                             ***类型定义***   ********************************************************************************************************************************/
enum symbol//所有符号的枚举
{
  null,ident,excep,number,//空，标识符，异常字符，数字
  addi,subs,mult,divi,assign,//加，减，乘，除,赋值
  gt,ge,lt,le,equ,nequ,//>,>=,<,<=,==,!=
  input,output,//输入和输出
  comma,chara,strings,semicon,lparen,rparen,lbrac,rbrac,//界符 , ch string ; ( ) { }
  rsv_min,
  rsv_break,rsv_char,rsv_continue,rsv_else,rsv_extern,rsv_if,rsv_in,rsv_int,rsv_out,rsv_return,rsv_string,rsv_void,rsv_while//保留字
  ,rsv_max
};
enum errcode//所有的错误码
{
  //词法错误
  charwrong,strwrong,str2long,num2long,id2long,excpchar,
  //语法错误
  semiconlost,commalost,typelost,identlost,semiconwrong,typewrong,//变量声明部分的错误类型
  paralost,rparenlost,lbraclost,rbraclost,//函数定义部分的错误类型
  statementexcp,localidentlost,lparenlost,lparenwrong,staterparenlost,rparenwrong,elselost,elsespelterr,elsewrong,//复合语句部分的错误类型
  idtaillost,returnwrong,arglost,argwrong,arglistwrong,na_input,input_err,output_err,
  opplost,oppwrong,exprlost,exprparenlost,exprwrong,//表达式错误
  //语义错误
  var_redef,para_redef,localvar_redef,fun_redef,fun_def_err,fun_dec_err,//声明类错误
  str_nadd,void_ncal,//表达式类错误
  void_nassi,assi_ncomtype,ret_type_err,fun_undec,var_undec,real_args_err,str_nb_cond
  ,void_nb_cond,break_nin_while,continue_nin_while,void_nin,//语句错误
};
struct var_record;//变量声明记录
struct fun_record;//函数声明记录
class Table;//符号表

#define idLen 30	/*标识符的最大长度30*/
#define numLen 9	/*数字的最大位数9*/
#define stringLen 255	/*字符串的最大长度255*/
//宏定义
#define GET_CHAR if(-1==getChar())return -1;
/**  *****************************************************************************************************************************
                                                             ***全局声明***
   ********************************************************************************************************************************/
extern FILE * fin;//全局变量，文件输入指针
extern FILE * fout;//全局变量，文件输出指针
extern enum symbol sym;//当前符号，getSym()->给语法分析使用


extern char str[];//记录当前string，给erorr处理
extern char id[];//记录当前ident
extern int num;//记录当前num
extern char letter;//记录当前number
extern int errorNum;//记录错误数
extern int warnNum;//记录警告数
extern int lineNum;//记录行数
extern int synerr;//语法错误个数
extern int semerr;//语义错误个数

extern bool showLex;
extern bool showSyn;
extern bool showSem;
extern bool showGen;
extern bool showTab;



extern int compileOk;

extern string fileName;

extern enum symbol token;//记录有效的符号
extern char symName[][30];//符号名字表（调试用）

extern var_record tvar;
extern fun_record tfun;
extern Table table;

int getSym();
void program();
void semerror(enum errcode err);
var_record* genExp(var_record*p_factor1,symbol token,var_record*p_factor2,int &var_num);
var_record* genAssign(var_record*des,var_record*src,int &var_num);
void genReturn(var_record*ret,int &var_num);
void genFunhead();
void genLocvar(int val);
void genFuntail();
void genCondition(var_record*cond);
int genBlock(int n);
void genInput(var_record*p_i,int&var_num);
void genOutput(var_record*p_o,int&var_num);

#endif

//行数=2290 2011-4-9 18:08
