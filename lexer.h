#pragma once
#include "common.h"
#include <ext/hash_map>
using namespace __gnu_cxx;
/*******************************************************************************
                                   扫描器
*******************************************************************************/

class Scanner
{
	//文件指针
	char*fileName;//文件名
	FILE*file;//文件指针
	
	//内部状态
	static const int BUFLEN=80;//扫描缓冲区长度
	char line[BUFLEN];
	int lineLen;//当前行的长度
	int readPos;//读取的位置
	char lastch;//上一个字符，主要用于判断换行位置	
	
	//读取状态
	int lineNum;//记录行号
	int colNum;//列号
	
	//显示字符
	void showChar(char ch);
	
public:

	//构造与初始化
	Scanner(char* name);
	~Scanner();
	
	//扫描
	int scan();//基于缓冲区的字符扫描算法,文件扫描接受后自动关闭文件
	
	//外部接口
	char*getFile();//获取文件名
	int getLine();//获取行号
	int getCol();//获取列号
	
};

/*******************************************************************************
                                   关键字表
*******************************************************************************/

class Keywords
{
	//hash函数
	struct string_hash{
		size_t operator()(const string& str) const{
			return __stl_hash_string(str.c_str());
		}
	};
	hash_map<string, Tag, string_hash> keywords;
public:
	Keywords();//关键字列表初始化
	Tag getTag(string name);//测试是否是关键字
};


/*******************************************************************************
                                   词法分析器
*******************************************************************************/

class Lexer
{
	static Keywords keywords;//关键字列表
	
	Scanner &scanner;//扫描器
	char ch;//读入的字符
	bool scan(char need=0);//扫描与匹配
	
	Token*token;//记录扫描的词法记号
	
public:
	Lexer (Scanner&sc);
	~Lexer();
	Token* tokenize();//有限自动机匹配，词法记号解析
};

