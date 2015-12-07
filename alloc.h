#pragma once
#include "common.h"
#include "set.h"
/*******************************************************************************
                            为变量分配寄存器或者栈帧偏移
*******************************************************************************/

/*
	冲突图节点类型
*/
struct Node
{
	Var*var;//节点对应的变量
	int degree;//节点的度，跟踪节点边的个数，为图着色算法提供选择依据
	int color;//节点颜色，与var->regId对应，-1为无效值（无法/不能着色）
	Set exColors;//节点排斥的颜色，避免相邻节点着同色。其补集中第一个有效元素为color！
	vector<Node*> links;//节点的连接边，按照有序插入，使用二分查找是否存在已有邻居
	Node(Var*v,Set& E);//创建v对应的节点，初始化
	void addLink(Node*node);//添加一条边，有序插入，度+1！保证二分查找
	void addExColor(int color);//添加一个排除色
	void paint(Set& colorBox);//根据exColors选择第一个有效的颜色，度=-1
};

/*
	作用域类型
*/
struct Scope
{
	struct scope_less
	{
		bool operator()(Scope*left,Scope*right){
			return left->id<right->id;//按照id确定节点大小
		}
	};
	int id;
	int esp;
	vector<Scope*> children;//子作用域
	Scope(int i,int addr);
	~Scope();
	Scope* find(int i);//查找i作用域，不存在则产生一个子作用域，id=i，esp继承当前父esp
	//输出信息控制，|--开始位置
	int x;//列位置，实际未用到
	int y;//行位置
	Scope*parent;//记录父亲节点
};

/*
	冲突图类型
*/
class CoGraph
{
	struct node_less
	{
		bool operator()(Node*left,Node*right){
			return left->degree<=right->degree;//按照度确定节点大小,非严格小于，避免多余移动操作
		}
	};
	list<InterInst*> optCode;//中间代码
	LiveVar* lv;//活跃变量分析对象，使用该对象提供集合对应的变量信息
	vector<Node*> nodes;//图节点数组，图着色选择最大度节点时，使用堆排序！
	vector<Var*>varList;//变量列表，缓存所有需要分配的变量
	Set U;//颜色集合全集
	Set E;//颜色集合空集
	Scope* scRoot;//根作用域指针
	Fun* fun;//函数对象
	Node* pickNode();//选择度最大的未处理节点，利用最大堆根据节点度堆排序
	void regAlloc();//寄存器分配图着色算法,将regNum个寄存器着色到图上
	int& getEsp(vector<int>& path);//根据当前变量的作用域路径获取栈帧偏移地址
	void stackAlloc();//为不能着色的变量分配栈帧地址
	void __printTree(Scope*root,int blk,int x,int& y);//树形输出作用域
	void printTree(Scope*root,bool tree_style=true);//封装的函数
public:
	//参数列表，局部变量DEC声明创建所有变量的Node。 
	//参数变量如果分配后在寄存器内，需要在函数开始处添加load指令
	//全局变量不作寄存器优化！！！
	CoGraph(list<InterInst*>&optCode,vector<Var*>&para,LiveVar*lv,Fun*f);
	~CoGraph();
	void alloc();//分配算法
};