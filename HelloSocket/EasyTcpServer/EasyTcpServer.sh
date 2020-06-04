cd `dirname $0`
#服务端IP地址
#strIP="any"
#服务端端口
#nPort=4567
#消息处理线程数量
#nThread=1
#客户端连接上限
#nClient=2

#./EasyTcpServer $strIP $nPort $nThread $nClient

#服务端IP地址
cmd='strIP=127.0.0.1'
#服务端端口
cmd=$cmd' nPort=4567'
#消息处理线程数量
cmd=$cmd" nThread=1"
#客户端连接上限
cmd="$cmd nClient=2"

./EasyTcpServer $cmd

read -p "...press any key to ecit..." var

