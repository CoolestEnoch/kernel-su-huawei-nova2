#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <huawei_platform/log/hw_log.h> 
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <huawei_platform/usb/huawei_hishow.h>


static struct class *hishow_class = NULL;
static struct device *hishow_device = NULL;
static int hishow_dev_state = HIVIEW_DEVICE_OFFLINE;
static int hishow_dev_no = HIVIEW_INVALID_DEVICENO;
 
static ssize_t hishow_dev_info_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
	char* dev_state[3] = { "UNKNOWN" ,"DISCONNECTED", "CONNECTED"};
	char* cur_state = dev_state[0];
	hw_hishow_dbg("HiView Dev State : %s : %x\n", (hishow_dev_state==HIVIEW_DEVICE_OFFLINE ? "OFFLINE" : "ONLINE"),hishow_dev_no);
	switch(hishow_dev_state)
	{
	case HIVIEW_DEVICE_OFFLINE:
		cur_state = dev_state[1];
		break;
	case HIVIEW_DEVICE_ONLINE:
		cur_state = dev_state[2];
		break;
	default:
		break;
	}
	return scnprintf(buf, PAGE_SIZE, "%s:%d\n", cur_state,hishow_dev_no);
}

static DEVICE_ATTR(dev, S_IRUGO, hishow_dev_info_show, NULL);

static struct attribute *hishow_ctrl_attributes[] = {
	&dev_attr_dev.attr,
	NULL,
};
static const struct attribute_group hishow_attr_group = {
	.attrs = hishow_ctrl_attributes,
};

void hishow_notify_android_uevent(int disconnedOrNot,int hishow_devno)
{
	char *disconnected[3] = { "HISHOWDEV_STATE=DISCONNECTED", NULL,NULL };
	char *connected[3]    = { "HISHOWDEV_STATE=CONNECTED", NULL,NULL };
	char  device_data[32] = {0};

	hw_hishow_dbg("hishow_notify_android_uevent  hishow_devno(%d) ++++ \n",hishow_devno);
	if(IS_ERR(hishow_device) || (!hishow_device))
	{
		hw_hishow_err("hishow_notify_android_uevent  device uninit \n");
		return;
	}

	if(hishow_devno <= 0 || hishow_devno > HIVIEW_DEVICE_MAXNO)
	{
		hw_hishow_err("hishow_notify_android_uevent  hishow_devno(%d) error \n",hishow_devno);
		return;
	}
	hishow_dev_state = disconnedOrNot;
	hishow_dev_no = hishow_devno;

	snprintf(device_data , 32 , "DEVICENO=%d",hishow_devno); 

	switch(disconnedOrNot)
	{
	case HIVIEW_DEVICE_ONLINE:
	       {
			   connected[1] = device_data;
			   kobject_uevent_env(&hishow_device->kobj,
				   KOBJ_CHANGE, connected);
			   hw_hishow_dbg("hishow_notify_android_uevent  kobject_uevent_env connected \n");
	       }
	       break;
	case HIVIEW_DEVICE_OFFLINE:
	       {
			   disconnected[1] = device_data;
			   kobject_uevent_env(&hishow_device->kobj,
				   KOBJ_CHANGE, disconnected);
			   hw_hishow_dbg("hishow_notify_android_uevent  kobject_uevent_env disconnected \n");
	       }
	       break;
	}
}
EXPORT_SYMBOL_GPL(hishow_notify_android_uevent);

static void hishow_destroy_monitor_device(struct platform_device *pdev)
{
	if(!pdev)
	{
	   return;
	}
	if(!IS_ERR(hishow_device))
	{
		sysfs_remove_group(&hishow_device->kobj, &hishow_attr_group);
		device_destroy(hishow_device->class, hishow_device->devt);
	}
	if (!IS_ERR(hishow_class))
	{
		class_destroy(hishow_class);
	}
	hishow_device = NULL;
	hishow_class = NULL;
}

static void hishow_init_monitor_device(struct platform_device *pdev)
{
	int ret = -1;
	if(hishow_device  || hishow_class)
	{
		hishow_destroy_monitor_device(pdev);
	}
	hishow_class = class_create(THIS_MODULE, "hishow");
	if (IS_ERR(hishow_class))
	{
		ret =  PTR_ERR(hishow_class);
		goto err_init;
	}

	hishow_device = device_create(hishow_class, NULL,0, NULL, "monitor");
	if (IS_ERR(hishow_device))
	{
		ret =  PTR_ERR(hishow_device);
		goto err_init;
	}
	ret = sysfs_create_group(&hishow_device->kobj, &hishow_attr_group);
	if (ret)
	{
		hw_hishow_err("%s: hishow sysfs group create error\n", __func__);
	}
	return ;
err_init:
	hw_hishow_err("hishow_init_monitor_device failed %x \n",ret);
	hishow_destroy_monitor_device(pdev);
	return;
}

/**********************************************************
*  Function:       hishow_probe
*  Description:    hishow module probe
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/

static int hishow_probe(struct platform_device *pdev)
{
	hishow_init_monitor_device(pdev);
	return 0; 
}

/**********************************************************
*  Function:       hishow_remove
*  Description:    hishow module remove
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int hishow_remove(struct platform_device *pdev)
{
	hishow_destroy_monitor_device(pdev);
	return 0;
}


static struct of_device_id hishow_match_table[] = {
	{
		.compatible = "huawei,hishow",
		.data = NULL,
	},
	{
	},
};

static struct platform_driver hishow_driver = {
	.probe = hishow_probe,
	.remove = hishow_remove,
	.driver = {
		   .name = "huawei,hishow",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(hishow_match_table),
	},
};

/**********************************************************
*  Function:       hishow_init
*  Description:    hishow module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/

static int __init hishow_init(void)
{
    return  platform_driver_register(&hishow_driver);
}

/**********************************************************
*  Function:       hishow_exit
*  Description:    hishow module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit hishow_exit(void)
{
	platform_driver_unregister(&hishow_driver);
	return;
}

module_init(hishow_init);
module_exit(hishow_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei hishow event module driver");
MODULE_AUTHOR("HUAWEI Inc");
