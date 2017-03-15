#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>

static struct i2c_board_info at24cxx_info = {
	I2C_BOARD_INFO("at24c08", 0x50), // 7Œªµÿ÷∑
};

static struct i2c_client *at24cxx_client;

static int at24cxx_dev_init(void)
{
	struct i2c_adapter *i2c_ada;
	i2c_ada = i2c_get_adapter(0);
	at24cxx_client = i2c_new_device(i2c_ada, &at24cxx_info);
	i2c_put_adapter(i2c_ada);
	
	return 0;
}

static void at24cxx_dev_exit(void)
{
	i2c_unregister_device(at24cxx_client);
}

module_init(at24cxx_dev_init);
module_exit(at24cxx_dev_exit);
MODULE_LICENSE("GPL");

