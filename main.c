/**
编译器，汇编器，链接器命令行整合
命令行格式：./cit 源文件+ 输出文件 [开关]
	源文件：*.c
	输出文件：任意文件名
	开关：
		-s 输出汇编文件*.s
		-o 输出目标文件*.o
		-x 显示所有信息，等价于 -c -a -l
		-c 显示编译信息，等价于-lex -syn -sem -gen
		-lex 显示词法分析过程
		-syn 显示语法分析过程
		-sem 显示语义分析过程
		-gen 显示代码生成过程
		-tab 显示符号表信息
		-a 显示汇编信息
		-l 显示链接信息
		-h 显示帮助信息
		-v 显示版本信息
*/
#include <iostream>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
using namespace std;
void work();
void exeCmd(string cmd)
{
	system(cmd.c_str());
}
bool genAsm=false
		,genObj=false
		,showLex=false
		,showSyn=false
		,showSem=false
		,showGen=false
		,showTab=false
		,showAss=false
		,showLink=false;
vector<string>srcFileNames;//源文件目录列表
vector<string>kernelFileNames;//源文件核心文件名列表
string desFileName="";
string curDir;//当前程序绝对目录
int main(int argc,char*argv[])
{
	char buffer[256];
 	curDir=getcwd(buffer,256);
 	curDir+="/";
	if(argc==1||argc==2&&strcmp(argv[1],"-h")==0)
	{
		cout<<"CIT2.0命令格式：[源文件[源文件] 输出文件 [选项]][-h|-v]\n\n"
			<<"\t源文件\t\t必须是以.c结尾的文件\n"
			<<"\t输出文件\t必须是合法的文件路径\n"
			<<"\t-s\t\t输出汇编文件*.s\n"
			<<"\t-o\t\t输出目标文件*.o\n"
			<<"\t-x\t\t显示所有信息，等价于 -c -a -l\n"
			<<"\t-c\t\t显示编译信息，等价于 -lex -syn -sem -gen\n"
			<<"\t-lex\t\t显示词法分析过程\n"
			<<"\t-syn\t\t显示语法分析过程\n"
			<<"\t-sem\t\t显示语义分析过程\n"
			<<"\t-gen\t\t显示代码生成过程\n"
			<<"\t-tab\t\t显示符号表信息\n"
			<<"\t-a\t\t显示汇编信息\n"
			<<"\t-l\t\t显示链接信息\n"
			<<"\t-h\t\t显示帮助信息\n"
			<<"\t-v\t\t显示版本信息\n\n";
			return 0;
	}
	else if(argc==2&&strcmp(argv[1],"-v")==0)
	{
		cout<<"CIT 2.0 版本(BY FLORIAN)\n版权(c)2012 中国石油大学\n"
  		<<"这是一个名字叫\"cit\"(just compile it!)的小型C编译器，通过它可以加深对编译，汇编，链接过程的理解。\n";
		return 0;
	}
	bool flag=true;//标志命令格式合法
	string arg;
	int i=0;
	#define NEXT if(flag){arg=argv[i];i++;if(i>=argc)flag=false;}//读入一个命令单元
	NEXT
	while(flag)
	{
		NEXT
		int index=arg.rfind(".c");//.c位置
		if(index==-1||index!=arg.length()-2)//不存在.c或者最后一个.c不在最后
		{
			int x=access(arg.c_str(),W_OK);
			if((arg[0]!='-')&&(arg[arg.size()-1]!='/'))
			{
				desFileName=arg;//记录输出文件名
			}
			else
			{
				cout<<"请指定输出文件！"<<endl;
				return 0;
			}
			break;
		}
		if(access(arg.c_str(),R_OK)!=0)
		{
			cout<<"源文件"+arg+"无法访问！"<<endl;
			return 0;
		}
		if(arg.find("/")!=0)//相对路径
			arg=curDir+arg;
		srcFileNames.push_back(arg);//记录一个源文件名
		string fileName=arg;		
		int start=fileName.rfind("/");
		if(start==-1)start=0;
		int end=fileName.rfind(".");
		string realName="work"+fileName.substr(start,end-start);
		kernelFileNames.push_back(curDir+realName);
	}
	kernelFileNames.push_back(curDir+"work/common");//记录附加文件
	if(srcFileNames.size()==0)//没有源文件
	{
		cout<<"请指定源文件！"<<endl;
		return 0;
	}
	if(desFileName=="")//还没有记录输出文件
	{
		if(flag)
		{
			NEXT
			if((arg[0]!='-')&&(arg[arg.size()-1]!='/'))
			{
				desFileName=arg;//记录输出文件名
			}
		}
		else
		{
			cout<<"请指定输出文件！"<<endl;
			return 0;
		}
	}
	if(desFileName.find("/")!=0)//相对路径
			desFileName=curDir+"work/"+desFileName;
	//测试输出路径的有效性
	FILE*f=fopen(desFileName.c_str(),"wb");
	if(f==0)
	{
		cout<<"输出文件路径无效！"<<endl;
		return 0;
	}
	else
	{
		fclose(f);
		remove(desFileName.c_str());//删除测试的文件
	}
	while(flag)
	{
		NEXT
		if(arg=="-s")genAsm=true;
		if(arg=="-o")genObj=true;
		if(arg=="-x")showLex=showSyn=showSem=showGen=showTab=showAss=showLink=true;
		if(arg=="-c")showLex=showSyn=showSem=showGen=showTab=true;
		if(arg=="-lex")showLex=true;
		if(arg=="-syn")showSyn=true;
		if(arg=="-sem")showSem=true;
		if(arg=="-gen")showGen=true;
		if(arg=="-tab")showTab=true;
		if(arg=="-a")showAss=true;
		if(arg=="-l")showLink=true;
	}
	work();
	return 0;
}
void work()
{
	cout<<"开始检测输入文件信息...\n";
	//编译
	string opt="";
	opt+=(showLex)?" y":" n";
	opt+=(showSyn)?" y":" n";
	opt+=(showSem)?" y":" n";
	opt+=(showGen)?" y":" n";
	opt+=(showTab)?" y":" n";	
	
	for(int i=0;i<srcFileNames.size();i++)
	{
		cout<<"正在编译文件"<<srcFileNames[i]<<"。\n";
		exeCmd("cd ./compiler\n./compiler "+srcFileNames[i]+opt+"\ncd ..\n");
	}
	//汇编
	bool flag=true;
	for(int i=0;i<kernelFileNames.size();i++)
	{
		string realName=kernelFileNames[i]+".s";
		if(access(realName.c_str(),F_OK)!=0)
		{
			flag=false;
			cout<<"编译过程出现错误，操作停止。"<<endl;
			break;
		}
	}
	if(flag)//编译成功
	{
		opt="";
		opt+=(showAss)?" y":" n";
		string objNames="";
		for(int i=0;i<kernelFileNames.size();i++)
		{
			string realName=kernelFileNames[i]+".s";
			cout<<"正在汇编文件"<<kernelFileNames[i]+".s"<<"。\n";
			exeCmd("cd ./ass\n./ass "+kernelFileNames[i]+opt+"\ncd ..\n");
			objNames+=kernelFileNames[i]+".o ";
		}
		opt="";
		opt+=(showLink)?" y":" n";
		cout<<"正在执行链接。\n";
		objNames+=desFileName+" ";
		exeCmd("cd ./lit\n./lit "+objNames+opt+"\ncd ..\n");
		//cout<<"./lit "+objNames+opt+"\n";
		if(access(desFileName.c_str(),F_OK)!=0)
		{
			flag=false;
			cout<<"链接过程出现错误，操作停止。"<<endl;
		}
		else
		{
			exeCmd("chmod +x "+desFileName);//产生可执行权限
			cout<<"生成可执行文件"<<desFileName<<"。"<<endl;
		}
	}
	if(genAsm==false||flag==false)
	{
		//cout<<"清理汇编文件(work/*.s)。\n";
		for(int i=0;i<kernelFileNames.size();i++)
		{
			string realName=kernelFileNames[i]+".s";
			if(access(realName.c_str(),F_OK)==0)
			{
				remove(realName.c_str());
			}
		}
	}
	if(genObj==false||flag==false)
	{
		//cout<<"清理目标文件(work/*.o)。\n";
		for(int i=0;i<kernelFileNames.size();i++)
		{
			string realName=kernelFileNames[i]+".o";
			if(access(realName.c_str(),F_OK)==0)
			{
				remove(realName.c_str());
			}
		}
	}
	if(flag)
		cout<<"操作成功！"<<endl;
	else
	{
		remove(desFileName.c_str());//删除生成的错误文件
		cout<<"操作失败！"<<endl;
	}
}













