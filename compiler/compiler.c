#include "compiler.h"
#include "common.h"
#include "semantic.h"


FILE * fin=NULL;//全局变量，文件输入指针
FILE * fout=NULL;//全局变量，文件输出指针



extern char ch;
extern char oldCh;
extern int chAtLine;//当前字符列位置
extern int lineLen;//当前行的长度

extern int ID;//标签ID
extern int lopID;//while的ID
extern int ifID;//if的ID
extern int stringId;//串空间ID

bool showLex;
bool showSyn;
bool showSem;
bool showGen;
bool showTab;
string fileName;
Compiler::Compiler(bool l,bool sy,bool se,bool g,bool t)
{
	showLex=l;
	showSyn=sy;
	showSem=se;
	showGen=g;
	showTab=t;
}


void Compiler::genCommonFile()
{
	fout=fopen("../work/common.s","w");//共用输出文件
	fprintf(fout,"section .text\n");
  ///字符串长度异常输出函数
  fprintf(fout,
  	"@str2long:\n\tmov edx,@str_2long_data_len\n\tmov ecx,@str_2long_data\n\tmov ebx, 1\n\tmov eax, 4\n\tint 128\n");
  fprintf(fout,"\tmov ebx, 0\n\tmov eax, 1\n\tint 128\n\tret\n");
  ///缓冲区处理函数
  fprintf(fout,"@procBuf:\n");
  //计算缓冲区的具体字符个数，第一个\n之前的所有字符,同时计算如是数字的值，放在eax
  fprintf(fout,"\tmov esi,@buffer\n\tmov edi,0\n\tmov ecx,0\n\tmov eax,0\n\tmov ebx,10\n");
  fprintf(fout,"@cal_buf_len:\n");
  fprintf(fout,"\tmov cl,[esi+edi]\n\tcmp ecx,10\n\tje @cal_buf_len_exit\n");
  fprintf(fout,"\tinc edi\n\timul ebx\n\tadd eax,ecx\n\tsub eax,48\n\tjmp @cal_buf_len\n");
  fprintf(fout,"@cal_buf_len_exit:\n\tmov ecx,edi\n\tmov [@buffer_len],cl\n\tmov bl,[esi]\n");//保存长度，供字符串拷贝用;
  //eax-数字的值,bl-字符，ecx-串长度
  fprintf(fout,"\tret\n");
  //main function calling position
  fprintf(fout,"global _start\n_start:\n");//main函数头
  fprintf(fout,"\tcall main\n");
  fprintf(fout,"\tmov ebx, 0\n\tmov eax, 1\n\tint 128\n");
  
  //生成数据段
  fprintf(fout,"section .data\n\t@str_2long_data db \"字符串长度溢出！\",10,13\n\t@str_2long_data_len equ 26\n");
  fprintf(fout,"\t@buffer times 255 db 0\n\t@buffer_len db 0\n");//输入缓冲区
  //生成辅助数据栈
  fprintf(fout,"\t@s_esp dd @s_base\n\t@s_ebp dd 0\n");
  fprintf(fout,"section .bss\n\t@s_stack times 65536 db 0\n@s_base:\n");//栈放在.bss段
  
  
  fclose(fout);
}
void Compiler::compile(char* name)
{
	fileName=name;
	//初始化变量状态
	table.clear();
  lineLen=0;
  chAtLine=0;
  lineNum=0;
  oldCh=ch;
  ch=' ';
  errorNum=0;
  warnNum=0;
  synerr=0;
  semerr=0;
  stringId=0;
  ID=0;
  lopID=0;
  ifID=0;


  fin=fopen(fileName.c_str(),"r");//输入文件
  int start=fileName.rfind("/");
  if(start==-1)start=0;
  int end=fileName.rfind(".");
  string realName="../work/"+fileName.substr(start,end-start);
  fout=fopen((realName+".s").c_str(),"w");//输出文件

	fprintf(fout,"section .text\n");
  program();//编译程序
  fprintf(fout,"section .bss\n");//添加一个空.bss段，为链接器定位末端服务
  fclose(fin);
  fclose(fout);
  if(errorNum!=0)
  {
    remove((realName+".s").c_str());//删除输出文件
  }
}

