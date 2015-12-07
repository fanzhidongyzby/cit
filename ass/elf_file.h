/* 
	本文件封装了标准ELF文件格式。
*/

#ifndef _ELF_FILE_H
#define	_ELF_FILE_H

//elf文件所有数据结构和宏定义
#include "common.h"
#include"elf.h"
//STL模板库
#include <ext/hash_map>
#include <vector>
using namespace std;
using namespace __gnu_cxx;

// 需要自己写hash函数
struct string_hash
{
  size_t operator()(const string& str) const
  {
    return __stl_hash_string(str.c_str());
  }
};

//重定位信息
struct RelInfo
{
	string tarSeg;//重定位目标段
	int offset;//重定位位置的偏移
	string lbName;//重定位符号的名称
	int type;//重定位类型0-R_386_32；1-R_386_PC32
	RelInfo(string seg,int addr,string lb,int t);
};


//elf文件类，包含elf文件的重要内容，处理elf文件
class Elf_file
{
public:
	//elf文件重要数据结构
	Elf32_Ehdr ehdr;//文件头
	hash_map<string, Elf32_Shdr*,string_hash> shdrTab;//段表
	vector<string>shdrNames;//段表名和索引的映射关系，方便符号查询自己的段信息
	hash_map<string,Elf32_Sym*,string_hash>symTab;//符号表
	vector<string>symNames;//符号名与符号表项索引的映射关系，对于重定位表生成重要
	vector<RelInfo*>relTab;//重定位表
	//辅助数据
	char*shstrtab;//段表字符串表数据
	int shstrtabSize;//段表字符串表长
	char*strtab;//字符串表数据
	int strtabSize;//字符串表长
	vector<Elf32_Rel*>relTextTab,relDataTab;
public:
	Elf_file();
	int getSegIndex(string segName);//获取指定段名在段表下标
	int getSymIndex(string symName);//获取指定符号名在符号表下标
	void addShdr(string sh_name,int size);
	void addShdr(string sh_name,Elf32_Word sh_type,Elf32_Word sh_flags,Elf32_Addr sh_addr,Elf32_Off sh_offset,
			Elf32_Word sh_size,Elf32_Word sh_link,Elf32_Word sh_info,Elf32_Word sh_addralign,
			Elf32_Word sh_entsize);//添加一个段表项
	void addSym(lb_record*lb);
	void addSym(string st_name,Elf32_Sym*);//添加一个符号表项
	RelInfo* addRel(string seg,int addr,string lb,int type);//添加一个重定位项，相同段的重定位项连续（一般是先是.rel.text后.rel.data）
	void padSeg(string first,string second);//填充段间的空隙
	void assmObj();//组装文件
	void writeElfTail();//输出文件尾部
	void writeElf();
	void printAll();	
	~Elf_file();
};

#endif //elf_file.h
