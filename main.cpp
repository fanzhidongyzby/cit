#include "compiler.h"
#include "error.h"
#include<iostream>
#include<cstring>

/*
	命令格式：compile 源文件[源文件] [选项]
	选项：
		-o						#执行优化
		-char					#显示文件字符
		-token				#显示词法记号
		-symbol				#显示符号表信息
		-ir						#显示中间代码
		-or						#显示优化后的中间代码
		-block				#显示基本块和流图关系
		-h						#显示帮助信息
*/
int main(int argc,char *argv[])
{
	vector<char*>srcfiles;//源文件
	if(argc>1){//至少1个参数
		for(int i=1;i<argc-1;i++){//取最后一个参数之前的参数
			srcfiles.push_back(argv[i]);
		}
		char*opt=argv[argc-1];//最后一个参数
		if(!strcmp(opt,"-char"))Args::showChar=true;
		else if(!strcmp(opt,"-token"))Args::showToken=true;
		else if(!strcmp(opt,"-symbol"))Args::showSym=true;
		else if(!strcmp(opt,"-ir"))Args::showIr=true;
		else if(!strcmp(opt,"-or")){Args::opt=true;Args::showOr=true;}
		else if(!strcmp(opt,"-block")){Args::opt=true;Args::showBlock=true;}
		else if(!strcmp(opt,"-o"))Args::opt=true;
		else if(!strcmp(opt,"-h"))Args::showHelp=true;
		else srcfiles.push_back(opt);
	}
	if(Args::showHelp){
		cout<<
					"命令格式：cit 源文件[源文件][选项]\n"
					"选项：\n"
					"\t-o\t\t#执行优化\n"
					"\t-char\t\t#显示文件字符\n"
					"\t-token\t\t#显示词法记号\n"
					"\t-symbol\t\t#显示符号表信息\n"
					"\t-ir\t\t#显示中间代码\n"
					"\t-or\t\t#显示优化后的中间代码\n"
					"\t-block\t\t#显示基本块和流图关系\n"
					"\t-h\t\t#显示帮助信息\n"
				;
	}
	else if(srcfiles.size()){//存在源文件
		Compiler compiler;
		for(int i=0;i<srcfiles.size();i++){
			compiler.compile(srcfiles[i]);
		}
		int error=Error::getErrorNum();
		int warn=Error::getWarnNum();
		cout<<"编译完成：错误="<<error<<",警告="<<warn<<"."<<endl;
	}
	else
		cout<<"命令格式错误！(使用-h选项查看帮助)"<<endl;
	return 0;
}
