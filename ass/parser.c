#include "common.h"
#include "semantic.h"
/**  *****************************************************************************************************************************
                                                             ***语法分析***
   ********************************************************************************************************************************/
void program();
void lbtail(string lbName);
void inst();
void basetail(string lbName,int times);
int len();
void values(string lbName,int times,int len);
void type(int cont[],int&cont_len,int len);
void valtail(int cont[],int&cont_len,int len);
void opr(int &regNum,int&type,int&len);
int reg();
void mem();
void addr();
void regaddr(symbol basereg,const int type);
void regaddrtail(symbol basereg,const int type,symbol sign);
void off();

int wait=0;//指导nextToken是否继续取符号，每次设置只能作用一次
#define BACK wait=1;
/**
  读取下一个有效的符号，testSym[]作为测试观察使用
  返回值:编译结束返回-1，一般情况现返回0.
*/
enum symbol token;
string curSeg="";//当前段名称
int dataLen=0;//有效数据长度
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
				token=null;
				if(scanLop==1)//准备第二编扫描
				{
					table.switchSeg();//第一次扫描最后记录以下最后一个段信息
					fclose(fin);
					fin=fopen((finName+".s").c_str(),"r");//输入文件
					oldCh=ch;
					ch=' ';
					lineLen=0;
					chAtLine=0;
					lineNum=0;
					scanLop++;
					continue;
				}
				return -1;
      }
    }
    else//get effective symbol
    {
       /*
       if(sym==strings)
 				printf("字符串\t\t\"%s\"\n",str);
       else if(sym==ident)
 				printf("标识符\t\t(%s)\n",id);
       else if(sym==number)
 				printf("数字\t\t[%d]\n",num);
       else if(sym>=br_al&&sym<=a_dd)
 				printf("关键字\t\t<%s>\n",id);
       else
 				printf("界符\t\t\n");
       */
      token=sym;
      return 0;
    }
  }
}
void match(symbol s)
{
  if(nextToken()==0)
  {
    if(token!=s)
    {
      printf("语法符号匹配出错[line:%d]\n",lineNum);
    }
  }
}
// f=fread(buffer,1,1024,ftmp);
// fwrite(buffer,1,f,fout);
int scanLop=1;//记录扫描的次数，第一遍扫描计算所有符号的地址或者值，第二编扫描，生成指令的二进制内容
void program()//untest:调用之前是否提前测试了符号
{
  if(nextToken()==-1||token==null)//文件末尾 #
  {
    //cout<<"代码长度="<<lb_record::curAddr-textAddr<<endl;
    table.exportSyms();//导出符号表
    obj.printAll();
    return ;
  }
  else
  {
    string lbName="";
    switch(token)
    {
      case ident:
				lbName+=id;
				lbtail(lbName);
				break;
      case a_sec:
				match(ident);
				table.switchSeg();
				break;
      case a_glb:
				match(ident);//定义全局入口符号，这里默认是_start,链接器默认也是lit，这里不做具体处理
				break;
      default:
				BACK
				inst();
    }
    program();
  }
}
void lbtail(string lbName)
{
  nextToken();
  switch(token)
  {
    case a_times:
      match(number);
      basetail(lbName,num);
      break;
    case a_equ:
      match(number);
      {
				lb_record *lr=new lb_record(lbName,num);
				table.addlb(lr);
      }
      break;
    case colon:
      {
				lb_record *lr=new lb_record(lbName,false);
				table.addlb(lr);
      }
      break;
    default:
      BACK
      basetail(lbName,1);
  }
}
void basetail(string lbName,int times)
{
  int l=len();
  values(lbName,times,l);
}
int len()
{
  nextToken();
  switch(token)
  {
    case a_db:
      return 1;
      break;
    case a_dw:
      return 2;
      break;
    case a_dd:
      return 4;
      break;
    default:
      printf("len err![line:%d]\n",lineNum);
      return 0;
  }
}
void values(string lbName,int times,int len)
{
  int cont[255]={0};
  int cont_len=0;
  type(cont,cont_len,len);
  valtail(cont,cont_len,len);
  lb_record *lr=new lb_record(lbName,times,len,cont,cont_len);
  table.addlb(lr);
}
void type(int cont[],int&cont_len,int len)
{
  //查看lbName是否存在，否则添加到符号表，若存在忽略times和len字段
  nextToken();
  switch(token)
  {
    case number:
      cont[cont_len]=num;
      cont_len++;
      break;
    case strings:
      for(int i=0;;i++)
      {
				if(str[i]!=0)
				{
					cont[cont_len]=(unsigned char)str[i];
					cont_len++;
				}
				else
					break;
      }
      break;
    case ident:
      {
      	string name="";
				name+=id;
				lb_record*lr=table.getlb(name);
      	cont[cont_len]=lr->addr;//把地址作为占位，第二次扫描还会刷新，为该位置生成重定位项(equ除外)
				//处理数据段重定位项
				if(scanLop==2)//第二次扫描记录重定位项
				{
					if(!lr->isEqu)//不是equ
						obj.addRel(curSeg,lb_record::curAddr+cont_len*len,name,R_386_32);//数据段重定位位置很好确定，直接添加重定位项即可
				}
				cont_len++;
      }
      break;
    default:
      printf("type err![line:%d]\n",lineNum);
  }
}
void valtail(int cont[],int&cont_len,int len)
{
  nextToken();
  switch(token)
  {
    case comma:
      type(cont,cont_len,len);
      valtail(cont,cont_len,len);
      break;
    default:
      BACK
      return;
  }
}
ModRM modrm;
SIB sib;
Inst instr;
void inst()
{
  instr.init();
  int len=0;
  nextToken();
  if(token>=i_mov&&token<=i_lea)
  {
    symbol t=token;
    int d_type=0,s_type=0;
    int regNum=0;
    opr(regNum,d_type,len);
    match(comma);
    opr(regNum,s_type,len);
    gen2op(t,d_type,s_type,len);
  }
  else if(token>=i_call&&token<=i_pop)
  {
    symbol t=token;
    int type=0,regNum=0;
    opr(regNum,type,len);
    gen1op(t,type,len);
  }
  else if(token==i_ret)
  {
    symbol t=token;
    gen0op(t);
  }
  else
  {
    printf("opcode err[line:%d]\n",lineNum);
  }
}
lb_record*relLb=NULL;//记录指令中可能需要重定位的标签（使用了符号）
void opr(int &regNum,int&type,int&len)
{
  string name="";
  lb_record *lr;
  nextToken();
  switch(token)
  {
    case number://立即数
      type=immd;
      instr.imm32=num;
      break;
    case ident://立即数
      type=immd;
      name+=id;
      lr=table.getlb(name);
			instr.imm32=lr->addr;
			//处理数据段重定位项
			if(scanLop==2)//第二次扫描记录重定位项
			{
				if(!lr->isEqu)//不是equ
				{
					//记录符号
					relLb=lr;
				}
			}
      break;
    case lbrac://内存寻址
      type=memr;
      BACK
      mem();
      break;
    case subs://负立即数
      type=immd;
      match(number);
      instr.imm32=-num;
      break;
    default://寄存器操作数 11 rm=des reg=src 
      type=regs;
      BACK
      len=reg();
      if(regNum!=0)//双reg，将原来reg写入rm作为目的操作数，本次写入reg
      {
				modrm.mod=3;//双寄存器模式
				modrm.rm=modrm.reg;//因为统一采用opcode rm,r 的指令格式，比如mov rm32,r32就使用0x89,若是使用opcode r,rm 形式则不需要
				modrm.reg=token-br_al-(1-len%4)*8;//计算寄存器的编码
      }
      else//第一次出现reg，临时在reg中，若双reg这次是目的寄存器，需要交换位置
      {
				modrm.reg=token-br_al-(1-len%4)*8;//计算寄存器的编码
      }
      regNum++;
  }
}
int reg()
{
  nextToken();
  if(token>=br_al&&token<=br_bh)//8位寄存器
  {
    return 1;
  }
  else if(token>=dr_eax&&token<=dr_edi)//32位寄存器
  {
    return 4;
  }
  else
  {
    printf("reg err![line:%d]\n",lineNum);
    return 0;
  }
}
void mem()
{
  match(lbrac);
  addr();
  match(rbrac);
}
void addr()
{
  string name="";
  lb_record*lr;
  nextToken();
  switch(token)
  {
    case number://直接寻址 00 xxx 101 disp32
      modrm.mod=0;
      modrm.rm=5;
      instr.setDisp(num,4);
      break;
    case ident://直接寻址
      modrm.mod=0;
      modrm.rm=5;
      name+=id;
      lr=table.getlb(name);
			instr.setDisp(lr->addr,4);
			if(scanLop==2)//第二次扫描记录重定位项
			{
				if(!lr->isEqu)//不是equ
				{
					//记录符号
					relLb=lr;
				}
			}
      break;
    default://寄存器寻址
      BACK
      int type=reg();
      regaddr(token,type);
  }
}
void regaddr(symbol basereg,const int type)
{
  nextToken();
  if(token==addi||token==subs)//有变址寄存器
  {
    BACK
    off();
    regaddrtail(basereg,type,token);
  }
  else//寄存器间址 00 xxx rrr <esp ebp特殊考虑>
  {
    if(basereg==dr_esp)//[esp]
    {
      modrm.mod=0;
      modrm.rm=4;//引导SIB
      sib.scale=0;
      sib.index=4;
      sib.base=4;
    }
    else if(basereg==dr_ebp)//[ebp],生成汇编代码中未出现
    {
      modrm.mod=1;//8-bit 0 disp，或者mod=2 32-bit 0 disp
      modrm.rm=5;
      instr.setDisp(0,1);
    }
    else//一般寄存器
    {
      modrm.mod=0;
      modrm.rm=basereg-br_al-(1-type%4)*8;
    }
    BACK
    return;
  }
}
void off()
{
  nextToken();
  if(token==addi||token==subs)
  {}
  else
  {
    printf("addr err![line:%d]\n",lineNum);
  }
}
void regaddrtail(symbol basereg,const int type ,symbol sign)
{
  nextToken();
  switch(token)
  {
    case number://寄存器基址寻址 01/10 xxx rrr disp8/disp32
      if(sign==subs)
				num=-num;
      if(num>=-128&&num<128)//disp8
      {
				modrm.mod=1;
				instr.setDisp(num,1);
      }
      else
      {
				modrm.mod=2;
				instr.setDisp(num,4);
      }
      modrm.rm=basereg-br_al-(1-type%4)*8;
      
      if(basereg==dr_esp)//sib
      {
      	modrm.rm=4;//引导SIB
				sib.scale=0;
				sib.index=4;
				sib.base=4;
      }
      break;
    default://基址变址寻址 00 xxx 100 00=scale rrr2=index rrr1=base,不会发生在esp和ebp上，没有生成这样的指令
      BACK
      int typei=reg();
      modrm.mod=0;
      modrm.rm=4;
      sib.scale=0;
      sib.index=token-br_al-(1-typei%4)*8;
      sib.base=basereg-br_al-(1-type%4)*8;
  }
}


























