/****************************************Copyright (c)************************************************
**                                      [艾克姆科技]
**                                        IIKMSIK 
**                            官方店铺：https://acmemcu.taobao.com
**                            官方论坛：http://www.e930bbs.com
**                                   
**--------------File Info-----------------------------------------------------------------------------
** File name         : main.c
** Last modified Date: 2020-10-14        
** Last Version      :		   
** Descriptions      : 使用的SDK版本-SDK_17.0.2
**						
**----------------------------------------------------------------------------------------------------
** Created by        : [艾克姆]Macro Peng
** Created date      : 2019-7-27
** Version           : 1.0
** Descriptions      : 本例在[实验1-1：BLE主机工程模板]的基础上修改:本例实现了扫描功能，对扫描的设备没有进行过滤
**---------------------------------------------------------------------------------------------------*/
//引用的C库头文件
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "app_error.h"
#include "app_uart.h"
//#include "ble_db_discovery.h"
//APP定时器需要引用的头文件
#include "app_timer.h"
#include "app_util.h"
#include "bsp_btn_ble.h"
#include "ble.h"
//#include "ble_gap.h"
//SoftDevice handler configuration需要引用的头文件
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

//#include "nrf_ble_gatt.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_ble_scan.h"
//Log需要引用的头文件
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#if defined (UART_PRESENT)
#include "nrf_uart.h"
#endif
#if defined (UARTE_PRESENT)
#include "nrf_uarte.h"
#endif
#include "app_uart.h"


#define APP_BLE_CONN_CFG_TAG    1   //SoftDevice BLE配置标志                                                                   
NRF_BLE_SCAN_DEF(m_scan);           //定义名称为m_scan的扫描器实例                                             

#define UART_TX_BUF_SIZE 256       //串口发送软件缓存大小（字节数）
#define UART_RX_BUF_SIZE 256       //串口接收软件缓存大小（字节数）


unsigned char beacon_rssi_g = 0;
unsigned char beacon_mac_g[12];
unsigned char beacon_scaned_flag_g = 0;
//扫描参数
static ble_gap_scan_params_t const m_scan_param =
{
    .active        = 0x01,//使用主动扫描
    .interval      = NRF_BLE_SCAN_SCAN_INTERVAL,//扫描间隔设置为：160*0.625ms = 100ms
    .window        = NRF_BLE_SCAN_SCAN_WINDOW,//扫描窗口设置为：80*0.625ms = 50ms
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,//接收除了不是发给本机的定向广播之外的所有广播数据
    .timeout       = 0,//扫描超时时间设置为0，即扫描不超时
    .scan_phys     = BLE_GAP_PHY_1MBPS,// 1 Mbps PHY
};


//启动扫描
static void scan_start(void)
{
    ret_code_t ret;
    //启动扫描器m_scan
    ret = nrf_ble_scan_start(&m_scan);
    APP_ERROR_CHECK(ret);
    //设置指示灯
    ret = bsp_indication_set(BSP_INDICATE_SCANNING);
    APP_ERROR_CHECK(ret);
}
//应用程序扫描事件处理函数
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
	  //翻转LED指示灯D4的状态，指示扫描到设备
	  nrf_gpio_pin_toggle(LED_4);
    //判断事件类型
    switch(p_scan_evt->scan_evt_id)
    {
         //因为不过滤扫描信息，即不设置过滤条件，所以只需关注NRF_BLE_SCAN_EVT_NOT_FOUND事件
			   case NRF_BLE_SCAN_EVT_NOT_FOUND:
         {
             //定义广播报告结构体指针p_adv并指向扫描事件结构体中的p_not_found
					   ble_gap_evt_adv_report_t const * p_adv =
                               p_scan_evt->params.p_not_found;
             if(p_adv->rssi > -40)
             {
               beacon_scaned_flag_g = 1;
               beacon_rssi_g = p_adv->rssi + 100;
               memcpy(&beacon_mac_g, p_adv->peer_addr.addr, 6);
               //打印MAC地址
               NRF_LOG_INFO("MAC ADDR: %02X%02X%02X%02X%02X%02X   ",
                          p_adv->peer_addr.addr[5],
                          p_adv->peer_addr.addr[4],
                          p_adv->peer_addr.addr[3],
                          p_adv->peer_addr.addr[2],
                          p_adv->peer_addr.addr[1],
                          p_adv->peer_addr.addr[0]
                          );
               //打印RSSI
               
               NRF_LOG_INFO("RSSI: %d\r\n", p_adv->rssi);
             }
         } break;
				 //扫描超时事件
         case NRF_BLE_SCAN_EVT_SCAN_TIMEOUT:
         {
             NRF_LOG_INFO("Scan timed out.");
					   //重启扫描
             scan_start();
         } break;

         default:
             break;
    }
}
//扫描初始化
static void scan_init(void)
{
    ret_code_t          err_code;
	  //定义扫描初始化结构体变量
    nrf_ble_scan_init_t init_scan;
    //先清零，再配置
    memset(&init_scan, 0, sizeof(init_scan));
    //自动连接设置为false
    init_scan.connect_if_match = false;
	  //使用初始化结构体中的扫描参数配置扫描器，这里p_scan_param指向定义的扫描参数
	  init_scan.p_scan_param     = &m_scan_param;
    //conn_cfg_tag设置为1
    init_scan.conn_cfg_tag     = APP_BLE_CONN_CFG_TAG;
    //初始化扫描器
    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);
}

//初始化指示灯
static void leds_init(void)
{
    ret_code_t err_code;
    //初始化BSP指示灯
    err_code = bsp_init(BSP_INIT_LEDS, NULL);
    APP_ERROR_CHECK(err_code);
}

//空闲状态处理函数。如果没有挂起的日志操作，则睡眠直到下一个事件发生后唤醒系统
static void idle_state_handle(void)
{
    //处理挂起的log
	  if (NRF_LOG_PROCESS() == false)
    {
        //运行电源管理，该函数需要放到主循环里面执行
			  nrf_pwr_mgmt_run();
    }
}


static uint16_t   m_ble_uarts_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */


#define BLE_UARTS_MAX_DATA_LEN (64)


//串口事件回调函数，串口初始化时注册，该函数中判断事件类型并进行处理
void uart_event_handle(app_uart_evt_t * p_event)
{
    static uint8_t data_array[BLE_UARTS_MAX_DATA_LEN];
    static uint8_t index = 0;
    uint32_t       err_code;

    switch (p_event->evt_type)
    {
          case APP_UART_DATA_READY:    
          UNUSED_VARIABLE(app_uart_get(&data_array[index]));
            index++;
            if (((data_array[index - 1] == '\n') &&
                (data_array[index - 2] == '\r')))
            {
                if (index > 1)
                {
                  if(strncmp((const char *)"scan_beacon\r\n", (const char *)data_array, index) == 0)
                  {
                    printf("scaned\r\n");
                  }
                  else if(strncmp((const char *)"get_rssi\r\n", (const char *)data_array, index) == 0)
                  {
                    if(beacon_scaned_flag_g == 1)
                    {
                        printf("rssi %d\r\n", beacon_rssi_g);
                        beacon_scaned_flag_g = 0;
                    }
                  }
                  else if(strncmp((const char *)"get_mac\r\n", (const char *)data_array, index) == 0)
                  {
                    if(beacon_scaned_flag_g == 1)
                    {
                       printf("mac %02X%02X%02X%02X%02X%02X\r\n",
                          beacon_mac_g[5],
                          beacon_mac_g[4],
                          beacon_mac_g[3],
                          beacon_mac_g[2],
                          beacon_mac_g[1],
                          beacon_mac_g[0]
                          );
                      beacon_scaned_flag_g = 0;
                    }
                  }
                  else
                  {
                    printf("not match\r\n");
                  }
                  memset(&data_array, 0, BLE_UARTS_MAX_DATA_LEN);
                }
                index = 0;
            }
            break;
        case APP_UART_COMMUNICATION_ERROR:
            NRF_LOG_INFO("APP_UART_COMMUNICATION_ERROR \r\n");
            APP_ERROR_HANDLER(p_event->data.error_communication);
            break;
        case APP_UART_FIFO_ERROR:
            NRF_LOG_INFO("APP_UART_FIFO_ERROR \r\n");
            APP_ERROR_HANDLER(p_event->data.error_code);
            break;
        default:
            break;
    }
}
//串口配置
void uart_config(void)
{
	uint32_t err_code;
	
	//定义串口通讯参数配置结构体并初始化
  const app_uart_comm_params_t comm_params =
  {
    RX_PIN_NUMBER,//定义uart接收引脚
    TX_PIN_NUMBER,//定义uart发送引脚
    RTS_PIN_NUMBER,//定义uart RTS引脚，流控关闭后虽然定义了RTS和CTS引脚，但是驱动程序会忽略，不会配置这两个引脚，两个引脚仍可作为IO使用
    CTS_PIN_NUMBER,//定义uart CTS引脚
    APP_UART_FLOW_CONTROL_DISABLED,//关闭uart硬件流控
    false,//禁止奇偶检验
    NRF_UART_BAUDRATE_115200//uart波特率设置为115200bps
  };
  //初始化串口，注册串口事件回调函数
  APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_event_handle,
                         APP_IRQ_PRIORITY_LOWEST,
                         err_code);

  APP_ERROR_CHECK(err_code);
	
}

//初始化BLE协议栈
static void ble_stack_init(void)
{
    ret_code_t err_code;
    //请求使能SoftDevice，该函数中会根据sdk_config.h文件中低频时钟的设置来配置低频时钟
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);
    
    //定义保存应用程序RAM起始地址的变量
    uint32_t ram_start = 0;
	  //使用sdk_config.h文件的默认参数配置协议栈，获取应用程序RAM起始地址，保存到变量ram_start
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    //使能BLE协议栈
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
}

//初始化电源管理模块
static void power_management_init(void)
{
    ret_code_t err_code;
	  //初始化电源管理
    err_code = nrf_pwr_mgmt_init();
	  //检查函数返回的错误代码
    APP_ERROR_CHECK(err_code);
}

static void log_init(void)
{
    //初始化log程序模块
	  ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    //设置log输出终端（根据sdk_config.h中的配置设置输出终端为UART或者RTT）
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

//初始化APP定时器模块
static void timers_init(void)
{
    //初始化APP定时器模块
    ret_code_t err_code = app_timer_init();
	  //检查返回值
    APP_ERROR_CHECK(err_code);

    //加入创建用户定时任务的代码，创建用户定时任务。 
}


//主函数
int main()
{
		//初始化log，因为本例使用串口输出扫描到的设备信息，因此log中不要配置UART为输出终端
		log_init();
		timers_init();
	  //初始化串口
		uart_config();
	
	  //初始化指示灯
	  leds_init();
    //初始化电源管理
    power_management_init();
	  //初始化协议栈
    ble_stack_init();

    //初始化扫描
    scan_init();

    printf("Scanner started.\r\n");
    NRF_LOG_INFO("Log test \r\n");
	  //启动扫描
    scan_start();
	
	  for (;;)
    {
        //处理挂起的LOG和运行电源管理
			  idle_state_handle();
    }
}

