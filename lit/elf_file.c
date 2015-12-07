#include"elf_file.h"
#include<stdio.h>
#include<string.h>
extern bool showLink;
RelItem::RelItem(string sname,Elf32_Rel*r,string rname)
{
	segName=sname;
	rel=r;
	relName=rname;
}

RelItem::~RelItem()
{
	delete rel;
}

void Elf_file::getData(char*buf,Elf32_Off offset,Elf32_Word size)
{
	FILE*fp=fopen(elf_dir,"rb");
	rewind(fp);
	fseek(fp,offset,0);//读取位置
	fread(buf,size,1,fp);//读取数据
	fclose(fp);
}

Elf_file::Elf_file()
{
	shstrtab=NULL;
	strtab=NULL;
	elf_dir=NULL;
}

void Elf_file::readElf(const char *dir)
{
	string d=dir;
	elf_dir=new char[d.length()+1];
	strcpy(elf_dir,dir);
	FILE*fp=fopen(dir,"rb");
	rewind(fp);
	fread(&ehdr,sizeof(Elf32_Ehdr),1,fp);//读取文件头

	if(ehdr.e_type==ET_EXEC)//可执行文件拥有程序头表
	{
		fseek(fp,ehdr.e_phoff,0);//程序头表位置
		for(int i=0;i<ehdr.e_phnum;++i)//读取程序头表
		{
			Elf32_Phdr*phdr=new Elf32_Phdr();
			fread(phdr,ehdr.e_phentsize,1,fp);//读取程序头
			phdrTab.push_back(phdr);//加入程序头表
		}
	}

	fseek(fp,ehdr.e_shoff+ehdr.e_shentsize*ehdr.e_shstrndx,0);//段表字符串表位置
	Elf32_Shdr shstrTab;
	fread(&shstrTab,ehdr.e_shentsize,1,fp);//读取段表字符串表项
	char*shstrTabData=new char[shstrTab.sh_size];
	fseek(fp,shstrTab.sh_offset,0);//转移到段表字符串表内容
	fread(shstrTabData,shstrTab.sh_size,1,fp);//读取段表字符串表
	//for(int i=0;i<shstrTab.sh_size;i++)printf("%c",shstrTabData[i]);printf("\t--shstrTab\n");

	fseek(fp,ehdr.e_shoff,0);//段表位置
	for(int i=0;i<ehdr.e_shnum;++i)//读取段表
	{
		Elf32_Shdr*shdr=new Elf32_Shdr();
		fread(shdr,ehdr.e_shentsize,1,fp);//读取段表项[非空]
		string name(shstrTabData+shdr->sh_name);
		shdrNames.push_back(name);//记录段表名位置
		if(name.empty())
			delete shdr;//删除空段表项
		else
		{
			//printf("%s\t\t%08x\n",shstrTabData+shdr->sh_name,shdr->sh_addralign);
			shdrTab[name]=shdr;//加入段表
		}
	}
	delete []shstrTabData;//清空段表字符串表

	Elf32_Shdr *strTab=shdrTab[".strtab"];//字符串表信息
	char*strTabData=new char[strTab->sh_size];
	fseek(fp,strTab->sh_offset,0);//转移到字符串表内容
	fread(strTabData,strTab->sh_size,1,fp);//读取字符串表
	//for(int i=0;i<strTab->sh_size;i++)printf("%c",strTabData[i]);printf("\t--strTab\n");

	Elf32_Shdr *sh_symTab=shdrTab[".symtab"];//符号表信息
	fseek(fp,sh_symTab->sh_offset,0);//转移到符号表内容
	int symNum=sh_symTab->sh_size/16;//符号个数[非空],16最好用2**sh_symTab->sh_entsize代替
	vector<Elf32_Sym*>symList;//按照序列记录符号表所有信息，方便重定位符号查询
	for(int i=0;i<symNum;++i)//读取符号
	{
		Elf32_Sym*sym=new Elf32_Sym();
		fread(sym,16,1,fp);//读取符号项[非空]
		symList.push_back(sym);//添加到符号序列
		string name(strTabData+sym->st_name);
		if(name.empty())//无名符号，对于链接没有意义,按照链接器设计需要记录全局和局部符号，避免名字冲突
			delete sym;//删除空符号项
		else
		{
			//if(ELF32_ST_BIND(sym->st_info)==STB_GLOBAL)
				//printf("%s\t\t%d\n",strTabData+sym->st_name,sym->st_shndx);
			symTab[name]=sym;//加入符号表
		}
	}
	if(showLink)
		printf("----------%s重定位数据:----------\n",elf_dir);
	for(hash_map<string, Elf32_Shdr*,string_hash>::iterator i=shdrTab.begin();i!=shdrTab.end();++i)//所有段的重定位项整合
	{
		if(i->first.find(".rel")==0)//是重定位段
		{
			Elf32_Shdr *sh_relTab=shdrTab[i->first];//重定位表信息
			fseek(fp,sh_relTab->sh_offset,0);//转移到重定位表内容
			int relNum=sh_relTab->sh_size/8;//重定位项数
			for(int j=0;j<relNum;++j)
			{
				Elf32_Rel*rel=new Elf32_Rel();
				fread(rel,8,1,fp);//读取重定位项
				string name(strTabData+symList[ELF32_R_SYM(rel->r_info)]->st_name);//获得重定位符号名字
				//使用shdrNames[sh_relTab->sh_info]访问目标段更标准
				relTab.push_back(new RelItem(i->first.substr(4),rel,name));//添加重定位项
				if(showLink)
					printf("%s\t%08x\t%s\n",i->first.substr(4).c_str(),rel->r_offset,name.c_str());
			}
		}
	}
	delete []strTabData;//清空字符串表

	fclose(fp);
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
		if(shdrNames[i]==symName)//找到符号
			break;
		++index;
	}
	return index;
}

void Elf_file::addPhdr(Elf32_Word type,Elf32_Off off,Elf32_Addr vaddr,Elf32_Word filesz,
		Elf32_Word memsz,Elf32_Word flags,Elf32_Word align)
{
	Elf32_Phdr*ph=new Elf32_Phdr();
	ph->p_type=type;
	ph->p_offset=off;
	ph->p_vaddr=ph->p_paddr=vaddr;
	ph->p_filesz=filesz;
	ph->p_memsz=memsz;
	ph->p_flags=flags;
	ph->p_align=align;
	phdrTab.push_back(ph);
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

/*
	dir:输出目录
	flag:1-第一次写，文件头+PHT；2-第二次写，段表字符串表+段表+符号表+字符串表；
*/
void Elf_file::writeElf(const char*dir,int flag)
{
	if(flag==1)
	{
		FILE*fp=fopen(dir,"w+");
		fwrite(&ehdr,ehdr.e_ehsize,1,fp);//elf文件头
		if(!phdrTab.empty())//程序头表
		{
			for(int i=0;i<phdrTab.size();++i)
				fwrite(phdrTab[i],ehdr.e_phentsize,1,fp);
		}
		fclose(fp);
	}
	else if(flag==2)
	{
		FILE*fp=fopen(dir,"a+");
		fwrite(shstrtab,shstrtabSize,1,fp);//.shstrtab
		for(int i=0;i<shdrNames.size();++i)//段表
		{
			Elf32_Shdr*sh=shdrTab[shdrNames[i]];
			fwrite(sh,ehdr.e_shentsize,1,fp);
		}
		for(int i=0;i<symNames.size();++i)//符号表
		{
			Elf32_Sym*sym=symTab[symNames[i]];
			fwrite(sym,sizeof(Elf32_Sym),1,fp);
		}
		fwrite(strtab,strtabSize,1,fp);//.strtab
		fclose(fp);
	}
}

Elf_file::~Elf_file()
{
	//清空程序头表
	for(vector<Elf32_Phdr*>::iterator i=phdrTab.begin();i!=phdrTab.end();++i)
	{
		delete *i;
	}
	phdrTab.clear();
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
	for(vector<RelItem*>::iterator i=relTab.begin();i!=relTab.end();++i)
	{
		delete *i;
	}
	relTab.clear();
	//清空临时存储数据
	if(shstrtab!=NULL)delete[] shstrtab;
	if(strtab!=NULL)delete[] strtab;
	if(elf_dir)
		delete elf_dir;
}
