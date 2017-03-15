#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/sysfs.h>

static struct i2c_board_info tea5767_info = {
	I2C_BOARD_INFO("tea5767", 0x60), // 7位地址
};

static struct i2c_client *tea5767_client;

static int tea5767_dev_init(void)
{
	struct i2c_adapter *i2c_ada;
	i2c_ada = i2c_get_adapter(0);
	tea5767_client = i2c_new_device(i2c_ada, &tea5767_info);
	i2c_put_adapter(i2c_ada);
	
	return 0;
}

static void tea5767_dev_exit(void)
{
	i2c_unregister_device(tea5767_client);
}

module_init(tea5767_dev_init);
module_exit(tea5767_dev_exit);
MODULE_LICENSE("GPL");

