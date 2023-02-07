#ifndef HUAWEI_HISHOW_H
#define HUAWEI_HISHOW_H 

 
#define  HIVIEW_DEVICE_OFFLINE    0x1
#define  HIVIEW_DEVICE_ONLINE     0x10

//HIVIEW  DEVICE NO Define

#define  HIVIEW_USB_DEVICE		 1
#define  HIVIEW_DEVICE_MAXNO     8
#define  HIVIEW_INVALID_DEVICENO  (-1)

void hishow_notify_android_uevent(int disconnedOrNot,int hishow_devno);

#define hw_hishow_dbg(format, arg...)    \
	do {                 \
		printk(KERN_INFO "[HISHOW_DEBUG][%s]"format, __func__, ##arg); \
	} while (0)
#define hw_hishow_err(format, arg...)    \
	do {                 \
		printk(KERN_ERR "[HISHOW_DEBUG]"format, ##arg); \
	} while (0)

#endif
