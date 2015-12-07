#include "common.h"
#include "iloc.h"

/*
	滑动窗口
*/
class Window
{
	int size;//窗口大小
	list<Arm*> cont;//窗口内的指令
	list<Arm*>& code;//ILOC代码序列
	list<Arm*>::iterator pos;//窗口位置，指向下个即将移入的指令
	bool match_single();//单指令模板匹配,若匹配执行替换动作
	bool match_double();//双指令模板匹配,若匹配执行替换动作
	bool match_triple();//三指令模板匹配,若匹配执行替换动作
public:
	Window(int sz,list<Arm*>& cd);//初始化窗口
	bool move();//窗口移动函数，跳过无效指令
	bool match();//指令模板匹配,若匹配执行替换动作
};

/*
	窥孔优化器
*/
class PeepHole
{
	ILoc&iloc;//ILoc代码
	bool __filter(int sz);//单趟匹配函数
public:
	PeepHole(ILoc&il);
	void filter();//优化
};