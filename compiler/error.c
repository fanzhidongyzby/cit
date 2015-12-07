#include "common.h"
int errorNum=0,warnNum=0;//所有错误的个数
int synerr=0;//语法错误的个数，一旦有语法错误，就不进行语义处理，因为没有太大的价值
int semerr=0;//语义错误个数，一旦有语义错误，就不进行代码生成，因为没有太大的价值，但是没有语义错误不代表就可以，有可能是语法错误，语义分析没有进行
/**  *****************************************************************************************************************************
                                                             ***错误处理*** ********************************************************************************************************************************/
/**
  打印语法错误信息
  参数:err 错误码
*/

void semerror(enum errcode err)
{
  if(synerr!=0)
    return;
  errorNum++;
  semerr++;
  printf("错误[语义错误->%s第%d行]",fileName.c_str(),lineNum);
  switch(err)
  {
    case var_redef:
      printf("全局变量名称重定义。\n");
      break;
    case para_redef:
      printf("参数名称重定义。\n");
      break;
    case localvar_redef:
      printf("局部变量名称重定义。\n");
      break;
    case fun_redef:
      printf("函数重定义。\n");
      break;
    case fun_def_err:
      printf("函数定义和先前的声明不一致。\n");
      break;
    case fun_dec_err:
      printf("函数的多次声明不一致。\n");
      break;
    case fun_undec:
      printf("函数被调用之前没有合法的声明。\n");
      break;
    case var_undec:
      printf("变量在使用之前没有合法的声明。\n");
      break;
    case str_nadd:
      printf("字符串不能运用于除了加法以外的运算。\n");
      break;
    case void_ncal:
      printf("void类型不能参加表达式运算。\n");
      break;
    case void_nassi:
      printf("void类型不能参加赋值运算。\n");
      break;
    case assi_ncomtype:
      printf("赋值类型不匹配。\n");
      break;
    case ret_type_err:
      printf("函数返回值与函数类型不匹配。\n");
      break;
    case real_args_err:
      printf("函数实参的类型不能与函数的形参声明严格匹配。\n");
      break;
    case str_nb_cond:
      printf("字符串类型不能作为条件。\n");
      break;
    case void_nb_cond:
      printf("void类型不能作为条件。\n");
      break;
    case break_nin_while:
      printf("break语句不能出现在while之外。\n");
      break;
    case continue_nin_while:
      printf("continue语句不能出现在while之外。\n");
      break;
    case void_nin:
      printf("void类型不能作为输入的对象。\n");
      break;
  }
}
/**
  打印语法错误信息
  参数:err 错误码
      pos 相对token的位置 正数-after 负数-before
*/

void synterror(enum errcode err,int pos)
{
  errorNum++;
  synerr++;
  printf("erro[Syntactic error at %s line %d]",fileName.c_str(),lineNum);
  switch(err)
  {
    case semiconlost:
      printf("the symbol ; may be lost .");
      break;
    case commalost:
      printf("the symbol , may be lost .");
      break;
    case typelost:
      printf("identifier type may be lost .");
      break;
    case identlost:
      printf("identifier name may be lost .");
      break;
    case semiconwrong:
      printf("the symbol ; can't be replaced by other symbol .");
      break;
    case typewrong:
      printf("unrecognized type .");
      break;
    case paralost:
      printf("the name of function's parameter may be lost .");
      break;
    case rparenlost:
      printf("the function's head should be end with symbol ) .");
      break;
    case lbraclost:
      printf("the symbol { may be lost in compound statement .");
      break;
    case rbraclost:
      printf("the symbol } may be lost in compound statement .");
      break;
    case statementexcp:
      printf("the symbol is not a effective statement start .");
      break;
    case localidentlost:
      printf("the local varibility's name may be lost .");
      break;
    case lparenlost:
      printf("the symbol ( in the statement may be lost .");
      break;
    case lparenwrong:
      printf("the symbol ( in the statement can't be replaced .");
      break;
    case staterparenlost:
      printf("the symbol ) in the statement may be lost .");
      break;
    case rparenwrong:
      printf("the symbol ) in the statement can't be replaced .");
      break;
    case elselost:
      printf("the reserved word 'else' in the statement may be lost .");
      break;
    case elsespelterr:
      printf("the reserved word 'else' in the statement may be spelt wrongly .");
      break;
    case elsewrong:
      printf("the reserved word 'else' in the statement can't be replaced .");
      break;
    case idtaillost:
      printf("unresonable identifier's quotion .");
      break;
    case returnwrong:
      printf("unresonable type or data was returned .");
      break;
    case arglost:
      printf("the actual args may be lost .");
      break;
    case argwrong:
      printf("unresonable actual args .");
      break;
    case arglistwrong:
      printf("unresonable separator in arglist .");
      break;
    case opplost:
      printf("operator may be lost in the expression .");
      break;
    case oppwrong:
      printf("excepted operator in the expression .");
      break;
    case exprlost:
      printf("expression may be lost .");
      break;
    case exprparenlost:
      printf("the symbol ) in the expression may be lost .");
      break;
    case exprwrong:
      printf("excepted expression .");
      break;
    case na_input:
      printf("unresonable input object .");
      break;
    case input_err:
      printf("the symbol >> may be wrong .");
      break;
    case output_err:
      printf("the symbol << may be wrong .");
      break;
  }
  if(pos<0)
    printf("(before ");
  else if(pos>0)
    printf("(after ");
  else if(pos==0)
    printf("(at ");

  switch(token)
  {
    case null:
      printf("file's ending)\n");
      break;
    case ident:
      printf("identifier %s)\n",id);
      break;
    case number:
      printf("number %d)\n",num);
      break;
    case chara:
      printf("character %c)\n",letter);
      break;
    case strings:
      printf("string %s)\n",str);
      break;
    default:
      if(token>rsv_min&&token<rsv_max)
      {
	printf("reserved word %s)\n",symName[token]);
      }
      else
	printf("symbol %s)\n",symName[token]);
  }
}
/**
  打印词法错误信息
  参数:err 错误码
      c 可选字符
*/
void lexerror(enum errcode err,char c)
{
  errorNum++;
  printf("erro[Lexical error at %s line %d]",fileName.c_str(),lineNum);
  switch(err)
  {
    case charwrong:
      printf("unrecognized const character without ending '\n");
      break;
    case strwrong:
      printf("unrecognized const string without ending \"\n");
      break;
    case str2long:
      printf("the const string is too long(only conserved %dByte ahead=%s) .\n",stringLen,str);
      break;
    case num2long:
      printf("the number is too long(only conserved %dByte ahead=%d) .\n",numLen,num);
      break;
    case id2long:
      printf("the identifier is too long(only conserved %dByte ahead=%s) .\n",idLen,id);
      break;
    case excpchar:
      printf("unrecognized symbol %c \n",c);
      break;
  }
}

