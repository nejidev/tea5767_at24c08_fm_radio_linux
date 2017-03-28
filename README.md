TEA5676 + AT24C08 FM收音机 搜台 存台 

硬件说明

TEA5767 + AT24c08 
要使用耳机收听，不加功放芯片，声音非常小。

这2个芯片都支持 3.3 或 5.0 电源支持
连线比较简单，sda scl 接到 2440 对应的 排针上，找出一路 3.3v GND ，这里我是从 com 口上接出。
如下接线图，tea5767 at24c08 来自于，之前用 51 单片机做的 FM 收音机，因为电平不同，就把 51 单片机取下来了。
1602 因为 2440 开发板，没有这么多可用 GPIO 这次就不接了。

中间的转换板是我自己焊的，是一个 5V 转 3.3V 和 LED 台灯 和 排针连接工具。 用的是 asm1117 3.3 。
制作中出现过一个严重的问题，led 灯全烧了，之前用 笔记本上面的 usb 供电，led 灯虽然说是亮些，但是没有烧过。
这次新买了一个 台达5v 5a 电源，接上线后全烧了。加了一个100欧，限流电阻后正常了。 

编译环境
ubunto 12 
arm-linux-gcc 4.3.2

使用说明
本项目基于 linux4.3.36 （2.6 内核肯定是不能用，3.1 之后的也许可以，如果不能用请自行修改）
首先要配置内核启用 i2c 的支持
make menuconfig
-> Device Drivers
   -> I2C support
      -> I2C support (I2C [=y])
         -> I2C Hardware Bus support    

-> Device Drivers
    -> I2C support
       -> I2C support (I2C [=y])


基本上有难度的是 at24c08 的 mmap 实现。 不是普通的读写。 这样做，程序写起来比较简单。

打开 fm 应用程序后，打开 tea5767 和 at24c08 对 at24c08 进行 mmap 映射为 unsigned long 来表现，电台的 频率， 单位是 Khz （因为小数不容易存）。
搜到台以后， 把台的频率（单位是Khz）保存到 mmap 中， 退出的时候，在写回 at24c08 。简化了程序。

内核驱动中需要注意的是， mmap 的 内存要求是物理地址连续，所以使用 dma_alloc_writecombine 分配。

请把 项目中的所有文件，下载到本地，修改 Makefile 中的 内核路径，除非你的和我的一样，这样就不用修改了。
执行 
在项目目录中 执行 make
生成 4个 ko 文件 和 fm 应用程序。 把这5个文件复制到，开发板上，或 nfs 中。

启动开发板，请一定要注意，内核有 i2c 硬件支援，然后加载4个ko 文件。
insmod tea5767/tea5767.ko
insmod tea5767/tea5767_dev.ko
insmod tea5767/at24cxx.ko
insmod tea5767/at24cxx_dev.ko

执行测试 fm 程序
tea5767/fm

输入 s 按回车 自动搜台
搜完毕后，自动打开第1个台
可以按 n 切换下一台
可以按 p 切换上一台
可以按 q 退出
可以按 h 查看帮助信息
可以按 t 进行测试模式 这里收听一个 北京台 99.6Mhz
打错了字 redio 应该是 radio
