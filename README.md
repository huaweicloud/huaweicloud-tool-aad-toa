huaweicloud-tool-aad-toa
------------------------

华为高防（https://www.huaweicloud.com/product/aad.html） 专用toa模块，在开源toa的基础上做了优化，用于在服务端解析真实源ip，可直接在Linux服务器上编译安装。



## 版本选择 ##
 
当前git仓库有3套toa代码：

centos6.5————对应Linux内核2.6.x

centos7————对应Linux内核3.10.x

toa_common————通用版本toa，一般针对Linux内核3.0及其以上的系统，如Ubuntu 14/16 、Suse 11/42等



## 编译安装 ##

 
下载代码并进入对应版本目录下，执行：
```shell
1、 make
```   


    

> 根据自己的服务器版本而定，有gcc即可；
如果报错显示找不到指定/lib/modules/xxxx/build/目录，表示系统缺少相关的头文件，无法编译，此时需要在系统上安装缺少的头文件
	执行：
	
```
	yum install kernel-devel  
	yum install kernel-headers
```
>>以上表示从默认的repo源下载内核相关扩展文件，安装之后一般情况在：/lib/modules/`uname -r`/build/目录下就会有正确的源码文件，若此目录报红，表示路径不正确，有可能此种方法安装的扩展源码版本与系统源码版本还是不对应，结果仍然无法编译。此时，
		可以考虑替换内核版本,执行：
		
```	
        yum install kernel
        yum install kernel-firmware
        yum install kernel-devel
        yum install kernel-headers
```
>>>安装内核源码，默认以上的四个命令安装的版本都是一样的（安装的时候可以看到，如果不一样，那就不能达到预计效果），安装完成之后                         在"/lib/modules/xxx"目录下就可以看到新的内核，对应的build目录下是有正常内容的，此时需要将已有的系统内核切换到新的：
		    此处提供centos7的切换方法，首先查询新的内核版本的具体名称，执行：
```
            	    cat  /boot/grub2/grub.cfg  | grep menuentry
```
>>>>执行结果：
                                'menuentry 'CentOS Linux (3.10.0-862.11.6.el7.x86_64) 7 (Core)' --class centos --class gnu-linux --class gnu --class os --unrestricted $menuentry_id_option 'gnulinux-3.10.0-693.2.2.el7.x86_64-advanced-eb448abb-3012-4d8d-bcde-94434d586a31' {menuentry 'CentOS Linux (3.10.0-693.2.2.el7.x86_64) 7 (Core)' --class centos --class gnu-linux --class gnu --class os --unrestricted $menuentry_id_option 'gnulinux-3.10.0-693.2.2.el7.x86_64-advanced-eb448abb-3012-4d8d-bcde-94434d586a31' {menuentry 'CentOS Linux (3.10.0-693.el7.x86_64) 7 (Core)' --class centos --class gnu-linux --class gnu --class os --unrestricted $menuentry_id_option 'gnulinux-3.10.0-693.el7.x86_64-advanced-eb448abb-3012-4d8d-bcde-94434d586a31' {......'
			        
>>>'CentOS Linux (3.10.0-862.11.6.el7.x86_64) 7 (Core)'此版本即为我新install的版本。
			        设置切换默认内核，执行:
```
 				grub2-set-default "CentOS Linux (3.10.0-862.11.6.el7.x86_64)"
```

>>>不报错即为成功

>>重启生效，执行：
```
reboot
```
>>重启之后查看当前内核是否替换成功，执行：
```
uname -r
```
>>结果显示：“3.10.0-862.11.6.el7.x86_64” ，即为成功，之后在回到toa的源码目录，make即可。
			
>>注意：内核的替换是有风险的，可能造成某些依赖底层的功能失效，甚至造成不可估量的结果。建议是在不需要太过依赖指定内核版本的情况下替换，替换之后，为保证原有进程的可靠性，建议充分测试。

```
2、rmmod toa #卸载以前的toa版本
3、insmod toa.ko   #挂载编译好的toa
4、lsmod | grep toa #查看是否成功，有显示即为成功
```
## 常见问题 ##
以下解决重启自动加载toa的问题：

>make成功之后在本目录下会生成toa.ko，可将此toa.ko复制到：
>/lib/modules/3.10.0-862.11.6.el7.x86_64/kernel/net/netfilter/ipvs/ 目录下:
```
	cp toa.ko /lib/modules/3.10.0-862.11.6.el7.x86_64/kernel/net/netfilter/ipvs/  #不一定存放在此目录，只是为了重启之后被加载
```
	>>此处提供centos重启挂载toa的方式：
		>>在/etc/sysconfig/modules/目录下加入脚本文件，方便重启加载toa
		```
		cd /etc/sysconfig/modules/
		echo "insmod /lib/modules/3.10.0-862.11.6.el7.x86_64/kernel/net/netfilter/ipvs/toa.ko" > toa.modules  # 创建toa.modules文件用于重启默认加载
		chmod 755 toa.modules  #增加权限，重启即可生效
		```

