#include "common.h"
#include <string.h>
//--词法分析
void checkReserved();
/**  *****************************************************************************************************************************
                                                             ***词法分析***
   ********************************************************************************************************************************/
/**
  扫描器Scanner：从文件中按顺序读取一个字符返回[文件不读完不要修改fin，否则数据丢失]
  返回值：-1表示文件结束
  关联：全局变量ch,行号lineNum
*/
//getChar()
#define maxLen 80	/*文件行的最大长度*/
char ch=' ';//当前字符
char oldCh=' ';//上一个字符
int lineNum=0;//行号
char line[maxLen];
int chAtLine=0;//当前字符列位置
int lineLen=0;//当前行的长度
int getChar()
{
  if(chAtLine>=lineLen)//超出索引，行读完,>=防止出现强制读取的bug
  {
    chAtLine=0;//字符，行，重新初始化
    lineLen=0;
    lineNum++;//行号增加
    ch=' ';
    while(ch!=10)//检测行行结束
    {
      if(fscanf(fin,"%c",&ch)==EOF)
      {
				line[lineLen]=0;//文件结束
				break;
      }
      line[lineLen]=ch;//循环读取一行的字符
      lineLen++;
      if(lineLen==maxLen)//单行程序过长
      {
				//不继续读就可以，不用报错
				break;
      }
    }
  }
  //正常读取
  oldCh=ch;
  ch=line[chAtLine];
  chAtLine++;
  if(ch==0)
    return -1;
  else
    return 0;
}

/**
  解析器Tokenizer：将标识符，关键字，数字，界符分别识别出来
  返回值：解析出的符号，负数表示有错误，绝对值表示错误码
  关联：全局变量sym,id[],str[],num,letter
*/
//getSym()
int err=0;//记录错误
enum symbol sym=null;//当前符号
char id[idLen+1];//存放标识符
int num=0;//存放的数字
char str[stringLen+1];//存放字符串
char letter=0;//存放字符
int getSym()
{
  while(ch==' '||ch==10||ch==9)//忽略空格，换行，TAB
  {
    getChar();
  }

  if(ch>='a'&&ch<='z'||ch>='A'&&ch<='Z'||ch=='_'||ch=='@'||ch=='.')//_,@,字母开头的_,@,字母，数字串：标识符（关键字）
  {
    int idCount=0;//为标识符的长度计数
    int reallen=0;//实际标识符长度
    int f;//getChar返回标记
    //取出标识符
    do
    {
      reallen++;
      if(idCount<idLen)//标识符过长部分掠去
      {
				id[idCount]=ch;
				idCount++;
      }
      f=getChar();
    }
    while(ch>='a'&&ch<='z'||ch>='A'&&ch<='Z'||ch=='_'||ch=='@'||ch=='.'||ch>='0'&&ch<='9');
    id[idCount]=0;//结尾
    if(reallen>idLen)//标识符过长
    {
      //lexerror(id2long,0);
    }
    checkReserved();
    return f;
  }
  else if(ch>='0'&&ch<='9')//数字,默认正数
  {
    sym=number;
    int numCount=0;//为数字的长度计数
    num=0;//数值迭代器
    int reallen=0;//实际数字长度
    int f;//getChar返回标记
    do
    {
      reallen++;
      if(numCount<numLen)//数字过长部分掠去
      {
				num=10*num+ch-'0';
				numCount++;
      }
      f=getChar();
    }
    while(ch>='0'&&ch<='9');
    if(reallen>numLen)//数字太长
    {
      //lexerror(num2long,0);
    }
    return f;
  }
  else
  {
    int strCount=0;//为字符串的长度计数
    int f=0;//getChar返回标记
    int reallen;//记录串的实际长度
    switch(ch)
    {
      case '+':
			sym=addi;
			GET_CHAR
			break;
      case '-':
			sym=subs;
			GET_CHAR
			break;
      case ':':
			sym=colon;
			GET_CHAR
			break;
      case ';':
			sym=null;
			GET_CHAR
			while(ch!='\n')//只要行不结束
			{
				GET_CHAR
			}
			GET_CHAR
			break;
      case ',':
			sym=comma;
			GET_CHAR
			break;
      case '"':
			sym=null;
			//GET_CHAR
			f=getChar();
			if(f==-1)//文件结束
			{
				//lexerror(strwrong,0);
				return -1;
			}
			reallen=0;
			while(ch!='"')//只要不是“结尾
			{
				reallen++;
				if(strCount<stringLen)//字符串过长部分掠去
				{
					str[strCount]=ch;
					strCount++;
				}
				//GET_CHAR
				f=getChar();
				if(f==-1)//文件结束
				{
					//lexerror(strwrong,0);
					return -1;
				}
			}
			str[strCount]=0;//结尾
			if(reallen>stringLen)//string太长
			{
				//lexerror(str2long,0);
			}
			sym=strings;
			GET_CHAR
			break;
      case '[':
			sym=lbrac;
			GET_CHAR
			break;
      case ']':
			sym=rbrac;
			GET_CHAR
			break;
      case 0://不在这个位置达到文件末尾，会暂时不处理，等到下一次调用时候在这里返回
			sym=null;
			return -1;
			break;
      default:
			sym=excep;
			//lexerror(excpchar,ch);
			printf("不能解析的词法符号[line:%d]\n",lineNum);
			err++;
			//虽然是词法错误，但是不影响语法语义的分析过程，暂且定位为警告
			//warnNum++;
			GET_CHAR
    }
  }
  return 0;
}

/**
  检查保留字：对id[idLen]的内容搜索，若是保留字修改sym,否则sym=ident
  关联：全局变量sym,id[]
*/
//checkReserved()
#define reservedNum 46
char reservedTable[reservedNum][idLen]=
{
  "al","cl","dl","bl","ah","ch","dh","bh",
  "eax","ecx","edx","ebx","esp","ebp","esi","edi",
  "mov","cmp","sub","add","lea",//2p
  "call","int","imul","idiv","neg","inc","dec","jmp","je","jg","jl","jge","jle","jne","jna","push","pop",//1p
  "ret",//0p
  "section","global","equ","times","db","dw","dd"
};
static enum symbol reservedSymbol[reservedNum]=
{
  br_al,br_cl,br_dl,br_bl,br_ah,br_ch,br_dh,br_bh,
  dr_eax,dr_ecx,dr_edx,dr_ebx,dr_esp,dr_ebp,dr_esi,dr_edi,
  i_mov,i_cmp,i_sub,i_add,i_lea,//2p
  i_call,i_int,i_imul,i_idiv,i_neg,i_inc,i_dec,i_jmp,i_je,i_jg,i_jl,i_jge,i_jle,i_jne,i_jna,i_push,i_pop,//1p
  i_ret,//0p
  a_sec,a_glb,a_equ,a_times,a_db,a_dw,a_dd
};

void checkReserved()
{
  for(int k=0;k<reservedNum;k++)
  {
    if(strcmp(id,reservedTable[k])==0)
    {
      sym=reservedSymbol[k];
      return;
    }
  }
  sym=ident;//搜索失败，是标识符
}
