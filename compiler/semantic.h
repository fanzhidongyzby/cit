#ifndef _SEMANTIC_H_
#define _SEMANTIC_H_
#include "common.h"
#include <ext/hash_map>
#include<iostream>
#include<string.h>
using namespace std;
#include <vector>
using namespace __gnu_cxx;
/**  *****************************************************************************************************************************
                                                             ***语义处理***
   ********************************************************************************************************************************/
struct var_record//变量声明记录
{
  symbol type;//类型
  string name;//名称
  union//值
  {
    int intVal;
    char charVal;
    int voidVal;
    int strValId;//用strValId=-1来标示临时string
  };
  //int locstrlen;//临时string的长度
  int localAddr;//局部变量相对与ebp指针的地址，或者临时string的索引地址
  int externed;//标示变量是不是外部的符号，针对全局变量来作用
  var_record();//默认构造函数
  void init(symbol dec_type,string dec_name);//声明初始化函数
  void copy(const var_record*src);//拷贝函数
  var_record(const var_record& v);//拷贝构造函数
  ~var_record();
};
struct fun_record//函数声明记录
{
  symbol type;//返回类型
  string name;//名称
  vector<symbol> *args;//参数类型列表
  vector<var_record*>*localvars;//局部变量列表,指向哈希表,仅仅为函数定义服务
  int defined;//函数是否给出定义
  int flushed;//函数参数是否已经缓冲写入，标记是否再清楚的时候清除缓冲区
  int hadret;//记录是否含有返回语句
  fun_record();//默认构造函数
  fun_record(const fun_record& f);//拷贝构造函数
  void init(symbol dec_type,string dec_name);//初始化函数
  void addarg();//添加一个参数，默认使用tvar,同时修改pushlocalvar以防函数定义，要是声明就不管他的信息了
  int hasname(string id_name);//防止参数的名字在写入之前重复
  void pushlocalvar();//添加局部变量，默认使用tvar.
  int getCurAddr();//取得上一个进栈的变量的地址（相对于ebp）
  void flushargs();//将参数写到符号表
  void poplocalvars(int varnum);//弹出多个局部变量
  int equal(fun_record&f);
  var_record*create_tmpvar(symbol type,int hasVal,int &var_num);//根据常量添加一个临时变量，记得var_num++;
  ~fun_record();
};

// 需要自己写hash函数
struct string_hash
{
  size_t operator()(const string& str) const
  {
    return __stl_hash_string(str.c_str());
  }
};

class Table//变量表
{
  hash_map<string, var_record*, string_hash> var_map;//变量声明列表
  hash_map<string, fun_record*, string_hash> fun_map;//函数声明列表
  vector<string*>stringTable;//串空间
  vector<var_record*> real_args_list;//函数调用的实参列表，用于检查参数调用匹配和实参代码生成
public:
  Table();
  int addstring();//返回串空间的索引,仅仅是记录串值其他符号串自动存储
  string getstring(int index);//根据索引取字符串
  void addvar();//添加变量声明记录,默认使用tvar
  void addvar(var_record*v_r);//添加变量声明记录,重载
  var_record * getVar(string name);//获取已经定义的变量
  int hasname(string id_name);//测试名字是否重复，主要应对变量的重复定义
  void delvar(string var_name);//删除变量记录
  void addfun();//添加函数声明记录,默认使用tfun
  void addrealarg(var_record*arg,int& var_num);//为数据结构添加一个实际参数记录，用于代码生成
  var_record* genCall(string fname,int& var_num);//产生实参的代码
  void over();//最后的代码生成
  void clear();//清空所有符号表
  ~Table();//注销所有空间
};
#endif
