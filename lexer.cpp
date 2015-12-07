#include "lexer.h"
#include "token.h"
#include "error.h"
#include "compiler.h"

/*******************************************************************************
                                   扫描器
*******************************************************************************/

Scanner::Scanner(char* name)
{
	file=fopen(name,"r");//打开指定的待扫描文件
	if(!file){
		printf(FATAL"文件%s打开失败！请检查文件名和路径。\n",name);
		Error::errorNum++;//错误数累加
	}
	fileName=name;
	//初始化扫描状态
	lineLen=0;
	readPos=-1;
	lastch=0;
	lineNum=1;
	colNum=0;	
}

Scanner::~Scanner()
{
	if(file)
	{
		PDEBUG(WARN"文件未全部扫描！\n");
		Error::warnNum++;//警告数累加
		fclose(file);
	}
}

/*
	显示字符
*/
void Scanner::showChar(char ch)
{
	if(ch==-1)printf("EOF");
	else if(ch=='\n')printf("\\n");
	else if(ch=='\t')printf("\\t");
	else if(ch==' ')printf("<blank>");
	else printf("%c",ch);	
	printf("\t\t<%d>\n",ch);
}

/*
	基于缓冲区的字符扫描算法,文件扫描接受后自动关闭文件
	缓冲区：使用fread读取效率更高，数据更集中
	字符：从缓冲区内索引获取
*/
int Scanner::scan()
{
	if(!file)//没有文件
		return -1;
	if(readPos==lineLen-1)//缓冲区读取完毕
	{
		lineLen=fread(line,1,BUFLEN,file);//重新加载缓冲区数据
		if(lineLen==0)//没有数据了
		{
			//标记文件结束,返回文件结束标记-1
			lineLen=1;
			line[0]=-1;
		}
		readPos=-1;//恢复读取位置
	}
	readPos++;//移动读取点
	char ch=line[readPos];//获取新的字符
	if(lastch=='\n')//新行
	{
		lineNum++;//行号累加
		colNum=0;//列号清空
	}
	if(ch==-1)//文件结束，自动关闭
	{
		fclose(file);
		file=NULL;
	}
	else if(ch!='\n')//不是换行
		colNum++;//列号递增
	lastch=ch;//记录上个字符
	if(Args::showChar)showChar(ch);
	return ch;
}

/*
	获取文件名
*/
char* Scanner::getFile()
{
	return fileName;
}

/*
	获取行号
*/
int Scanner::getLine()
{
	return lineNum;
}

/*
	获取列号
*/
int Scanner::getCol()
{
	return colNum;
}

/*******************************************************************************
                                   关键字表
*******************************************************************************/

/*
	关键字列表初始化
*/
Keywords::Keywords()
{
	//add keyword mapping here ~
	keywords["int"]=KW_INT;
	keywords["char"]=KW_CHAR;
	keywords["void"]=KW_VOID;
	keywords["extern"]=KW_EXTERN;
	keywords["if"]=KW_IF;
	keywords["else"]=KW_ELSE;
	keywords["switch"]=KW_SWITCH;
	keywords["case"]=KW_CASE;
	keywords["default"]=KW_DEFAULT;
	keywords["while"]=KW_WHILE;
	keywords["do"]=KW_DO;
	keywords["for"]=KW_FOR;
	keywords["break"]=KW_BREAK;
	keywords["continue"]=KW_CONTINUE;
	keywords["return"]=KW_RETURN;
}
/*
	测试是否是关键字
*/
Tag Keywords::getTag(string name)
{
	return keywords.find(name)!=keywords.end()?keywords[name]:ID;
}

/*******************************************************************************
                                   词法分析器
*******************************************************************************/

Keywords Lexer::keywords;

Lexer::Lexer (Scanner&sc):scanner(sc)
{
	token=NULL;//初始化词法记号记录，该变量被共享
	ch=' ';//初始化为空格
}

Lexer::~Lexer ()
{
	if(!token)//删除已经记录的词法记号变量的内存，防止内存溢出
	{
		delete token;
	}
}

/*
	封装的扫描方法
*/
bool Lexer::scan(char need)
{
	ch=scanner.scan();//扫描出字符
	if(need){
		if(ch!=need)//与预期不吻合
			return false;
		ch=scanner.scan();//与预期吻合，扫描下一个
		return true;
	}
	return true;
}

//打印词法错误
#define LEXERROR(code) Error::lexError(code)

/*
	有限自动机匹配，词法记号解析
*/
Token* Lexer::tokenize()
{
	for(;ch!=-1;){//过滤掉无效的词法记号，只保留正常词法记号或者NULL
		Token*t=NULL;//初始化一个词法记号指针
		while(ch==' '||ch=='\n'||ch=='\t')//忽略空白符
			scan();
		//标识符 关键字
		if(ch>='a'&&ch<='z'||ch>='A'&&ch<='Z'||ch=='_'){
			string name="";
			do{
				name.push_back(ch);//记录字符
				scan();
			}while(ch>='a'&&ch<='z'||ch>='A'&&ch<='Z'||ch=='_'||ch>='0'&&ch<='9');
			//匹配结束
			Tag tag=keywords.getTag(name);
			if(tag==ID)//正常的标志符
				t=new Id(name);
			else//关键字
				t=new Token(tag);
		}
		//字符串
		else if(ch=='"'){
			string str="";
			while(!scan('"')){
				if(ch=='\\'){//转义
					scan();
					if(ch=='n')str.push_back('\n');
					else if(ch=='\\')str.push_back('\\');
					else if(ch=='t')str.push_back('\t');
					else if(ch=='"')str.push_back('"');
					else if(ch=='0')str.push_back('\0');
					else if(ch=='\n');//什么也不做，字符串换行
					else if(ch==-1){
						LEXERROR(STR_NO_R_QUTION);
						t=new Token(ERR);
						break;
					}
					else str.push_back(ch);
				}
				else if(ch=='\n'||ch==-1){//文件结束
					LEXERROR(STR_NO_R_QUTION);
					t=new Token(ERR);
					break;
				}
				else
					str.push_back(ch);
			}
			//最终字符串
			if(!t)t=new Str(str);
		}
		//数字
		else if(ch>='0'&&ch<='9'){
			int val=0;
			if(ch!='0'){//10进制
				do{
					val=val*10+ch-'0';
					scan();
				}while(ch>='0'&&ch<='9');
			}
			else{
				scan();
				if(ch=='x'){//16进制
					scan();
					if(ch>='0'&&ch<='9'||ch>='A'&&ch<='F'||ch>='a'&&ch<='f'){
						do{
							val=val*16+ch;
							if(ch>='0'&&ch<='9')val-='0';
							else if(ch>='A'&&ch<='F')val+=10-'A';
							else if(ch>='a'&&ch<='f')val+=10-'a';							
							scan();
						}while(ch>='0'&&ch<='9'||ch>='A'&&ch<='F'||ch>='a'&&ch<='f');
					}
					else{
						LEXERROR(NUM_HEX_TYPE);//0x后无数据
						t=new Token(ERR);
					}
				}
				else if(ch=='b'){//2进制
					scan();
					if(ch>='0'&&ch<='1'){
						do{
							val=val*2+ch-'0';
							scan();
						}while(ch>='0'&&ch<='1');
					}
					else{
						LEXERROR(NUM_BIN_TYPE);//0b后无数据
						t=new Token(ERR);
					}
				}
				else if(ch>='0'&&ch<='7'){//8进制
					do{
						val=val*8+ch-'0';
						scan();
					}while(ch>='0'&&ch<='7');
				}
			}
			//最终数字
			if(!t)t=new Num(val);
		}
		//字符
		else if(ch=='\''){
			char c;
			scan();//同字符串
			if(ch=='\\'){//转义
				scan();
				if(ch=='n')c='\n';
				else if(ch=='\\')c='\\';
				else if(ch=='t')c='\t';
				else if(ch=='0')c='\0';
				else if(ch=='\'')c='\'';
				else if(ch==-1||ch=='\n'){//文件结束 换行
					LEXERROR(CHAR_NO_R_QUTION);
					t=new Token(ERR);
				}
				else c=ch;//没有转义
			}
			else if(ch=='\n'||ch==-1){//行 文件结束
				LEXERROR(CHAR_NO_R_QUTION);
				t=new Token(ERR);
			}
			else if(ch=='\''){//没有数据
				LEXERROR(CHAR_NO_DATA);
				t=new Token(ERR);
				scan();//读掉引号
			}
			else c=ch;//正常字符
			if(!t){
				if(scan('\'')){//匹配右侧引号,读掉引号
					t=new Char(c);
				}
				else{
					LEXERROR(CHAR_NO_R_QUTION);
					t=new Token(ERR);
				}
			}			
		}
		else{
			switch(ch)//界符
			{
				case '#'://忽略行（忽略宏定义）
					while(ch!='\n' && ch!= -1)
						scan();//行 文件不结束
					t=new Token(ERR);
					break;
				case '+':
					t=new Token(scan('+')?INC:ADD);break;
				case '-':
					t=new Token(scan('-')?DEC:SUB);break;
				case '*':
					t=new Token(MUL);scan();break;
				case '/':
					scan();
					if(ch=='/'){//单行注释
						while(ch!='\n' && ch!= -1)
							scan();//行 文件不结束
						t=new Token(ERR);
					}
					else if(ch=='*'){//多行注释,不允许嵌套注释
						while(!scan(-1)){//一直扫描
							if(ch=='*'){
								if(scan('/'))break;
							}
						}
						if(ch==-1)//没正常结束注释
							LEXERROR(COMMENT_NO_END);
						t=new Token(ERR);
					}
					else
						t=new Token(DIV);
					break;
				case '%':
					t=new Token(MOD);scan();break;
				case '>':
					t=new Token(scan('=')?GE:GT);break;
				case '<':
					t=new Token(scan('=')?LE:LT);break;
				case '=':
					t=new Token(scan('=')?EQU:ASSIGN);break;
				case '&':
					t=new Token(scan('&')?AND:LEA);break;
				case '|':
					t=new Token(scan('|')?OR:ERR);
					if(t->tag==ERR)
						LEXERROR(OR_NO_PAIR);//||没有一对
					break;
				case '!':
					t=new Token(scan('=')?NEQU:NOT);break;
				case ',':
					t=new Token(COMMA);scan();break;
				case ':':
					t=new Token(COLON);scan();break;
				case ';':
					t=new Token(SEMICON);scan();break;
				case '(':
					t=new Token(LPAREN);scan();break;
				case ')':
					t=new Token(RPAREN);scan();break;
				case '[':
					t=new Token(LBRACK);scan();break;
				case ']':
					t=new Token(RBRACK);scan();break;
				case '{':
					t=new Token(LBRACE);scan();break;
				case '}':
					t=new Token(RBRACE);scan();break;
				case -1:scan();break;
				default:
					t=new Token(ERR);//错误的词法记号
					LEXERROR(TOKEN_NO_EXIST);
					scan();
			}
		}
		//词法记号内存管理
		if(token)delete token;
		token=t;//强制记录
		if(token&&token->tag!=ERR)//有效,直接返回
			return token;
		else
			continue;//否则一直扫描直到结束
	}
	//文件结束
	if(token)delete token;
	return token=new Token(END);
}





















