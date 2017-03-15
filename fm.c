#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

//io ctrl ����
#define IOCTL_CMD_SET_FQCY 101 // ����һ��Ƶ��
#define IOCTL_CMD_SET_MUTE 102 // ���� ����
#define TEA5767_MAX_KHZ 108000 // ���Ƶ�� 108M
#define TEA5767_MIN_KHZ 87500  // ���Ƶ�� 87.5M
#define TEA5767_ADC_LEVEL 9     // ��̨��ƽ��Чֵ

int tea5767_fd, at24cxx_fd;

//ʹ��mmap�ڴ�ӳ�� ����ÿ�ζ�дI2C
//��Ϊ�� 99.6Mhz С������ ������ 
unsigned long *at24cxx_buf;

//��ǰƵ����
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
	//�ӵ��������
	int Khz = TEA5767_MIN_KHZ;
	printf("Wait Seraching ... \n");
	//�ѵ����Ƶ�� �˳�
	while(Khz <= TEA5767_MAX_KHZ)
	{
		adc = set_frequency(Khz);
		printf("%.1f Mhz Dac : %d \n", (float)Khz/1000, adc);
		
		//�ж�ADC��ƽ �Ƿ��ѵ�̨
		if(TEA5767_ADC_LEVEL < adc)
		{
			//��̨
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

//������
void test()
{
	//���� ��� ֻ�ѵ���һ���ź���ǿ�� ��̨ͨ
	printf("Test 99.6 Mhz \n");
	set_frequency(99.6*1000);
}

int main(int argc, char **argv)
{
	//�������� Ƶ���� (Ӧ��û��127��̨)
	char cmd;
	
	//��ӡʹ����Ϣ
	welcome();

	//��2���豸
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
	
	//��ȡ at24cxx �б���ĵ�̨
	//Ĭ�ϴ򿪵�һ����̨
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
	
	//���ر��豸 ��ʵ����һֱ��ѭ���� �� Ctrl + c �˳��� ������Զ��ر� �򿪵��ļ�
	close(tea5767_fd);
	munmap(at24cxx_buf, 1024);
	close(at24cxx_fd);
	return 0;
}

