# huaweicloud-tool-aad-toa

华为高防toa模块：

	在开源toa的基础上做了优化，可直接在Linux服务器（centos系列，其他系统需要定制）上编译安装。

	操作步骤：
	>> make    #进入对于目录下，有gcc即可
	>> rmmod toa #卸载以前的toa版本
	>> insmod toa   #挂在编译好的toa
	>> lsmod | grep toa #查看是否成功


	当前toa模块新增功能：
	1，针对指定源ip做统计


	其他：
	如有疑问会改进建议，欢迎留言。
