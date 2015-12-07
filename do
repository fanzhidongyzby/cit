#/bin/sh
echo 更新编译器
cd ./compiler
make
cd ..
echo 生成编译程序./compiler/compiler
echo 更新汇编器
cd ./ass
make
cd ..
echo 生成汇编程序./ass/ass
echo 更新链接器
cd ./lit
make
cd ..
echo 生成链接程序./lit/lit
echo 更新命令控制
make
echo 生成命令控制程序./cit 
