::服务端IP地址
@set strIP=any
rem 服务端端口
@set nPort=4567
::消息处理线程数量
@set nThread=1
::客户端连接上限
@set nClient=2

EasyTcpServer %setIP% %nPort% %nThread% %nClient%

@pause