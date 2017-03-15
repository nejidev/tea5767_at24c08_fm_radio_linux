#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/delay.h>

//主设备号
static int major;
//类描述为创建设备使用
static struct class *cls;
//i2c client 发送接收数据使用
static struct i2c_client *tea5767_client;

//i2c table 和 board 中进行比较 相同调用 probe
static struct i2c_device_id tea5767_idtable[] = {
	{"tea5767", 0},{}
};

//当前设定 频率
static unsigned long now_Khz = 0;

// TEA5767 I2C 发送数据
struct tea5767_dat
{
	unsigned char tab1;
	unsigned char tab2;
	unsigned char tab3;
	unsigned char tab4;
	unsigned char tab5;
};

#define TEA5767_CLK     32768  //tea5767 晶振
#define TEA5767_MAX_KHZ 108000 //最高频率 108M
#define TEA5767_MIN_KHZ 87500  //最低频率 87.5M

// Khz 转为 pll
unsigned long tea5767_Khz_to_pll(unsigned long Khz)
{
	if(TEA5767_MAX_KHZ >= Khz && TEA5767_MIN_KHZ <= Khz)
	{
		return 4*(Khz*1000-225000)/TEA5767_CLK;
	}
	return 0;
}

// 转为 Khz
unsigned long tea5767_pll_to_Khz(struct tea5767_dat dat)
{
	unsigned long Khz,pll; 	
	pll = ((dat.tab1 & 0x3f)<<8 | dat.tab2);
	Khz = (pll*TEA5767_CLK/4+225*1000)/1000;
	return Khz;
}
static void tea5767_i2c_write(struct tea5767_dat dat)
{
	unsigned char *p;
	p = (unsigned char *)&dat;
	//调用 SMBus 发送
	//*p 是 第一个数据 &dat.tab2 是 后面4个数据
	i2c_smbus_write_i2c_block_data(tea5767_client, *p, 4, &dat.tab2);
}

static struct tea5767_dat tea5767_i2c_read(void)
{
	struct tea5767_dat dat;
	// 经过查询 SMBus 没有适合的函数
	//格式:发出TEA读地址后,读5个unsigned char
	i2c_master_recv(tea5767_client, (unsigned char *)&dat, 5);
	return dat;
}

// 获取 TEA5767 信号电平 强度
static int get_tea5767_adc(void)
{
	struct tea5767_dat dat ;
	dat = tea5767_i2c_read();
	return dat.tab4>>4;
}

// 设置 TEA5767 接收频率
static int set_tea5767_frequency(unsigned long Khz)
{
	int pll;
	struct tea5767_dat dat;
	now_Khz = Khz;
	pll = tea5767_Khz_to_pll(Khz);
	dat.tab1  = (pll>>8) & 0x3f;
	dat.tab2  = pll;
	dat.tab3  = 1;
	dat.tab4  = 0x11;
	dat.tab5  = 0x40;
	tea5767_i2c_write(dat);

	//休眠300ms后读取ADC电平 tea5767芯片不能马上反应过来
	mdelay(300);
	//设定频率后 返回 电台 ADC 信号电平
	return get_tea5767_adc();
}

// 设置 TEA5767 是否静音 1 静音 0 不静音
static void set_tea5767_mute(unsigned long mute)
{
	int pll;
	struct tea5767_dat dat;
	pll = tea5767_Khz_to_pll(now_Khz);
	dat.tab1  = (mute<<7) | ((pll>>8) & 0x3f);
	dat.tab2  = pll;
	dat.tab3  = 1;
	dat.tab4  = 0x11;
	dat.tab5  = 0x40;
	tea5767_i2c_write(dat);
}

//io ctrl 命令
#define IOCTL_CMD_SET_FQCY 101 // 设置一个频率
#define IOCTL_CMD_SET_MUTE 102 // 设置 静音

static long tea5767_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	//经过测试 cmd 不能是2 否则 不执行 原因未知 有知道的告诉我下
	switch(cmd)
	{
		case IOCTL_CMD_SET_FQCY: ret = set_tea5767_frequency(arg); break;
		case IOCTL_CMD_SET_MUTE: set_tea5767_mute(arg); break;
		default:break;
	}
	return ret;
}

static struct file_operations tea5767_ops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = tea5767_ioctl,
};

//当idtab中比较相同以后调用这个
static int tea5767_probe(struct i2c_client *client,const struct i2c_device_id *dev)
{
	//设给全局变量
	tea5767_client = client;
	//创建字库设备
	cls = class_create(THIS_MODULE, "tea5767");
	major = register_chrdev(0, "tea5767", &tea5767_ops);
	device_create(cls, NULL, MKDEV(major, 0), NULL, "tea5767");
	return 0;
}

static int tea5767_remove(struct i2c_client *client)
{
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
	unregister_chrdev(major, "tea5767");
	return 0;
}

static struct i2c_driver tea5767_drv = {
	.driver = {
		.name  = "tea5767",
		.owner = THIS_MODULE,
	},
	.probe  = tea5767_probe,
	.remove = tea5767_remove,
	.id_table = tea5767_idtable,
};

static int tea5767_init(void)
{
	i2c_add_driver(&tea5767_drv);
	return 0;
}

static void tea5767_exit(void)
{
	i2c_del_driver(&tea5767_drv);
}

module_init(tea5767_init);
module_exit(tea5767_exit);
MODULE_LICENSE("GPL");

