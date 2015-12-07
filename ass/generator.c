#include "common.h"
#include "semantic.h"
#include <iomanip>
/**  *****************************************************************************************************************************
                                                             ***代码生成*** ********************************************************************************************************************************/
static unsigned char i_2opcode[]=
{
//			8位操作数			 |		32位操作数
//r,r  r,rm|rm,r r,im|r,r  r,rm|rm,r r,im
  0x88,0x8a,0x88,0xb0,0x89,0x8b,0x89,0xb8,//mov
  0x38,0x3a,0x38,0x80,0x39,0x3b,0x39,0x81,//cmp
  0x28,0x2a,0x28,0x80,0x29,0x2b,0x29,0x81,//sub
  0x00,0x02,0x00,0x80,0x01,0x03,0x01,0x81,//add
  0x00,0x00,0x00,0x00,0x00,0x8d,0x00,0x00//lea
};
static unsigned short int i_1opcode[]=
{
  0xe8,0xcd,/*0xfe,*/0xf7,0xf7,0xf7,0x40,0x48,0xe9,//call,int,imul,idiv,neg,inc,dec,jmp<rel32>
  0x0f84,0x0f8f,0x0f8c,0x0f8d,0x0f8e,0x0f85,0x0f86,//je,jg,jl,jge,jle,jne,jna<rel32>
  //0xeb,//jmp rel8
  //0x74,0x7f,0x7c,0x7d,0x7e,0x75,0x76,//je,jg,jl,jge,jle,jne,jna<rel8>
  /*0x68,*/0x50,//push
  0x58//pop
};
static unsigned char i_0opcode[]=
{
  0xc3//ret
};
int inLen=0;
/*
	输出ModRM字节
	mod(2)|reg(3)|rm(3)
*/
void writeModRM()
{
	if(modrm.mod!=-1)//有效
	{
		unsigned char mrm=(unsigned char)(((modrm.mod&0x00000003)<<6)+((modrm.reg&0x0000007)<<3)+(modrm.rm&0x00000007));
		writeBytes(mrm,1); 
		//printf("输出ModRM=0x%08x\n",mrm);
	}
}
/*
	输出SIB字节
	scale(2)|index(3)|base(3)
*/
void writeSIB()
{
  if(sib.scale!=-1)
	{
		unsigned char _sib=(unsigned char)(((sib.scale&0x00000003)<<6)+((sib.index&0x00000007)<<3)+(sib.base&0x00000007));
		writeBytes(_sib,1);
		//printf("输出SIB=0x%08x\n",_sib);
	}
}
/*
	按照小端顺序（little endian）输出指定长度数据
	len=1：输出第4字节
	len=2:输出第3,4字节
	len=4:输出第1,2,3,4字节
*/
void writeBytes(int value,int len)
{
	lb_record::curAddr+=len;//计算地址
	if(scanLop==2)
  {
  	fwrite(&value,len,1,fout);
  	inLen+=len;
  }
	//cout<<lb_record::curAddr<<"\t"<<inLen<<endl;
}
/*
void write1B(int i)
{
  unsigned char ii=(unsigned char)i;
  fprintf(fout,"%c",ii);inLen++;
}
void write2B(int i)
{
  unsigned short ii=(unsigned short)i;
  unsigned char tmp;
  for(int k=0;k<2;k++)
  {
    tmp=(unsigned char)ii%256;
    fprintf(fout,"%c",tmp);
    ii/=256;inLen++;
  }
}
void write4B(int i)
{
  unsigned int ii=(unsigned int)i;
  unsigned char tmp;
  for(int k=0;k<4;k++)
  {
    tmp=(unsigned char)ii%256;
    fprintf(fout,"%c",tmp);
    ii/=256;inLen++;
  }
}
*/
bool processRel(int type)//处理可能的重定位信息
{
	if(scanLop==1||relLb==NULL)
	{
		relLb=NULL;
		return false;
	}
	bool flag=false;
	if(type==R_386_32)//绝对重定位
	{
		if(!relLb->isEqu)//只要是地址符号就必须重定位，宏除外[这里判断与否都可以，relLb非NULL都是非equ符号]
		{
			obj.addRel(curSeg,lb_record::curAddr,relLb->lbName,type);
			flag=true;
		}
	}
	else if(type==R_386_PC32)//相对重定位
	{
		if(relLb->externed)//对于跳转，内部的不需要重定位，外部的需要重定位
		{
			obj.addRel(curSeg,lb_record::curAddr,relLb->lbName,type);
			flag=true;
		}
	}
	relLb=NULL;
	return flag;
}
void gen2op(symbol opt,int des_t,int src_t,int len)
{
  int oldAddr=lb_record::curAddr;
    //lb_record::curAddr=0;
    //测试信息
//     cout<<"len="<<len<<"(1-Byte;4-DWord)\n";
//     cout<<"des:type="<<des_t<<"(1-imm;2-mem;3-reg)\n";
//     cout<<"src:type="<<src_t<<"(1-imm;2-mem;3-reg)\n";
   //  cout<<"ModR/M="<<modrm.mod<<" "<<modrm.reg<<" "<<modrm.rm<<endl;
    // cout<<"SIB="<<sib.scale<<" "<<sib.index<<" "<<sib.base<<endl;
    // cout<<"disp32="<<instr.disp32<<",disp8="<<(int)instr.disp8<<"(<-"<<instr.disptype<<":(0-disp8;1-disp32) imm32="<<instr.imm32<<endl;
	//计算操作码索引 (mov,8,reg,reg)=000 (mov,8,reg,mem)=001 (mov,8,mem,reg)=010 (mov,8,reg,imm)=011
		  //(mov,32,reg,reg)=100 (mov,32,reg,mem)=101 (mov,32,mem,reg)=110 (mov,32,reg,imm)=111  [0-7]*(i_lea-i_mov)
  int index=-1;
  if(src_t==immd)//鉴别操作数种类
		index=3;
  else
		index=(des_t-2)*2+src_t-2;
  index=(opt-i_mov)*8+(1-len%4)*4+index;//附加指令名称和长度
  unsigned char opcode=i_2opcode[index];
  unsigned char exchar;
  switch(modrm.mod)
  {
	case -1://reg,imm
		switch(opt)
		{
			case i_mov://b0+rb MOV r/m8,imm8 b8+rd MOV r/m32,imm32 
				opcode+=(unsigned char)(modrm.reg);
				writeBytes(opcode,1);
				break;
			case i_cmp://80 /7 ib CMP r/m8,imm8 81 /7 id CMP r/m32,imm32 
				writeBytes(opcode,1);
				exchar=0xf8;
				exchar+=(unsigned char)(modrm.reg);
				writeBytes(exchar,1);
				break;
			case i_add://80 /0 ib ADD r/m8, imm8 81 /0 id ADD r/m32, imm32 
				writeBytes(opcode,1);
				exchar=0xc0;
				exchar+=(unsigned char)(modrm.reg);
				writeBytes(exchar,1);
				break;
			case i_sub://80 /5 ib SUB r/m8, imm8 81 /5 id SUB r/m32, imm32
				writeBytes(opcode,1);
				exchar=0xe8;
				exchar+=(unsigned char)(modrm.reg);
				writeBytes(exchar,1);
				break;
		}
		//可能的重定位位置 mov eax,@buffer,也有可能是mov eax,@buffer_len，就不许要重定位，因为是宏
		processRel(R_386_32);
		writeBytes(instr.imm32,len);//一定要按照长度输出立即数
		break;
	case 0://[reg],reg reg,[reg]
		writeBytes(opcode,1);
		writeModRM();
		if(modrm.rm==5)//[disp32]
		{
			processRel(R_386_32);//可能是mov eax,[@buffer],后边disp8和disp32不会出现类似情况
			instr.writeDisp();//地址肯定是4字节长度
		}
		else if(modrm.rm==4)//SIB
		{
			writeSIB();
		}
		break;
	case 1://[reg+disp8],reg reg,[reg+disp8]
		writeBytes(opcode,1);
		writeModRM();
		if(modrm.rm==4)
			writeSIB();
		instr.writeDisp();
		break;
	case 2://[reg+disp32],reg reg,[reg+disp32]
		writeBytes(opcode,1);
		writeModRM();
		if(modrm.rm==4)
			writeSIB();
		instr.writeDisp();
		break;
	case 3://reg,reg
		writeBytes(opcode,1);
		writeModRM();
		break;
  }
}

void gen1op(symbol opt,int opr_t,int len)
{
  int oldAddr=lb_record::curAddr;
  unsigned char exchar;
  unsigned short int opcode=i_1opcode[opt-i_call];
  if(opt==i_call||opt>=i_jmp&&opt<=i_jna)
  {
  	//统一使用长地址跳转，短跳转不好定位
  	if(opt==i_call||opt==i_jmp)
  		writeBytes(opcode,1);
  	else
  	{
  		writeBytes(opcode>>8,1);
  		writeBytes(opcode,1);
  	}	
  	int rel=instr.imm32-(lb_record::curAddr+4);//调用符号地址相对于下一条指令地址的偏移，因此加4
    bool ret=processRel(R_386_PC32);//处理可能的相对重定位信息，call fun,如果fun是本地定义的函数就不会重定位了
    if(ret)//相对重定位成功，说明之前计算的偏移错误
    	rel=-4;//对于链接器必须的初始值
    writeBytes(rel,4);
  }
  else if(opt==i_int)
  {
    writeBytes(opcode,1);
    writeBytes(instr.imm32,1);
  }
  else if(opt==i_push)
  {
    if(opr_t==immd)
    {
			opcode=0x68;
			writeBytes(opcode,1);
			writeBytes(instr.imm32,4);
    }
    else
    {
		 opcode+=(unsigned char)(modrm.reg);
		 writeBytes(opcode,1);
    }
  }
  else if(opt==i_inc)
  {
    if(len==1)
    {
			opcode=0xfe;
			writeBytes(opcode,1);
			exchar=0xc0;
			exchar+=(unsigned char)(modrm.reg);
			writeBytes(exchar,1);
    }
    else
    {
			opcode+=(unsigned char)(modrm.reg);
			writeBytes(opcode,1);
    }
  }
  else if(opt==i_dec)
  {
    if(len==1)
    {
			opcode=0xfe;
			writeBytes(opcode,1);
			exchar=0xc8;
			exchar+=(unsigned char)(modrm.reg);
			writeBytes(exchar,1);
    }
    else
    {
			opcode+=(unsigned char)(modrm.reg);
			writeBytes(opcode,1);
    }
  }
  else if(opt==i_neg)
  {
    if(len==1)
			opcode=0xf6;
    exchar=0xd8;
    exchar+=(unsigned char)(modrm.reg);
    writeBytes(opcode,1);
    writeBytes(exchar,1);
  }
  else if(opt==i_pop)
  {
    opcode+=(unsigned char)(modrm.reg);
    writeBytes(opcode,1);
  }
  else if(opt==i_imul||opt==i_idiv)
  {
    writeBytes(opcode,1);
    if(opt==i_imul)
			exchar=0xe8;
    else
			exchar=0xf8;
    exchar+=(unsigned char)(modrm.reg);
    writeBytes(exchar,1);
  }
}

void gen0op(symbol opt)
{
  int oldAddr=lb_record::curAddr;
	unsigned char opcode=i_0opcode[0];
  writeBytes(opcode,1);
}



