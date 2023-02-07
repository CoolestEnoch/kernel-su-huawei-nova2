

#include <linux/module.h>
#include <linux/printk.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>

#include "hwsensor.h"
#include "sensor_commom.h"
#include "hw_csi.h"

#define I2S(i) container_of(i, sensor_t, intf)

static char *sensor_dts_name = "HI846_FOXCONN";
static bool power_on_status = false;//false: power off, true:power on
/*lint -e826 -e31 -e485 -e785 -e731 -e846 -e514 -e866 -e30 -e84 -e838 -e64 -e528 -e753 -e749 -e715 -esym(826, 31, 485, 785, 731, 846, 514, 866, 30, 84, 838, 64, 528, 753, 749, 715*)*/
/*lint -save -e826 -e31 -e485 -e785 -e731 -e846 -e514 -e866 -e30 -e84 -e838 -e64 -e528 -e753 -e749 -e715 -specific(-e826 -e31 -e485 -e785 -e731 -e846 -e514 -e866 -e30 -e84 -e838 -e64 -e528 -e753 -e749 -e715)*/

char const* hi846_get_name(hwsensor_intf_t* si);
int hi846_config(hwsensor_intf_t* si, void  *argp);
int hi846_power_up(hwsensor_intf_t* si);
int hi846_power_down(hwsensor_intf_t* si);
int hi846_match_id(hwsensor_intf_t* si, void * data);
int hi846_csi_enable(hwsensor_intf_t* si);
int hi846_csi_disable(hwsensor_intf_t* si);

static hwsensor_vtbl_t s_hi846_vtbl =
{
    .get_name = hi846_get_name,
    .config = hi846_config,
    .power_up = hi846_power_up,
    .power_down = hi846_power_down,
    .match_id = hi846_match_id,
    .csi_enable = hi846_csi_enable,
    .csi_disable = hi846_csi_disable,

};


struct sensor_power_setting hi846_power_up_setting[] = {
    //disable sec camera reset
    {
        .seq_type = SENSOR_SUSPEND,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    //SCAM AVDD 2.80V
    {
        .seq_type = SENSOR_AVDD,
        .data = (void*)"slave-sensor-avdd",
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },

    //SCAM DVDD1.2V
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_1P2V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

    //MCAM1 IOVDD 1.80V
    {
        .seq_type = SENSOR_IOVDD,
        .data = (void*)"main-sensor-iovdd",
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

     //MCAM1 AFVDD 2.85V
    {
        .seq_type = SENSOR_VCM_AVDD,
        .data = (void*)"cameravcm-vcc",
        .config_val = LDO_VOLTAGE_V2P85V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

    {
        .seq_type = SENSOR_VCM_PWDN,
        .config_val = SENSOR_GPIO_HIGH,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

    //MCAM1 AVDD 2.80V
    {
        .seq_type = SENSOR_AVDD2,
        .data = (void*)"main-sensor-avdd",
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

    //MCAM1 DVDD 1.05V
    {
        .seq_type = SENSOR_DVDD2,
        .config_val = LDO_VOLTAGE_1P2V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },

    //MCAM XSHUTDOWN RST
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_HIGH,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
};

struct sensor_power_setting hi846_power_down_setting[] = {
    {
        .seq_type = SENSOR_RST,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    {
        .seq_type = SENSOR_MCLK,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    //SCAM DVDD1.2V
    {
        .seq_type = SENSOR_DVDD2,
        .config_val = LDO_VOLTAGE_1P2V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    //MCAM1 AVDD 2.80V
    {
        .seq_type = SENSOR_AVDD2,
        .data = (void*)"main-sensor-avdd",
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    //MCAM1 VCM Enable
    {
        .seq_type = SENSOR_VCM_PWDN,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    //MCAM1 AFVDD 2.85V
    {
        .seq_type = SENSOR_VCM_AVDD,
        .data = (void*)"cameravcm-vcc",
        .config_val = LDO_VOLTAGE_V2P85V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    //MCAM1 IOVDD 1.80V
    {
        .seq_type = SENSOR_IOVDD,
        .data = (void*)"main-sensor-iovdd",
        .config_val = LDO_VOLTAGE_1P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    //SCAM DVDD1.2V
    {
        .seq_type = SENSOR_DVDD,
        .config_val = LDO_VOLTAGE_1P2V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 1,
    },
    //SCAM AVDD 2.80V
    {
        .seq_type = SENSOR_AVDD,
        .data = (void*)"slave-sensor-avdd",
        .config_val = LDO_VOLTAGE_V2P8V,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },
    //disable sec camera reset
    {
        .seq_type = SENSOR_SUSPEND,
        .config_val = SENSOR_GPIO_LOW,
        .sensor_index = SENSOR_INDEX_INVALID,
        .delay = 0,
    },
};

struct mutex hi846_power_lock;
atomic_t volatile hi846_powered = ATOMIC_INIT(0);
static sensor_t s_hi846 =
{
    .intf = { .vtbl = &s_hi846_vtbl, },
    .power_setting_array = {
            .size = ARRAY_SIZE(hi846_power_up_setting),
            .power_setting = hi846_power_up_setting,
     },
    .power_down_setting_array = {
            .size = ARRAY_SIZE(hi846_power_down_setting),
            .power_setting = hi846_power_down_setting,
    },
    .p_atpowercnt = &hi846_powered,
};

static const struct of_device_id s_hi846_dt_match[] =
{
    {
        .compatible = "huawei,hi846",
        .data = &s_hi846.intf,
    },
    {
    },
};

MODULE_DEVICE_TABLE(of, s_hi846_dt_match);

static struct platform_driver s_hi846_driver =
{
    .driver =
    {
        .name = "huawei,hi846",
        .owner = THIS_MODULE,
        .of_match_table = s_hi846_dt_match,
    },
};

char const* hi846_get_name(hwsensor_intf_t* si)
{
    sensor_t* sensor = I2S(si);
    printk(KERN_ALERT "%s enter.\n", __func__);
    return sensor->board_info->name;
}

int hi846_power_up(hwsensor_intf_t* si)
{
    int ret = 0;
    sensor_t* sensor = NULL;
    sensor = I2S(si);

    /* printk(KERN_ALERT "%s enter.\n", __func__); */
    printk(KERN_ALERT "enter %s. index = %d name = %s\n", __func__, sensor->board_info->sensor_index, sensor->board_info->name);
    /* printk(KERN_ALERT "hi846_match_id %d\n", 11); */

    ret = hw_sensor_power_up_config(s_hi846.dev, sensor->board_info);
    if (0 == ret ){
        printk(KERN_ALERT "%s. power up config success.\n", __func__);
    }else{
        printk(KERN_ALERT "%s. power up config fail.\n", __func__);
        return ret;
    }
    if (hw_is_fpga_board()) {
        ret = do_sensor_power_on(sensor->board_info->sensor_index, sensor->board_info->name);
    } else {
        ret = hw_sensor_power_up(sensor);
    }
    if (0 == ret )
    {
        printk(KERN_ALERT "%s. power up sensor success.\n", __func__);
    }
    else
    {
        printk(KERN_ALERT "%s. power up sensor fail.\n", __func__);
    }
    return ret;
}

int hi846_power_down(
        hwsensor_intf_t* si)
{
	int ret = 0;
	sensor_t* sensor = NULL;
	sensor = I2S(si);
	printk(KERN_ALERT "%s enter.\n", __func__);
	/* printk(KERN_ALERT "enter %s. index = %d name = %s\n", __func__, sensor->board_info->sensor_index, sensor->board_info->name); */
	if (hw_is_fpga_board()) {
		ret = do_sensor_power_off(sensor->board_info->sensor_index, sensor->board_info->name);
	} else {
		ret = hw_sensor_power_down(sensor);
	}
    if (0 == ret )
    {
        printk(KERN_ALERT "%s. power down sensor success.\n", __func__);
    }
    else
    {
        printk(KERN_ALERT "%s. power down sensor fail.\n", __func__);
    }
    hw_sensor_power_down_config(sensor->board_info);
    return ret;
}

int hi846_csi_enable(hwsensor_intf_t* si)
{
    printk(KERN_ALERT "%s enter.\n", __func__);
    return 0;
}

int hi846_csi_disable(hwsensor_intf_t* si)
{
    printk(KERN_ALERT "%s enter.\n", __func__);
    return 0;
}

int hi846_match_id(
        hwsensor_intf_t* si, void * data)
{
    sensor_t* sensor = I2S(si);
    struct sensor_cfg_data *cdata = (struct sensor_cfg_data *)data;

    /* printk(KERN_ALERT "hi846_match_id %d\n", 11); */
    /* printk(KERN_ALERT "%s enter.\n", __func__); */
    printk(KERN_ALERT "%s enter.", __func__);

	strncpy(cdata->cfg.name, sensor_dts_name, strlen(sensor_dts_name)+1);
    cdata->data = sensor->board_info->sensor_index;
	printk(KERN_ALERT "enter %s, name = %s index = %d\n",__func__, cdata->cfg.name, cdata->data);
    return 0;
}



int hi846_config(hwsensor_intf_t* si, void  *argp)
{
    struct sensor_cfg_data *data;

    int ret =0;
    printk(KERN_ALERT "%s enter.\n", __func__);

	if (NULL == si || NULL == argp){
		printk(KERN_ALERT "%s : si or argp is null\n", __func__);
		return -1;
	}

    data = (struct sensor_cfg_data *)argp;
    printk(KERN_ALERT "hi846 cfgtype = %d\n",data->cfgtype);
    switch(data->cfgtype){
        case SEN_CONFIG_POWER_ON:
            mutex_lock(&hi846_power_lock);
            if(false == power_on_status){
                ret = si->vtbl->power_up(si);
                if (ret == 0) {
                    power_on_status = true;
                }
            }
            /*lint -e455 -esym(455,*)*/
            mutex_unlock(&hi846_power_lock);
            /*lint -e455 +esym(455,*)*/
            break;
        case SEN_CONFIG_POWER_OFF:
            mutex_lock(&hi846_power_lock);
            if(true == power_on_status){
                ret = si->vtbl->power_down(si);
                if (ret != 0) {
                    printk(KERN_ALERT "%s. power_down fail.\n", __func__);
                }
                power_on_status = false;
            }
            /*lint -e455 -esym(455,*)*/
            mutex_unlock(&hi846_power_lock);
            /*lint -e455 +esym(455,*)*/
            break;
        case SEN_CONFIG_WRITE_REG:
            break;
        case SEN_CONFIG_READ_REG:
            break;
        case SEN_CONFIG_WRITE_REG_SETTINGS:
            break;
        case SEN_CONFIG_READ_REG_SETTINGS:
            break;
        case SEN_CONFIG_ENABLE_CSI:
            //ret = si->vtbl->csi_enable(si);
            break;
        case SEN_CONFIG_DISABLE_CSI:
            //ret = si->vtbl->csi_disable(si);
            break;
        case SEN_CONFIG_MATCH_ID:
            ret = si->vtbl->match_id(si,argp);
            break;
        default:
            printk(KERN_ALERT "%s cfgtype(%d) is error.\n", __func__, data->cfgtype);
            break;
    }

    return ret;
}

static int32_t hi846_platform_probe(struct platform_device* pdev)
{
    int rc;
    rc = 0;
    if (NULL == pdev){
        printk(KERN_ALERT "%s pdev is null.\n", __func__);
        return -1;
    }
	cam_notice("enter %s",__func__);
	if (pdev->dev.of_node) {
		rc = hw_sensor_get_dt_data(pdev, &s_hi846);
		if (rc < 0) {
			printk(KERN_ALERT "%s failed line %d\n", __func__, __LINE__);
			goto hi846_sensor_probe_fail;
        }
    } else {
        printk(KERN_ALERT "%s hi846 of_node is NULL.\n", __func__);
        goto hi846_sensor_probe_fail;
    }
    s_hi846.dev = &pdev->dev;
    mutex_init(&hi846_power_lock);
    rc = hwsensor_register(pdev, &s_hi846.intf);
    rc = rpmsg_sensor_register(pdev, (void*)&s_hi846);
hi846_sensor_probe_fail:
    return rc;
}

static int __init hi846_init_module(void)
{
    printk(KERN_ALERT "Enter: %s\n", __func__);
    return platform_driver_probe(&s_hi846_driver,
            hi846_platform_probe);
}

static void __exit hi846_exit_module(void)
{
    rpmsg_sensor_unregister((void*)&s_hi846);
    hwsensor_unregister(&s_hi846.intf);
    platform_driver_unregister(&s_hi846_driver);
}

/*lint -restore*/
/*lint -e826 -e31 -e485 -e785 -e731 -e846 -e514 -e866 -e30 -e84 -e838 -e64 -e528 -e753 -e749 +esym(826, 31, 485, 785, 731, 846, 514, 866, 30, 84, 838, 64, 528, 753, 749, 715*)*/
/*lint -e528 -esym(528,*)*/
module_init(hi846_init_module);
module_exit(hi846_exit_module);
/*lint -e528 +esym(528,*)*/
/*lint -e753 -esym(753,*)*/
MODULE_DESCRIPTION("hi846");
MODULE_LICENSE("GPL v2");
/*lint -e753 +esym(753,*)*/
