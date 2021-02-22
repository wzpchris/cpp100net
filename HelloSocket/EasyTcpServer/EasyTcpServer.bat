@echo off
::服务端IP地址
::set strIP="any"
rem 服务端端口
::set nPort=4567
::消息处理线程数量
::set nThread=1
::客户端连接上限
::set nClient=2

::服务端IP地址
set cmd="strIP=127.0.0.1"
rem 服务端端口
set cmd=%cmd% nPort=4567
::消息处理线程数量
set cmd=%cmd% nThread=1
::客户端连接上限
set cmd=%cmd% nMaxClient=10000
::客户端发送缓冲区大小(字节) 
set cmd=%cmd% nSendBuffSize=20480
::客户端接收缓冲区大小(字节)
set cmd=%cmd% nRecvBuffSize=20480
::收到消息后将返回应答消息
set cmd=%cmd% -sendback
::提示发送缓冲区已写满
::当出现sendfull提示时，表示当次消息被丢弃
set cmd=%cmd% -sendfull
::检查接收到的客户端消息ID是否连续
set cmd=%cmd% -checkMsgID
::自定义标志 未使用
set cmd=%cmd% -p

::EasyTcpServer %strIP% %nPort% %nThread% %nClient%
EasyTcpServer %cmd%

pause