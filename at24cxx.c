#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

//主设备号
static int major;
//类描述为创建设备使用
static struct class *cls;
//i2c client 发送接收数据使用
static struct i2c_client *at24cxx_client;

//互斥锁
static struct mutex at24cxx_mutex;

//i2c table 和 board 中进行比较 相同调用 probe
static struct i2c_device_id at24cxx_id_table[] = {
	{"at24c08", 0},
	{}
};

//mmap 使用内存
static unsigned char *at24cxx_buf;
//物理地址 使用和 lcd fb 一样的分配内存的方法 dma_alloc_writecombine
static dma_addr_t at24cxx_phys;

#define AT24cxx_BUF_SIZE 64

static int at24cxx_mmap(struct file *file, struct vm_area_struct *vma)
{
	u64 len = vma->vm_end - vma->vm_start;
	len = min(len, AT24cxx_BUF_SIZE);
	//no cache
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	return vm_iomap_memory(vma, at24cxx_phys, len);
}

static int at24cxx_open(struct inode *note, struct file *f)
{
	//打开时 填充 mmap 的buf
	int i;
	memset(at24cxx_buf, 0, AT24cxx_BUF_SIZE);
	for(i=0; i<AT24cxx_BUF_SIZE; i++)
	{
		at24cxx_buf[i] = i2c_smbus_read_byte_data(at24cxx_client, i);
	}
	return 0;
}

static int at24cxx_release(struct inode *note, struct file *f)
{
	//写 mmap 中的数据回 at24c08
	int i;
	for(i=0; i<AT24cxx_BUF_SIZE; i++)
	{
		i2c_smbus_write_byte_data(at24cxx_client, i, at24cxx_buf[i]);
		mdelay(100);
	}
	return 0;
}

static struct file_operations at24cxx_ops = {
	.owner   = THIS_MODULE,
	.open    = at24cxx_open,
	.release = at24cxx_release,
	.mmap    = at24cxx_mmap,
};

//当idtab中比较相同以后调用这个
static int at24cxx_probe(struct i2c_client *client,const struct i2c_device_id *dev)
{
	//初始化锁
	mutex_init(&at24cxx_mutex);
	//设给全局变量
	at24cxx_client = client;
	//创建字库设备
	cls = class_create(THIS_MODULE, "at24cxx");
	major = register_chrdev(0, "at24cxx", &at24cxx_ops);
	device_create(cls, NULL, MKDEV(major, 0), NULL, "at24cxx");

	//vmalloc 分配的物理地址连续
	at24cxx_buf = dma_alloc_writecombine(NULL, AT24cxx_BUF_SIZE, &at24cxx_phys, GFP_KERNEL);
	return 0;
}

static int at24cxx_remove(struct i2c_client *client)
{
	device_destroy(cls, MKDEV(major, 0));
	class_destroy(cls);
	unregister_chrdev(major, "at24cxx");
	dma_free_writecombine(NULL, AT24cxx_BUF_SIZE, at24cxx_buf, at24cxx_phys);
	return 0;
}

static struct i2c_driver at24cxx_drv = {
	.driver = {
		.name  = "at24cxx",
		.owner = THIS_MODULE,
	},
	.probe  = at24cxx_probe,
	.remove = at24cxx_remove,
	.id_table = at24cxx_id_table,
};

static int at24cxx_init(void)
{
	i2c_add_driver(&at24cxx_drv);
	return 0;
}

static void at24cxx_exit(void)
{
	i2c_del_driver(&at24cxx_drv);
}

module_init(at24cxx_init);
module_exit(at24cxx_exit);
MODULE_LICENSE("GPL");

