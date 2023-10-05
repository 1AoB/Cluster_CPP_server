# 遇到的问题

1. /usr/bin/ld: cannot find -lhiredis
解决办法: sudo apt-get install libhiredis-dev

2.如何查看占用6000端口的进程,并将它杀死?
在终端中运行以下命令以查找占用6000端口的进程ID（PID）:
lsof -i :6000

这将显示正在使用6000端口的进程及其PID。

确定要终止的进程的PID。

kill -9 <PID>
