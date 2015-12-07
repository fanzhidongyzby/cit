#include "compiler.h"
#include "semantic.h"
int main(int argc,char*argv[])
{
	Compiler cmplr(argv[2][0]=='y',argv[3][0]=='y',argv[4][0]=='y',argv[5][0]=='y',argv[6][0]=='y');
	cmplr.genCommonFile();
	cmplr.compile(argv[1]);
	return 0;
}
