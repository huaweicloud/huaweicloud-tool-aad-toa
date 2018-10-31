本目录下包含Makefile、toa.c两个有效文件。

1、将这两个文件放到toa源码目录下：linux-2.6.32-220.23.1.el6.x86_64.rs/net/toa/，
 覆盖此目录下的Makefile、toa.c两个文件。原有的Makefile、toa.c请手动备份；

2、之后，在此目录下直接编译，执行make，若无错误，会新生成toa.ko；

3、将新生成的toa.ko复制到原toa内核模块路径下，替换原有toa.ko,（路径一般为：/lib/modules/`uname -r`/kernel/net/toa/toa.ko，查看toa详细路径也可通过命令：modinfo toa ,原有toa.ko请手动备份）；

4、进入toa内核目录：cd /lib/modules/`uname -r`/kernel/net/toa/
   卸载toa，执行：rmmod toa 
   重新挂载本目录下的新的toa，执行：modprobe toa   （或 insmod ./toa.ko）
   若无任何报错，即为安装成功，通过命令：lsmod | grep toa ，将会显示出已安装的toa模块；
   
至此，toa升级完成。

5、为保证系统重启仍然能自动挂载toa，请在root用户下执行以下命令：
   echo "modprobe toa" >> /etc/rc.local
   若不想重启之后自动挂载toa，请忽略此条
