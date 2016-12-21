#include"linker.h"
#include<stdio.h>
#include<string.h>
extern bool showLink;
Block::Block(char*d,unsigned int off,unsigned int s)
{
	data=d;
	offset=off;
	size=s;
}

Block::~Block()
{
	delete [] data;
}

SegList::~SegList()
{
	ownerList.clear();
	for(int i=0;i<blocks.size();++i)
	{
		delete blocks[i];
	}
	blocks.clear();
}

/*
	name:段名
	off:文件偏移地址
	base:加载基址，修改后提供给其他段
*/
void SegList::allocAddr(string name,unsigned int& base,unsigned int& off)
{

	begin=off;//记录对齐前偏移
	//虚拟地址对齐，让所有的段按照4k字节对齐
	if(name!=".bss")//.bss段直接紧跟上一个段，一般是.data,因此定义处理段时需要将.data和.bss放在最后
		base+=(MEM_ALIGN-base%MEM_ALIGN)%MEM_ALIGN;
	//偏移地址对齐，让一般段按照4字节对齐，文本段按照16字节对齐
	int align=DISC_ALIGN;
	if(name==".text")
		align=16;
	off+=(align-off%align)%align;
	//使虚址和偏移按照4k模同余
	base=base-base%MEM_ALIGN+off%MEM_ALIGN;
	//累加地址和偏移
	baseAddr=base;
	offset=off;
	size=0;
	for(int i=0;i<ownerList.size();++i)
	{
		size+=(DISC_ALIGN-size%DISC_ALIGN)%DISC_ALIGN;//对齐每个小段，按照4字节
		Elf32_Shdr*seg=ownerList[i]->shdrTab[name];
		//读取需要合并段的数据
		if(name!=".bss")
		{
			char* buf=new char[seg->sh_size];//申请数据缓存
			ownerList[i]->getData(buf,seg->sh_offset,seg->sh_size);//读取数据
			blocks.push_back(new Block(buf,size,seg->sh_size));//记录数据，对于.bss段数据是空的，不能重定位！没有common段！！！
		}
		//修改每个文件中对应段的addr
		seg->sh_addr=base+size;//修改每个文件的段虚拟，为了方便计算符号或者重定位的虚址，不需要保存合并后文件偏移
		size+=seg->sh_size;//累加段大小
	}
	base+=size;//累加基址
	if(name!=".bss")//.bss段不修改偏移
		off+=size;
	
}

/*
	relAddr:重定位虚拟地址
	type:重定位类型
	symAddr:重定位符号的虚拟地址
*/
void SegList::relocAddr(unsigned int relAddr,unsigned char type,unsigned int symAddr)
{
	unsigned int relOffset=relAddr-baseAddr;//同类合并段的数据偏移
	//查找修正地址所在位置
	Block*b=NULL;
	for(int i=0;i<blocks.size();++i)
	{
		if(blocks[i]->offset<=relOffset&&blocks[i]->offset+blocks[i]->size>relOffset)
		{
			b=blocks[i];
			break;//找到了
		}
	}
	//处理字节为b->data[relOffset-b->offset]
	int *pAddr=(int*)(b->data+relOffset-b->offset);
	if(type==R_386_32)//绝对地址修正
	{
		if(showLink)
			printf("绝对地址修正：原地址=%08x\t",*pAddr);
		*pAddr=symAddr;
		if(showLink)
			printf("修正后地址=%08x\n",*pAddr);
	}
	else if(type==R_386_PC32)//相对地之修正
	{
		if(showLink)
			printf("相对地址修正：原地址=%08x\t",*pAddr);
		*pAddr=symAddr-relAddr+*pAddr;
		if(showLink)
			printf("修正后地址=%08x\n",*pAddr);
	}
}

Linker::Linker()
{
	segNames.push_back(".text");
	segNames.push_back(".data");
	segNames.push_back(".bss");//.bss段有尾端对齐功能，不能删除
	for(int i=0;i<segNames.size();++i)
		segLists[segNames[i]]=new SegList();
}

void Linker::addElf(const char*dir)
{
	Elf_file*elf=new Elf_file();
	elf->readElf(dir);//读入目标文件，构造elf文件对象
	elfs.push_back(elf);//添加目标文件对象
}

void Linker::collectInfo()
{
	for(int i=0;i<elfs.size();++i)//扫描输入文件
	{
		Elf_file*elf=elfs[i];
		//记录段表信息
		for(int i=0;i<segNames.size();++i)
			if(elf->shdrTab.find(segNames[i])!=elf->shdrTab.end())
				segLists[segNames[i]]->ownerList.push_back(elf);
		//记录符号引用信息
		for(hash_map<string,Elf32_Sym*,string_hash>::iterator symIt=elf->symTab.begin();
			symIt!=elf->symTab.end();++symIt)//所搜该文件的所有有用符号
		{
			//if(ELF32_ST_BIND(symIt->second->st_info)==STB_GLOBAL)//不是只考虑全局符号[但是按照汇编器设计，局部符号避免名字冲突]
			{
				SymLink*symLink=new SymLink();
				symLink->name=symIt->first;//记录名字
				if(symIt->second->st_shndx==STN_UNDEF)//引用符号
				{
					symLink->recv=elf;//记录引用文件
					symLink->prov=NULL;//标记未定义
					symLinks.push_back(symLink);
					//printf("%s---未定义\n",symLink->name.c_str());
				}
				else
				{
					symLink->prov=elf;//记录定义文件
					symLink->recv=NULL;//标示该定义符号未被任何文件引用
					symDef.push_back(symLink);
					//printf("%s---定义\n",symLink->name.c_str());
				}
			}
		}
	}
}

bool Linker::symValid()
{
	bool flag=true;
	startOwner=NULL;
	for(int i=0;i<symDef.size();++i)//遍历定义的符号,寻找重定义信息，和入口信息
	{
		if(ELF32_ST_BIND(symDef[i]->prov->symTab[symDef[i]->name]->st_info)!=STB_GLOBAL)//只考虑全局符号
			continue;
		if(symDef[i]->name==START)//记录程序入口文件
			startOwner=symDef[i]->prov;
		for(int j=i+1;j<symDef.size();++j)//遍历后边定义的符号
		{
			if(ELF32_ST_BIND(symDef[j]->prov->symTab[symDef[j]->name]->st_info)!=STB_GLOBAL)//只考虑全局符号
				continue;
			//printf("%s---VS---%s\n",symDef[i]->name.c_str(),symDef[j]->name.c_str());
			if(symDef[i]->name==symDef[j]->name//同名符号
				//&&symDef[i]->prov->symTab[symDef[i]->name]->st_info
				//==symDef[j]->prov->symTab[symDef[j]->name]->st_info
			)//类型相同[!!!不允许函数名符号和变量名符号相同,在汇编时无法区分这种差别!!!]
			{
				//unsigned char info=symDef[i]->prov->symTab[symDef[i]->name]->st_info;
				//string type;
				//if(ELF32_ST_TYPE(info)==STT_OBJECT)type="变量";
				//else if(ELF32_ST_TYPE(info)==STT_FUNC)type="函数";
				//else type="符号";
				printf("符号名%s在文件%s和文件%s中发生链接冲突。\n",symDef[i]->name.c_str()
				,symDef[i]->prov->elf_dir,symDef[j]->prov->elf_dir);
				flag=false;
			}
		}
	}
	if(startOwner==NULL)
	{
		printf("链接器找不到程序入口%s。\n",START);
		flag=false;
	}
	for(int i=0;i<symLinks.size();++i)//遍历未定义符号
	{
		for(int j=0;j<symDef.size();++j)//遍历定义的符号
		{
			if(ELF32_ST_BIND(symDef[j]->prov->symTab[symDef[j]->name]->st_info)!=STB_GLOBAL)//只考虑全局符号
				continue;
			//printf("%s---VS---%s,%d<->%d\n",symLinks[i]->name.c_str(),symDef[j]->name.c_str()
				//,symLinks[i]->recv->symTab[symLinks[i]->name]->st_info
				//,symDef[j]->prov->symTab[symDef[j]->name]->st_info);
			if(symLinks[i]->name==symDef[j]->name//同名符号
				//&&symLinks[i]->recv->symTab[symLinks[i]->name]->st_info
				//==symDef[j]->prov->symTab[symDef[j]->name]->st_info
			)//类型相同[!!!不允许函数名符号和变量名符号相同,在汇编时无法区分这种差别!!!]
			{
				symLinks[i]->prov=symDef[j]->prov;//记录符号定义的文件信息
				symDef[j]->recv=symDef[j]->prov;//该赋值没有意义，只是保证recv不为NULL
				//printf("解析%s\n",symLinks[i]->name.c_str());
			}
		}
		if(symLinks[i]->prov==NULL)//未定义
		{
			unsigned char info=symLinks[i]->recv->symTab[symLinks[i]->name]->st_info;
			string type;
			if(ELF32_ST_TYPE(info)==STT_OBJECT)type="变量";
			else if(ELF32_ST_TYPE(info)==STT_FUNC)type="函数";
			else type="符号";
			printf("文件%s的%s名%s未定义。\n",symLinks[i]->recv->elf_dir
				,type.c_str(),symLinks[i]->name.c_str());
			if(flag)
				flag=false;
		}
	}
	return flag;
}

void Linker::allocAddr()
{
	unsigned int curAddr=BASE_ADDR;//当前加载基址
	unsigned int curOff=52+32*segNames.size();//默认文件偏移,PHT保留.bss段
	if(showLink)
		printf("----------地址分配----------\n");
	for(int i=0;i<segNames.size();++i)//按照类型分配地址，不紧邻.data与.bss段
	{
		//unsigned int oldOff=curOff;//记录分配前的文件偏移
		segLists[segNames[i]]->allocAddr(segNames[i],curAddr,curOff);//自动分配
		//if(segNames[i]==".bss")//不做处理，在函数内部对off不累加
		//	segLists[segNames[i]]->offset=segLists[segNames[i]]->end=curOff=oldOff;//撤销.bss对curOff的修改
		if(showLink)
			printf("%s\taddr=%08x\toff=%08x\tsize=%08x(%d)\n",segNames[i].c_str(),
			segLists[segNames[i]]->baseAddr,segLists[segNames[i]]->offset
			,segLists[segNames[i]]->size,segLists[segNames[i]]->size);
	}
}

void Linker::symParser()
{
	//扫描所有定义符号，原地计算虚拟地址
	if(showLink)
		printf("----------定义符号解析----------\n");
	for(int i=0;i<symDef.size();++i)
	{
		Elf32_Sym*sym=symDef[i]->prov->symTab[symDef[i]->name];//定义的符号信息
		string segName=symDef[i]->prov->shdrNames[sym->st_shndx];//段名
		//if(sym->st_shndx==SHN_COMMON)//bss,该链接器定义不允许出现COMMON
			//segName=".bss";
		sym->st_value=sym->st_value+//偏移
			symDef[i]->prov->shdrTab[segName]->sh_addr;//段基址
		if(showLink)
			printf("%s\t%08x\t%s\n",symDef[i]->name.c_str(),sym->st_value,symDef[i]->prov->elf_dir);
	}
	//扫描所有符号引用，绑定虚拟地址
	if(showLink)
		printf("----------未定义符号解析----------\n");
	for(int i=0;i<symLinks.size();++i)
	{
		Elf32_Sym*provsym=symLinks[i]->prov->symTab[symLinks[i]->name];//被引用的符号信息
		Elf32_Sym*recvsym=symLinks[i]->recv->symTab[symLinks[i]->name];//被引用的符号信息
		recvsym->st_value=provsym->st_value;//被引用符号已经解析了
		if(showLink)
			printf("%s\t%08x\t%s\n",symLinks[i]->name.c_str(),recvsym->st_value,symLinks[i]->recv->elf_dir);
	}
}

void Linker::relocate()
{
	//重定位项符号必然在符号表中，且地址已经解析完毕
	if(showLink)
		printf("--------------重定位----------------\n");
	for(int i=0;i<elfs.size();++i)
	{
		vector<RelItem*>tab=elfs[i]->relTab;//得到重定位表
		for(int j=0;j<tab.size();++j)//遍历重定位项
		{
			Elf32_Sym* sym=elfs[i]->symTab[tab[j]->relName];//重定位符号信息
			unsigned int symAddr=sym->st_value;//解析后的符号段偏移为虚拟地址
			unsigned int relAddr=elfs[i]->shdrTab[tab[j]->segName]->sh_addr+tab[j]->rel->r_offset;//重定位地址
			//重定位操作
			if(showLink)
				printf("%s\trelAddr=%08x\tsymAddr=%08x\n",tab[j]->relName.c_str(),relAddr,symAddr);
			segLists[tab[j]->segName]->relocAddr(relAddr,ELF32_R_TYPE(tab[j]->rel->r_info),symAddr);
		}
	}
}

void Linker::assemExe()
{
	//printf("----------------生成elf文件------------------\n");
	//初始化文件头
	int*p_id=(int*)exe.ehdr.e_ident;
	*p_id=0x464c457f;p_id++;*p_id=0x010101;p_id++;*p_id=0;p_id++;*p_id=0;
	//for(int i=0;i<EI_NIDENT;++i)printf("%02x ",exe.ehdr.e_ident[i]);printf("\n");
	exe.ehdr.e_type=ET_EXEC;
	exe.ehdr.e_machine=EM_386;
	exe.ehdr.e_version=EV_CURRENT;
	exe.ehdr.e_flags=0;
	exe.ehdr.e_ehsize=52;
	//数据位置指针
	unsigned int curOff=52+32*segNames.size();//文件头52B+程序头表项32*个数
	//printf("Elf header & Pht(32N):\tbase=00000000\tsize=%08x\n",curOff);
	exe.addShdr("",0,0,0,0,0,0,0,0,0);//空段表项
	int shstrtabSize=26;//".shstrtab".length()+".symtab".length()+".strtab".length()+3;//段表字符串表大小
	for(int i=0;i<segNames.size();++i)
	{
		string name=segNames[i];
		shstrtabSize+=name.length()+1;//考虑结束符'\0'
		//生成程序头表
		Elf32_Word flags=PF_W|PF_R;//读写
		Elf32_Word filesz=segLists[name]->size;//占用磁盘大小
		if(name==".text")flags=PF_X|PF_R;//.text段可读可执行
		if(name==".bss")filesz=0;//.bss段不占磁盘空间
		exe.addPhdr(PT_LOAD,segLists[name]->offset,segLists[name]->baseAddr,
		filesz,segLists[name]->size,flags,MEM_ALIGN);//添加程序头表项
		//计算有效数据段的大小和偏移,最后一个决定
		//printf("%s:\tbase=%08x\tsize=%08x\n",name.c_str(),curOff,segLists[name]->size);
		curOff=segLists[name]->offset;//修正当前偏移，循环结束后保留的是.bss的基址
		
		//生成段表项
		Elf32_Word sh_type=SHT_PROGBITS;
		Elf32_Word sh_flags=SHF_ALLOC|SHF_WRITE;
		Elf32_Word sh_align=4;//4B
		if(name==".bss")sh_type=SHT_NOBITS;
		if(name==".text")
		{
			sh_flags=SHF_ALLOC|SHF_EXECINSTR;
			sh_align=16;//16B
		}
		exe.addShdr(name,sh_type,sh_flags,segLists[name]->baseAddr,segLists[name]->offset,
			segLists[name]->size,SHN_UNDEF,0,sh_align,0);//添加一个段表项，暂时按照4字节对齐
	}
	exe.ehdr.e_phoff=52;
	exe.ehdr.e_phentsize=32;
	exe.ehdr.e_phnum=segNames.size();
	//填充shstrtab数据
	char*str=exe.shstrtab=new char[shstrtabSize];
	exe.shstrtabSize=shstrtabSize;
	int index=0;
	//段表串名与索引映射
	hash_map<string,int,string_hash> shstrIndex;
	shstrIndex[".shstrtab"]=index;strcpy(str+index,".shstrtab");index+=10;
	shstrIndex[".symtab"]=index;strcpy(str+index,".symtab");index+=8;
	shstrIndex[".strtab"]=index;strcpy(str+index,".strtab");index+=8;
	shstrIndex[""]=index-1;
	for(int i=0;i<segNames.size();++i)
	{
		shstrIndex[segNames[i]]=index;
		strcpy(str+index,segNames[i].c_str());
		index+=segNames[i].length()+1;
	}
	//for(int i=0;i<shstrtabSize;++i)printf("%c",str[i]);printf("\n");
	//for(int i=0;i<shstrtabSize;++i)printf("%d|",str[i]);printf("\n");
	//生成.shstrtab
	//printf(".shstrtab:\tbase=%08x\tsize=%08x\n",curOff,shstrtabSize);
	exe.addShdr(".shstrtab",SHT_STRTAB,0,0,curOff,shstrtabSize,SHN_UNDEF,0,1,0);//.shstrtab
	exe.ehdr.e_shstrndx=exe.getSegIndex(".shstrtab");//1+segNames.size();//空表项+所有段数
	curOff+=shstrtabSize;//段表偏移
	exe.ehdr.e_shoff=curOff;
	exe.ehdr.e_shentsize=40;
	exe.ehdr.e_shnum=4+segNames.size();//段表数
	//printf("Sht(40N):\tbase=%08x\tsize=%08x\n",curOff,40*exe.ehdr.e_shnum);
	//生成符号表项
	curOff+=40*(4+segNames.size());//符号表偏移
	//printf(".symtab:\tbase=%08x\tsize=%08x\n",curOff,(1+symDef.size())*16);
	//符号表位置=（空表项+所有段数+段表字符串表项+符号表项+字符串表项）*40
	//.symtab,sh_link 代表.strtab索引，默认在.symtab之后,sh_info不能确定
	exe.addShdr(".symtab",SHT_SYMTAB,0,0,curOff,(1+symDef.size())*16,0,0,1,16);
	exe.shdrTab[".symtab"]->sh_link=exe.getSegIndex(".symtab")+1;//。strtab默认在.symtab之后
	int strtabSize=0;//字符串表大小
	exe.addSym("",NULL);//空符号表项
	for(int i=0;i<symDef.size();++i)//遍历所有符号
	{
		string name=symDef[i]->name;
		strtabSize+=name.length()+1;
		Elf32_Sym*sym=symDef[i]->prov->symTab[name];
		sym->st_shndx=exe.getSegIndex(symDef[i]->prov->shdrNames[sym->st_shndx]);//重定位后可以修改了
		exe.addSym(name,sym);
	}
	//记录程序入口
	exe.ehdr.e_entry=exe.symTab[START]->st_value;//程序入口地址
	curOff+=(1+symDef.size())*16;//.strtab偏移
	exe.addShdr(".strtab",SHT_STRTAB,0,0,curOff,strtabSize,SHN_UNDEF,0,1,0);//.strtab
	//printf(".strtab:\tbase=%08x\tsize=%08x\n",curOff,strtabSize);
	//填充strtab数据
	str=exe.strtab=new char[strtabSize];
	exe.strtabSize=strtabSize;
	index=0;
	//串表与索引映射
	hash_map<string,int,string_hash> strIndex;
	strIndex[""]=strtabSize-1;
	for(int i=0;i<symDef.size();++i)
	{
		strIndex[symDef[i]->name]=index;
		strcpy(str+index,symDef[i]->name.c_str());
		index+=symDef[i]->name.length()+1;
	}
	//for(int i=0;i<strtabSize;++i)printf("%c",str[i]);printf("\n");
	//for(int i=0;i<strtabSize;++i)printf("%d|",str[i]);printf("\n");
	//更新符号表name
	for(hash_map<string,Elf32_Sym*,string_hash>::iterator i=exe.symTab.begin();i!=exe.symTab.end();++i)
	{
		i->second->st_name=strIndex[i->first];
		//printf("%s\t%08x\t%s\n",i->first.c_str(),strIndex[i->first],str+strIndex[i->first]);
	}
	//更新段表name
	for(hash_map<string, Elf32_Shdr*,string_hash>::iterator i=exe.shdrTab.begin();i!=exe.shdrTab.end();++i)
	{
		i->second->sh_name=shstrIndex[i->first];
	}
}

void Linker::exportElf(const char*dir)
{
	exe.writeElf(dir,1);//输出链接后的elf前半段
	//输出重要段数据
	FILE*fp=fopen(dir,"a+");
	char pad[1]={0};
	for(int i=0;i<segNames.size();++i)
	{
		SegList*sl=segLists[segNames[i]];
		int padnum=sl->offset-sl->begin;
		while(padnum--)
			fwrite(pad,1,1,fp);//填充
		//输出数据
		if(segNames[i]!=".bss")
		{
			Block*old=NULL;
			char instPad[1]={(char)0x90};
			for(int j=0;j<sl->blocks.size();++j)
			{
				Block*b=sl->blocks[j];
				if(old!=NULL)//填充小段内的空隙
				{
					padnum=b->offset-(old->offset+old->size);
					while(padnum--)
						fwrite(instPad,1,1,fp);//填充
				}
				old=b;
				fwrite(b->data,b->size,1,fp);
			}
		}		
	}
	fclose(fp);
	exe.writeElf(dir,2);//输出链接后的elf后半段
}

bool Linker::link(const char*dir)
{
	collectInfo();//搜集段/符号信息
	if(!symValid())//符号引用验证
		return false;
	allocAddr();//分配地址空间
	symParser();//符号地址解析
	relocate();//重定位
	assemExe();//组装文件exe
	exportElf(dir);//输出elf可执行文件
	return true;
}

Linker::~Linker()
{
	//清空合并段序列
	for(hash_map<string,SegList*,string_hash>::iterator i=segLists.begin();i!=segLists.end();++i)
	{
		delete i->second;
	}
	segLists.clear();
	//清空符号引用序列
	for(vector<SymLink*>::iterator i=symLinks.begin();i!=symLinks.end();++i)
	{
		delete *i;
	}
	symLinks.clear();
	//清空符号定义序列
	for(vector<SymLink*>::iterator i=symDef.begin();i!=symDef.end();++i)
	{
		delete *i;
	}
	symDef.clear();
	//清空目标文件
	for(int i=0;i<elfs.size();++i)
	{
		delete elfs[i];
	}
	elfs.clear();
}
