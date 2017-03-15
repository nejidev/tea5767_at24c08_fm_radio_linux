#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

//io ctrl 命令
#define IOCTL_CMD_SET_FQCY 101 // 设置一个频率
#define IOCTL_CMD_SET_MUTE 102 // 设置 静音
#define TEA5767_MAX_KHZ 108000 // 最高频率 108M
#define TEA5767_MIN_KHZ 87500  // 最低频率 87.5M
#define TEA5767_ADC_LEVEL 9     // 搜台电平有效值

int tea5767_fd, at24cxx_fd;

//使用mmap内存映射 不必每次读写I2C
//因为是 99.6Mhz 小数保存 不方便 
unsigned long *at24cxx_buf;

//当前频道号
int ch = 0;

void welcome()
{
	printf("/////////////////////////////////////////\n");
	printf("/ TEA5767 + AT24Cxx Auto Serach Redio   /\n");
	printf("/ Power By: Ning Ci                     /\n");
	printf("/ Press key 's' To Auto Serach All Band /\n");
	printf("/ Press key 'm' To Redio Mute           /\n");
	printf("/ Press key 'p' To Set Redio Prev CH    /\n");
	printf("/ Press key 'n' To Set Redio Next CH    /\n");
	printf("/ Press key 'q' To Quit Close Redio     /\n");
	printf("/////////////////////////////////////////\n");
}
int set_frequency(long Khz)
{
	int adc = 0;
	adc = ioctl(tea5767_fd, IOCTL_CMD_SET_FQCY, Khz);
	return adc;
}

void serach()
{
	int adc = 0;
	ch = 0;
	//从低向高搜索
	int Khz = TEA5767_MIN_KHZ;
	printf("Wait Seraching ... \n");
	//搜到最高频率 退出
	while(Khz <= TEA5767_MAX_KHZ)
	{
		adc = set_frequency(Khz);
		printf("%.1f Mhz Dac : %d \n", (float)Khz/1000, adc);
		
		//判断ADC电平 是否搜到台
		if(TEA5767_ADC_LEVEL < adc)
		{
			//存台
			at24cxx_buf[ch] = Khz;
			ch++;
		}
		Khz += 100;
	}
	printf("done find %d Redio\n", ch);
	ch = 0;
}

void mute()
{
	static int mute_stat = 1;
	mute_stat++;

	if(mute_stat%2)
	{
		ioctl(tea5767_fd, IOCTL_CMD_SET_MUTE, 0);
		printf("Mute OFF \n");
	}
	else
	{
		ioctl(tea5767_fd, IOCTL_CMD_SET_MUTE, 1);
		printf("Mute ON \n");
	}
}

void set_ch(char ch)
{
	ch = (0>ch) ? 0 : ch;
	printf("Ch : %d %.1f Mhz\n", ch, (float)at24cxx_buf[ch]/1000);
	set_frequency(at24cxx_buf[ch]);
}

//测试用
void test()
{
	//北京 这边 只搜到了一个信号最强的 交通台
	printf("Test 99.6 Mhz \n");
	set_frequency(99.6*1000);
}

int main(int argc, char **argv)
{
	//控制命令 频道数 (应该没有127个台)
	char cmd;
	
	//打印使用信息
	welcome();

	//打开2个设备
	tea5767_fd = open("/dev/tea5767", O_RDWR);
	if(0 > tea5767_fd)
	{
		printf("cat't open tea5767 \n");
		return 0;
	}

	at24cxx_fd = open("/dev/at24cxx", O_RDWR);
	if(0 > at24cxx_fd)
	{
		printf("cat't open at24cxx \n");
		return 0;
	}

	//at24cxx mmap
	at24cxx_buf = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, at24cxx_fd,0);
	if(((void *)-1) == at24cxx_buf)
	{
		printf("at24cxx mmap error \n");
		close(tea5767_fd);
		close(at24cxx_fd);
		return -1;
	}
	
	//读取 at24cxx 中保存的电台
	//默认打开第一个电台
	set_frequency(at24cxx_buf[ch]);
	
	while('q' != (cmd = getchar()))
	{
		switch(cmd)
		{
			case 't': test();       break;
			case 's': serach();     break;
			case 'm': mute();       break;
			case 'p': set_ch(--ch); break;
			case 'n': set_ch(++ch); break;
			case 'h': welcome();    break;
		}
	}
	
	//最后关闭设备 其实程序一直在循环中 按 Ctrl + c 退出后 程序会自动关闭 打开的文件
	close(tea5767_fd);
	munmap(at24cxx_buf, 1024);
	close(at24cxx_fd);
	return 0;
}

