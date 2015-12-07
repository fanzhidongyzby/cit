#pragma once

#include"common.h"
#include"arm-regtypes.h"

/*
	寄存器池类，用于选择一个未被占用的的寄存器
*/
class RegPool
{
	static unsigned int reg_used;//寄存器使用位图
public:
	static int getReg();//获取一个空闲的寄存器,返回合法的寄存器编号,无空闲寄存器返回-NOMOREREG
	static void putReg(int reg_num);//返回一个寄存器,要求编号合法，最好与getReg配对使用
	static void putAll();//放回所有寄存器
	static bool isEmpty();//判断寄存器池是否为空
	static int inUse(unsigned int reg_id);//判断某个寄存器是否正在使用，即可被申请
	static bool useReg(unsigned int reg_id);//点名使用某个寄存器
	static unsigned int regnum_id(int reg_num);//寄存器编号转化为位图
	static int regid_num(unsigned int reg_id);//寄存器位图转化为编号
};

//初始化寄存器位图
unsigned int RegPool::reg_used=R_USABLE;

/*
	重新初始化寄存器位图
*/
void RegPool::putAll()
{
	reg_used=R_USABLE;
}

/*
	按照寄存器编号从小到大的顺序获得寄存器编号
	编号范围由R_USABLE决定。
	没有空闲的寄存器返回-NOMOREREG。
*/
int RegPool::getReg()
{
	unsigned int regs=reg_used;
	unsigned int reg_id=1;//寄存器的位图
	int reg_num=0;//寄存器的编号
	while(regs&1)
	{
		reg_id<<=1;
		reg_num++;
		regs>>=1;
	}
	if(!reg_id)//是否申请到寄存器
		reg_num=-NOMOREREG;//防止编号溢出
	else
		reg_used|=reg_id;//更新寄存器位图
	return reg_num;
}

/*
	将寄存器编号转化为位图，需要验证编号的合法性
*/
unsigned int RegPool::regnum_id(int reg_num)
{
	#ifdef DEBUG
	if(reg_num<0||reg_num>=MAXBIT)
	{
		PDEBUG(ERROR"寄存器编号%d越界.\n",reg_num);
		return 0;
	}
	#endif
	unsigned int reg_id = 1<<reg_num;
	#ifdef DEBUG
	if(reg_id&R_LEGAL)//非法位图
	{
		PDEBUG(ERROR"寄存器编号%d存在非法访问.\n",reg_num);
		return 0;
	}
	#endif
	return reg_id;
}

/*
	将编号对应的寄存器位图清0
*/
void RegPool::putReg(int reg_num)
{
	unsigned int reg_id=regnum_id(reg_num);
	#ifdef DEBUG
	if(!reg_id)//无效编号
		return;
	if(!(reg_id&reg_used))
	{
		PDEBUG(WARN"寄存器R%d未被使用.\n",reg_num);
		return;
	}
	#endif
	reg_used &= ~reg_id;
}

/*
	判断寄存器池是否为空
*/
bool RegPool::isEmpty()
{
	return !(reg_used^0xffffffff);//测试所有寄存器位
}

/*
	将寄存器位图转化为编号,需要验证位图的合法性
	寄存器位图必须满足R_LEGAL限制，并且只能有一位为1
*/
int RegPool::regid_num(unsigned int reg_id)
{
	#ifdef DEBUG
	if(!reg_id||(reg_id&R_LEGAL))//非法位图
	{
		PDEBUG(ERROR"寄存器位图0x%08x存在非法访问.\n",reg_id);
		return -WRONGARGS;
	}
	#endif
	int reg_num=0;
	unsigned int temp=reg_id;
	while(!(temp&1))//转换
	{
		temp>>=1;
		reg_num++;
	}
	#ifdef DEBUG
	if((1<<reg_num)!=reg_id)//非0位过多
	{
		PDEBUG(ERROR"寄存器位图0x%08x信息错误.\n",reg_id);
		reg_num = -WRONGARGS;
	}
	#endif
	return reg_num;
}

/*
	测试一个寄存器位图对应的寄存器是否已经被占用，建议使用宏作为参数
*/
int RegPool::inUse(unsigned int reg_id)
{
	#ifdef DEBUG
	if(regid_num(reg_id) == -WRONGARGS)
	{
		return -WRONGARGS;
	}
	#endif
	return !!(reg_used&reg_id);//测试寄存器位
}

/*
	要求使用一个特定的寄存器，指定该寄存器的位图，建议使用宏作为参数
*/
bool RegPool::useReg(unsigned int reg_id)
{
	int stats=inUse(reg_id);
	if(!stats)
	{
		reg_used|=reg_id;
		return true;
	}
	else if(stats==1)
	{
		PDEBUG(WARN"寄存器(位图=0x%08x)已经被占用.\n",reg_id);
	}
	return false;
}






