#include"elf_file.h"
#include<stdio.h>
#include "semantic.h"
#include "common.h"

Elf_file obj;//输出目标文件，使用fout输出


RelInfo::RelInfo(string seg,int addr,string lb,int t)
{
	offset=addr;
	tarSeg=seg;
	lbName=lb;
	type=t;
}


Elf_file::Elf_file()
{
	shstrtab=NULL;
	strtab=NULL;
	addShdr("",0,0,0,0,0,0,0,0,0);//空段表项
	addSym("",NULL);//空符号表项
}



int Elf_file::getSegIndex(string segName)
{
	int index=0;
	for(int i=0;i<shdrNames.size();++i)
	{
		if(shdrNames[i]==segName)//找到段
			break;
		++index;
	}
	return index;
}

int Elf_file::getSymIndex(string symName)
{
	int index=0;
	for(int i=0;i<symNames.size();++i)
	{
		if(symNames[i]==symName)//找到符号
			break;
		++index;
	}
	return index;
}

//sh_name和sh_offset都需要重新计算
void Elf_file::addShdr(string sh_name,int size)
{
	int off=52+dataLen;
	if(sh_name==".text")
	{
		addShdr(sh_name,SHT_PROGBITS,SHF_ALLOC|SHF_EXECINSTR,0,off,size,0,0,4,0);
	}
	else if(sh_name==".data")
	{
		addShdr(sh_name,SHT_PROGBITS,SHF_ALLOC|SHF_WRITE,0,off,size,0,0,4,0);
	}
	else if(sh_name==".bss")
	{
		addShdr(sh_name,SHT_NOBITS,SHF_ALLOC|SHF_WRITE,0,off,size,0,0,4,0);
	}
}

void Elf_file::addShdr(string sh_name,Elf32_Word sh_type,Elf32_Word sh_flags,Elf32_Addr sh_addr,Elf32_Off sh_offset,
			Elf32_Word sh_size,Elf32_Word sh_link,Elf32_Word sh_info,Elf32_Word sh_addralign,
			Elf32_Word sh_entsize)//添加一个段表项
{
	Elf32_Shdr*sh=new Elf32_Shdr();
	sh->sh_name=0;
	sh->sh_type=sh_type;
	sh->sh_flags=sh_flags;
	sh->sh_addr=sh_addr;
	sh->sh_offset=sh_offset;
	sh->sh_size=sh_size;
	sh->sh_link=sh_link;
	sh->sh_info=sh_info;
	sh->sh_addralign=sh_addralign;
	sh->sh_entsize=sh_entsize;
	shdrTab[sh_name]=sh;
	shdrNames.push_back(sh_name);
}

void Elf_file::addSym(lb_record*lb)
{
	//解析符号的全局性局部性，避免符号冲突
	bool glb=false;
	string name=lb->lbName;
	/*
	//对于@while_ @if_ @lab_ @cal_开头和@s_stack的都是局部符号，可以不用导出，但是为了objdump方便而导出
	*/
	if(name.find("@lab_")==0||name.find("@if_")==0||name.find("@while_")==0||name.find("@cal_")==0||name=="@s_stack")
		return;
	
	if(lb->segName==".text")//代码段
	{
		if(name=="@str2long"||name=="@procBuf")
			glb=true;
		else if(name[0]!='@')//不带@符号的，都是定义的函数或者_start,全局的
			glb=true;
	}
	else if(lb->segName==".data")//数据段
	{
		int index=name.find("@str_");
		if(index==0)//@str_开头符号
		{
			glb=!(name[5]>='0'&&name[5]<='9');//不是紧跟数字，全局str
		}
		else//其他类型全局符号
			glb=true;
	}
	else if(lb->segName=="")//外部符号
	{
		glb=lb->externed;//false
	}
	Elf32_Sym*sym=new Elf32_Sym();
	sym->st_name=0;
	sym->st_value=lb->addr;//符号段偏移,外部符号地址为0
	sym->st_size=lb->times*lb->len*lb->cont_len;//函数无法通过目前的设计确定，而且不必关心
	if(glb)//统一记作无类型符号，和链接器lit协议保持一致
		sym->st_info=ELF32_ST_INFO(STB_GLOBAL,STT_NOTYPE);//全局符号
	else
		sym->st_info=ELF32_ST_INFO(STB_LOCAL,STT_NOTYPE);//局部符号，避免名字冲突
	sym->st_other=0;
	if(lb->externed)
		sym->st_shndx=STN_UNDEF;
	else
		sym->st_shndx=getSegIndex(lb->segName);
	addSym(lb->lbName,sym);
}

void Elf_file::addSym(string st_name,Elf32_Sym*s)
{
	Elf32_Sym*sym=symTab[st_name]=new Elf32_Sym();
	if(st_name=="")
	{
		sym->st_name=0;
		sym->st_value=0;
		sym->st_size=0;
		sym->st_info=0;
		sym->st_other=0;
		sym->st_shndx=0;
	}
	else
	{
		sym->st_name=0;
		sym->st_value=s->st_value;
		sym->st_size=s->st_size;
		sym->st_info=s->st_info;
		sym->st_other=s->st_other;
		sym->st_shndx=s->st_shndx;
	}
	symNames.push_back(st_name);
}


RelInfo* Elf_file::addRel(string seg,int addr,string lb,int type)
{
	RelInfo*rel=new RelInfo(seg,addr,lb,type);
	relTab.push_back(rel);
	return rel;
}
void Elf_file::writeElf()
{
	fclose(fout);
	fout=fopen((finName+".o").c_str(),"w");//输出文件
	assmObj();//组装文件
  fwrite(&ehdr,ehdr.e_ehsize,1,fout);//输出elf文件头
  //输出.text
  fclose(fin);
  fin=fopen((finName+".t").c_str(),"r");//临时输出文件，供代码段使用
  char buffer[1024]={0};
  int f=-1;
  while(f!=0)
  {
    f=fread(buffer,1,1024,fin);
    fwrite(buffer,1,f,fout);
  }
  //cout<<"---------------------输出指令的长度="<<inLen<<"  计算指令长度="<<shdrTab[".text"]->sh_size<<endl;
  padSeg(".text",".data");
  table.write();//.data
  padSeg(".data",".bss");
  //.bss不用输出，对齐即可
  //.shstrtab，段表，符号表，.strtab，.rel.text，.rel.data
  obj.writeElfTail();//输出剩下的尾部
}

void Elf_file::padSeg(string first,string second)//填充段间的空隙
{
	char pad[1]={0};
	int padNum=shdrTab[second]->sh_offset-(shdrTab[first]->sh_offset+shdrTab[first]->sh_size);
	while(padNum--)
	{
		fwrite(pad,1,1,fout);//填充
	}
}


void Elf_file::assmObj()
{
	//-----elf头
	int*p_id=(int*)ehdr.e_ident;
	*p_id=0x464c457f;p_id++;*p_id=0x010101;p_id++;*p_id=0;p_id++;*p_id=0;
	ehdr.e_type=ET_REL;
	ehdr.e_machine=EM_386;
	ehdr.e_version=EV_CURRENT;
	ehdr.e_entry=0;
	ehdr.e_phoff=0;
	ehdr.e_flags=0;
	ehdr.e_ehsize=52;
	ehdr.e_phentsize=0;
	ehdr.e_phnum=0;
	ehdr.e_shentsize=40;
	ehdr.e_shnum=9;
	ehdr.e_shstrndx=4;
	//-----填充.shstrtab数据	
	int curOff=52+dataLen;//header+(.text+pad+.data+pad)数据偏移，.shstrtab偏移
	shstrtabSize=51;//".rel.text".length()+".rel.data".length()+".bss".length()
											//".shstrtab".length()+".symtab".length()+".strtab".length()+5;//段表字符串表大小
	char*str=shstrtab=new char[shstrtabSize];
	int index=0;
	//段表串名与索引映射
	hash_map<string,int,string_hash> shstrIndex;
	shstrIndex[".rel.text"]=index;strcpy(str+index,".rel.text");
	shstrIndex[".text"]=index+4;index+=10;
	shstrIndex[""]=index-1;
	shstrIndex[".rel.data"]=index;strcpy(str+index,".rel.data");
	shstrIndex[".data"]=index+4;index+=10;
	shstrIndex[".bss"]=index;strcpy(str+index,".bss");index+=5;
	shstrIndex[".shstrtab"]=index;strcpy(str+index,".shstrtab");index+=10;
	shstrIndex[".symtab"]=index;strcpy(str+index,".symtab");index+=8;
	shstrIndex[".strtab"]=index;strcpy(str+index,".strtab");index+=8;
	//for(int i=0;i<shstrtabSize;++i)printf("%c",str[i]);printf("\n");
	//for(int i=0;i<shstrtabSize;++i)printf("%d|",str[i]);printf("\n");
	//string segNames[]={"",".rel.text",".rel.data",".bss",".shstrtab",".symtab",".strtab"};
	//添加.shstrtab
	addShdr(".shstrtab",SHT_STRTAB,0,0,curOff,shstrtabSize,SHN_UNDEF,0,1,0);//.shstrtab
	//-----定位段表
	curOff+=shstrtabSize;
	ehdr.e_shoff=curOff;
	//cout<<"--------------------------段表偏移"<<curOff<<endl;
	//-----添加符号表	
	curOff+=9*40;//8个段+空段，段表字符串表偏移，符号表表偏移
	//.symtab,sh_link 代表.strtab索引，默认在.symtab之后,sh_info不能确定
	addShdr(".symtab",SHT_SYMTAB,0,0,curOff,symNames.size()*16,0,0,1,16);
	shdrTab[".symtab"]->sh_link=getSegIndex(".symtab")+1;//.strtab默认在.symtab之后
	//-----添加.strtab
	strtabSize=0;//字符串表大小
	for(int i=0;i<symNames.size();++i)//遍历所有符号
	{
		strtabSize+=symNames[i].length()+1;
	}
	curOff+=symNames.size()*16;//.strtab偏移
	addShdr(".strtab",SHT_STRTAB,0,0,curOff,strtabSize,SHN_UNDEF,0,1,0);//.strtab
	//填充strtab数据
	str=strtab=new char[strtabSize];
	index=0;
	//串表与符号表名字更新
	for(int i=0;i<symNames.size();++i)
	{
		symTab[symNames[i]]->st_name=index;
		strcpy(str+index,symNames[i].c_str());
		index+=(symNames[i].length()+1);
	}
	//for(int i=0;i<strtabSize;++i)printf("%c",str[i]);printf("\n");
	//for(int i=0;i<strtabSize;++i)printf("%d|",str[i]);printf("\n");
	//处理重定位表
	for(int i=0;i<relTab.size();i++)
	{
		Elf32_Rel*rel=new Elf32_Rel();
		rel->r_offset=relTab[i]->offset;
		rel->r_info=ELF32_R_INFO((Elf32_Word)getSymIndex(relTab[i]->lbName),relTab[i]->type);
		if(relTab[i]->tarSeg==".text")
			relTextTab.push_back(rel);
		else if(relTab[i]->tarSeg==".data")
			relDataTab.push_back(rel);
	}
	//-----添加.rel.text
	curOff+=strtabSize;
	addShdr(".rel.text",SHT_REL,0,0,curOff,relTextTab.size()*8,getSegIndex(".symtab"),getSegIndex(".text"),1,8);//.rel.text
	//-----添加.rel.data
	curOff+=relTextTab.size()*8;
	addShdr(".rel.data",SHT_REL,0,0,curOff,relDataTab.size()*8,getSegIndex(".symtab"),getSegIndex(".data"),1,8);//.rel.data
	//更新段表name
	for(int i=0;i<shdrNames.size();++i)
	{
		int index=shstrIndex[shdrNames[i]];
		shdrTab[shdrNames[i]]->sh_name=index;
	}
}

void Elf_file::writeElfTail()
{
	//-----输出
	fwrite(shstrtab,shstrtabSize,1,fout);//.shstrtab
	for(int i=0;i<shdrNames.size();++i)//段表
	{
		Elf32_Shdr*sh=shdrTab[shdrNames[i]];
		fwrite(sh,ehdr.e_shentsize,1,fout);
	}
	for(int i=0;i<symNames.size();++i)//符号表
	{
		Elf32_Sym*sym=symTab[symNames[i]];
		fwrite(sym,sizeof(Elf32_Sym),1,fout);
	}
	fwrite(strtab,strtabSize,1,fout);//.strtab
	for(int i=0;i<relTextTab.size();++i)//.rel.text
	{
		Elf32_Rel*rel=relTextTab[i];
		fwrite(rel,sizeof(Elf32_Rel),1,fout);
		delete rel;
	}
	for(int i=0;i<relDataTab.size();++i)//.rel.data
	{
		Elf32_Rel*rel=relDataTab[i];
		fwrite(rel,sizeof(Elf32_Rel),1,fout);
		delete rel;
	}
}

Elf_file::~Elf_file()
{
	//清空段表
	for(hash_map<string, Elf32_Shdr*,string_hash>::iterator i=shdrTab.begin();i!=shdrTab.end();++i)
	{
		delete i->second;
	}
	shdrTab.clear();
	shdrNames.clear();
	//清空符号表
	for(hash_map<string,Elf32_Sym*,string_hash>::iterator i=symTab.begin();i!=symTab.end();++i)
	{
		delete i->second;
	}
	symTab.clear();
	//清空重定位表
	for(vector<RelInfo*>::iterator i=relTab.begin();i!=relTab.end();++i)
	{
		delete *i;
	}
	relTab.clear();
	//清空缓冲区
	if(shstrtab)
		delete shstrtab;
	if(strtab)
		delete strtab;
}


void Elf_file:: printAll()
{
	if(!showAss)
		return;
	cout<<"------------段信息------------\n";
	for(hash_map<string, Elf32_Shdr*,string_hash>::iterator i=shdrTab.begin();i!=shdrTab.end();++i)
	{
		if(i->first=="")
			continue;
		cout<<i->first<<":"<<i->second->sh_size<<endl;
	}
	cout<<"------------符号信息------------\n";
	for(hash_map<string,Elf32_Sym*,string_hash>::iterator i=symTab.begin();i!=symTab.end();++i)
	{
		if(i->first=="")
			continue;
		cout<<i->first<<":";
		if(i->second->st_shndx==0)
			cout<<"外部";
		if(ELF32_ST_BIND(i->second->st_info)==STB_GLOBAL)
			cout<<"全局";
		else if(ELF32_ST_BIND(i->second->st_info)==STB_LOCAL)
			cout<<"局部";
		cout<<endl;
	}
	cout<<"------------重定位信息------------\n";
	for(vector<RelInfo*>::iterator i=relTab.begin();i!=relTab.end();++i)
	{
		cout<<(*i)->tarSeg<<":"<<(*i)->offset<<"<-"<<(*i)->lbName<<endl;
	}
}
