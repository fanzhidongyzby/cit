#ifndef _SEMANTIC_H_
#define _SEMANTIC_H_
#include "common.h"
#include <ext/hash_map>
#include<iostream>
#include<string.h>
using namespace std;
#include <vector>
using namespace __gnu_cxx;
#include "elf_file.h"
/**  *****************************************************************************************************************************
                                                             ***语义处理*** ********************************************************************************************************************************/
struct lb_record//符号声明记录
{
  static int curAddr;//一个段内符号的偏移累加量
  string segName;//隶属于的段名，三种：.text .data .bss
  string lbName;//符号名
  bool isEqu;//是否是L equ 1
  bool externed;//是否是外部符号，内容是1的时候表示为外部的，此时curAddr不累加
  int addr;//符号段偏移
  int times;//定义重复次数
  int len;//符号类型长度：db-1 dw-2 dd-4
  int *cont;//符号内容数组
  int cont_len;//符号内容长度
  lb_record(string n,bool ex);//L:或者创建外部符号(ex=true:L dd @e_esp)
  lb_record(string n,int a);//L equ 1
  lb_record(string n,int t,int l,int c[],int c_l);//L times 5 dw 1,"abc",L2 或者 L dd 23
  void write();//输出符号内容
  ~lb_record();
};

class Table//符号表
{
public:
  hash_map<string, lb_record*, string_hash> lb_map;//符号声明列表
  vector<lb_record*>defLbs;//记录数据定义符号顺序
  int hasName(string name);
  void addlb(lb_record*p_lb);//添加符号
  lb_record * getlb(string name);//获取已经定义的符号
  void switchSeg();//切换下一个段，由于一般只有.text和.data，因此可以此时创建段表项目
	void exportSyms();//导出所有的符号到elf
  void write();
  ~Table();//注销所有空间
};

struct ModRM//modrm字段
{
  int mod;//0-1
  int reg;//2-4
  int rm;//5-7
  ModRM();
  void init();
};
struct SIB//sib字段
{
  int scale;//0-1
  int index;//2-4
  int base;//5-7
  SIB();
  void init();
};
struct Inst//指令的其他部分
{
  unsigned char opcode;
  int disp;
  int imm32;
  int dispLen;//偏移的长度
  Inst();
  void init();
  void setDisp(int d,int len);//设置disp，自动检测disp长度（符号），及时是无符号地址值也无妨
  void writeDisp();
};
#endif
