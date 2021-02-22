# cpp100net


#-------------windows-----------
#默认的动态端口范围
netsh int ipv4 show dynamicport tcp
netsh int ipv4 show dynamicport udp
netsh int ipv6 show dynamicport tcp
netsh int ipv6 show dynamicport udp

#使用如下命令可以重新设置
netsh int set dynamic start=number num=range
#简单例子如下:
netsh int ipv4 set dynamic tcp start=1025 num=64510
netsh int ipv4 set dynamic udp start=10000 num=1000
netsh int ipv6 set dynamic tcp start=10000 num=1000
netsh int ipv6 set dynamic udp start=10000 num=1000

#-------------linux-----------
#查看可用端口范围
cat /proc/sys/net/ipv4/ip_local_port_range
#修改可用端口范围
sudo gedit /etc/sysctl.conf
net.ipv4.ip_local_port_range = 1024 65535
sysctl -p
#当前终端执行的进程最多允许同时打开的文件句柄数
ulimit -n
#默认单个进程最多允许同时打开的文件句柄数
ulimit -Sn
#单个进程最多允许同时打开的文件句柄数
ulimit -Hn
#修改单一进程能同时打开的文件句柄数有2种方法:
1、直接使用ulimit命令,如:
ulimit -n 1048576
执行成功之后，ulimit n、Sn、Hn的值均会变为1048576，但该方法设置的值只会在当前终端有效，且设置的值不能保存
2、对 /etc/security/limits.conf文件，添加或修改
* soft nofile 1048576
* hard nofile 1048576
sudo gedit /etc/security/limits.conf
然后重启
-----------------------
#系统所有进程最多允许同时打开的文件句柄数
cat /proc/sys/fs/file-max
要修改它，需要对 /etc/sysctl.conf文件，增加一行内容:
sudo gedit /etc/sysctl.conf
fs.file-max = 1048576