#include "semantic.h"
/**  *****************************************************************************************************************************
							    ***语义处理***
  ********************************************************************************************************************************/
int lb_record::curAddr=0x00000000;//对于重定位文件是0，链接后开始于0x08048100
lb_record::lb_record(string n,bool ex)//L:或者(ex=true:L dd @e_esp)
{
  lbName=n;
  addr=lb_record::curAddr;
  externed=ex;
  isEqu=false;
  segName=curSeg;
  times=0;
  len=0;
  cont=NULL;
  cont_len=0;
  if(ex)
  {
		addr=0;
		segName="";
  }
}

lb_record::lb_record(string n,int a)//L equ 1
{
  lbName=n;
  segName=curSeg;
  addr=a;
  isEqu=true;
  externed=false;
  times=0;
  len=0;
  cont=NULL;
  cont_len=0;
}

lb_record::lb_record(string  n,int t,int l,int c[],int c_l)//L times 10 dw 10,"1234"
{
  lbName=n;
  addr=lb_record::curAddr;
  segName=curSeg;
  isEqu=false;
  times=t;
  len=l;
  cont_len=c_l;
  cont=new int[c_l];
  for(int i=0;i<c_l;i++)
	{
	  cont[i]=c[i];
	}
	/*
	——编译器没有输出extern，因此这里必然是有效数据
  externed=(cont[0]==1);//L dd 1说明L是外部的符号
  if(!externed)//内部符号累加地址
  {
    lb_record::curAddr+=t*l*c_l;//修改地址
  }
  else
  {
  	addr=0x00000000;//外部符号地址不确定,记作0
  	segName="";//不属于任何段
  }
  */
  externed=false;
  lb_record::curAddr+=t*l*c_l;//修改地址
  /*cout<<lbName<<":"<<t*l*c_l<<endl;
  for(int i=0;i<c_l;i++)
  	printf("%x ",c[i]);
  cout<<"*"<<t<<"<"<<l<<">\n";*/
}

void lb_record::write()
{
  for(int i=0;i<times;i++)
  {
    for(int j=0;j<cont_len;j++)
    {
    	writeBytes(cont[j],len);
    }
  }
}
lb_record::~lb_record()
{
	if(cont!=NULL)
	  delete[]cont;
}

int Table::hasName(string name)
{
  return(lb_map.find(name)!=lb_map.end());//没有
}

void Table::addlb(lb_record*p_lb)//添加符号
{ 
	if(scanLop!=1)//只在第一遍添加新符号
	{
		delete p_lb;//不添加
		return;
	}
  if(hasName(p_lb->lbName))//符号存在
  {
  	if(lb_map[p_lb->lbName]->externed=true&&p_lb->externed==false)//本地符号覆盖外部符号
  	{
  		delete lb_map[p_lb->lbName];
  		lb_map[p_lb->lbName]=p_lb;
  	}
  	//else情况不会出现，符号已经定义就不可能找不到该符号而产生未定义符号
  }
  else
  {
    lb_map[p_lb->lbName]=p_lb;
  }
  if(p_lb->times!=0&&p_lb->segName==".data")//包含数据段内容的符号：数据段内除了不含数据（times==0）的符号，外部符号段名为空
  {
  	defLbs.push_back(p_lb);
  }
  //cout<<p_lb->lbName<<" time="<<p_lb->times<<" len="
  //<<p_lb->len<<" cont_len="<<p_lb->cont_len<<" addr="<<p_lb->addr<<endl;
}

lb_record * Table::getlb(string name)
{
	lb_record*ret;
  if(hasName(name))
    ret=lb_map[name];
  else
  {
  	lb_record*p_lb=lb_map[name]=new lb_record(name,true);//未知符号，添加到符号表（仅仅添加了一次，第一次扫描添加的）
    ret=p_lb;
  }
  
  return ret;
}

void Table::switchSeg()
{
	if(scanLop==1)
	{
		dataLen+=(4-dataLen%4)%4;
		obj.addShdr(curSeg,lb_record::curAddr);//新建一个段
		if(curSeg!=".bss")
			dataLen+=lb_record::curAddr;
	}
	curSeg="";curSeg+=id;//切换下一个段名
	lb_record::curAddr=0;//清0段偏移
}

void  Table::exportSyms()
{
	hash_map<string, lb_record*, string_hash>::iterator lb_i,lb_iend;
  lb_iend=lb_map.end();
  for(lb_i=lb_map.begin();lb_i!=lb_iend;lb_i++)
  {
    lb_record *lr=lb_i->second;
    if(!lr->isEqu)//equ不用导出
        obj.addSym(lr);
  }
}

void Table::write()
{
	for(int i=0;i<defLbs.size();i++)
		defLbs[i]->write();
	if(showAss)
	{
		//只输出定义符号
		cout<<"------------定义符号------------"<<endl;
		for(int i=0;i<defLbs.size();i++)
		{
			cout<<defLbs[i]->lbName<<endl;
			/*
			cout<<":"<<defLbs[i]->times*defLbs[i]->len*defLbs[i]->cont_len<<endl;
		  for(int j=0;j<defLbs[i]->cont_len;j++)
		  	printf("%x ",defLbs[i]->cont[j]);
		  cout<<"*"<<defLbs[i]->times<<"<"<<defLbs[i]->len<<">\n";
		  */
		}
  }
}
Table::~Table()//注销所有空间
{
  hash_map<string, lb_record*, string_hash>::iterator lb_i,lb_iend;
  lb_iend=lb_map.end();
  for(lb_i=lb_map.begin();lb_i!=lb_iend;lb_i++)
  {
    delete lb_i->second;//删除记录对象
  }
  lb_map.clear();
}

Table table;//符号表全局对象

//ModRM,SIB,Inst结构
ModRM::ModRM()
{
	init();
}

void ModRM::init()
{
	mod=-1;
	reg=0;
	rm=0;
}

SIB::SIB()
{
	init();
}

void SIB::init()
{
	scale=-1;
	index=0;
	base=0;
}

Inst::Inst()
{
	init();
}

void Inst::init()
{
	opcode=0;
	disp=0;
	dispLen=0;
	imm32=0;
	modrm.init();
	sib.init();
}

//设置disp，自动检测disp长度（符号），及时是无符号地址值也无妨
void Inst::setDisp(int d,int len)
{
	dispLen=len;
	disp=d;
}
//按照记录的disp长度输出
void Inst::writeDisp()
{
  if(dispLen)
  {
    writeBytes(disp,dispLen);
    dispLen=0;//还原
  }    
}
  


