/* 
	本文件封装了标准ELF文件格式。
*/

#ifndef _ELF_FILE_H
#define	_ELF_FILE_H

//elf文件所有数据结构和宏定义
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

//构造重定位项信息
struct RelItem
{
	string segName;//重定位的目标段名
	Elf32_Rel*rel;//重定位信息
	string relName;//重定位符号名
	RelItem(string sname,Elf32_Rel*r,string rname);
	~RelItem();
};


//elf文件类，包含elf文件的重要内容，处理elf文件
class Elf_file
{
public:
	//elf文件重要数据结构
	Elf32_Ehdr ehdr;//文件头
	vector<Elf32_Phdr*>phdrTab;//程序头表
	hash_map<string, Elf32_Shdr*,string_hash> shdrTab;//段表
	vector<string>shdrNames;//段表名和索引的映射关系，方便符号查询自己的段信息
	hash_map<string,Elf32_Sym*,string_hash>symTab;//符号表
	vector<string>symNames;//符号名与符号表项索引的映射关系，对于重定位表生成重要
	vector<RelItem*>relTab;//重定位表
	//辅助数据
	char *elf_dir;//处理elf文件的目录
	char*shstrtab;//段表字符串表数据
	unsigned int shstrtabSize;//段表字符串表长
	char*strtab;//字符串表数据
	unsigned int strtabSize;//字符串表长
public:
	Elf_file();
	void readElf(const char *dir);//读入elf
	void getData(char*buf,Elf32_Off offset,Elf32_Word size);//读取数据
	int getSegIndex(string segName);//获取指定段名在段表下标
	int getSymIndex(string symName);//获取指定符号名在符号表下标
	void addPhdr(Elf32_Word type,Elf32_Off off,Elf32_Addr vaddr,Elf32_Word filesz,
		Elf32_Word memsz,Elf32_Word flags,Elf32_Word align);//添加程序头表项
	void addShdr(string sh_name,Elf32_Word sh_type,Elf32_Word sh_flags,Elf32_Addr sh_addr,Elf32_Off sh_offset,
			Elf32_Word sh_size,Elf32_Word sh_link,Elf32_Word sh_info,Elf32_Word sh_addralign,
			Elf32_Word sh_entsize);//添加一个段表项
	void addSym(string st_name,Elf32_Sym*);//添加一个符号表项
	void writeElf(const char*dir,int flag);//输出Elf文件
	~Elf_file();
};

#endif //elf_file.h
