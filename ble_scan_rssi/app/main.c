/****************************************Copyright (c)************************************************
**                                      [����ķ�Ƽ�]
**                                        IIKMSIK 
**                            �ٷ����̣�https://acmemcu.taobao.com
**                            �ٷ���̳��http://www.e930bbs.com
**                                   
**--------------File Info-----------------------------------------------------------------------------
** File name         : main.c
** Last modified Date: 2020-10-14        
** Last Version      :		   
** Descriptions      : ʹ�õ�SDK�汾-SDK_17.0.2
**						
**----------------------------------------------------------------------------------------------------
** Created by        : [����ķ]Macro Peng
** Created date      : 2019-7-27
** Version           : 1.0
** Descriptions      : ������[ʵ��1-1��BLE��������ģ��]�Ļ������޸�:����ʵ����ɨ�蹦�ܣ���ɨ����豸û�н��й���
**---------------------------------------------------------------------------------------------------*/
//���õ�C��ͷ�ļ�
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "nordic_common.h"
#include "app_error.h"
#include "app_uart.h"
//#include "ble_db_discovery.h"
//APP��ʱ����Ҫ���õ�ͷ�ļ�
#include "app_timer.h"
#include "app_util.h"
#include "bsp_btn_ble.h"
#include "ble.h"
//#include "ble_gap.h"
//SoftDevice handler configuration��Ҫ���õ�ͷ�ļ�
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

//#include "nrf_ble_gatt.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_ble_scan.h"
//Log��Ҫ���õ�ͷ�ļ�
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


#define APP_BLE_CONN_CFG_TAG    1   //SoftDevice BLE���ñ�־                                                                   
NRF_BLE_SCAN_DEF(m_scan);           //��������Ϊm_scan��ɨ����ʵ��                                             

#define UART_TX_BUF_SIZE 256       //���ڷ�����������С���ֽ�����
#define UART_RX_BUF_SIZE 256       //���ڽ�����������С���ֽ�����


unsigned char beacon_rssi_g = 0;
unsigned char beacon_mac_g[12];
unsigned char beacon_scaned_flag_g = 0;
//ɨ�����
static ble_gap_scan_params_t const m_scan_param =
{
    .active        = 0x01,//ʹ������ɨ��
    .interval      = NRF_BLE_SCAN_SCAN_INTERVAL,//ɨ��������Ϊ��160*0.625ms = 100ms
    .window        = NRF_BLE_SCAN_SCAN_WINDOW,//ɨ�贰������Ϊ��80*0.625ms = 50ms
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,//���ճ��˲��Ƿ��������Ķ���㲥֮������й㲥����
    .timeout       = 0,//ɨ�賬ʱʱ������Ϊ0����ɨ�費��ʱ
    .scan_phys     = BLE_GAP_PHY_1MBPS,// 1 Mbps PHY
};


//����ɨ��
static void scan_start(void)
{
    ret_code_t ret;
    //����ɨ����m_scan
    ret = nrf_ble_scan_start(&m_scan);
    APP_ERROR_CHECK(ret);
    //����ָʾ��
    ret = bsp_indication_set(BSP_INDICATE_SCANNING);
    APP_ERROR_CHECK(ret);
}
//Ӧ�ó���ɨ���¼�������
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
	  //��תLEDָʾ��D4��״̬��ָʾɨ�赽�豸
	  nrf_gpio_pin_toggle(LED_4);
    //�ж��¼�����
    switch(p_scan_evt->scan_evt_id)
    {
         //��Ϊ������ɨ����Ϣ���������ù�������������ֻ���עNRF_BLE_SCAN_EVT_NOT_FOUND�¼�
			   case NRF_BLE_SCAN_EVT_NOT_FOUND:
         {
             //����㲥����ṹ��ָ��p_adv��ָ��ɨ���¼��ṹ���е�p_not_found
					   ble_gap_evt_adv_report_t const * p_adv =
                               p_scan_evt->params.p_not_found;
             if(p_adv->rssi > -40)
             {
               beacon_scaned_flag_g = 1;
               beacon_rssi_g = p_adv->rssi + 100;
               memcpy(&beacon_mac_g, p_adv->peer_addr.addr, 6);
               //��ӡMAC��ַ
               NRF_LOG_INFO("MAC ADDR: %02X%02X%02X%02X%02X%02X   ",
                          p_adv->peer_addr.addr[5],
                          p_adv->peer_addr.addr[4],
                          p_adv->peer_addr.addr[3],
                          p_adv->peer_addr.addr[2],
                          p_adv->peer_addr.addr[1],
                          p_adv->peer_addr.addr[0]
                          );
               //��ӡRSSI
               
               NRF_LOG_INFO("RSSI: %d\r\n", p_adv->rssi);
             }
         } break;
				 //ɨ�賬ʱ�¼�
         case NRF_BLE_SCAN_EVT_SCAN_TIMEOUT:
         {
             NRF_LOG_INFO("Scan timed out.");
					   //����ɨ��
             scan_start();
         } break;

         default:
             break;
    }
}
//ɨ���ʼ��
static void scan_init(void)
{
    ret_code_t          err_code;
	  //����ɨ���ʼ���ṹ�����
    nrf_ble_scan_init_t init_scan;
    //�����㣬������
    memset(&init_scan, 0, sizeof(init_scan));
    //�Զ���������Ϊfalse
    init_scan.connect_if_match = false;
	  //ʹ�ó�ʼ���ṹ���е�ɨ���������ɨ����������p_scan_paramָ�����ɨ�����
	  init_scan.p_scan_param     = &m_scan_param;
    //conn_cfg_tag����Ϊ1
    init_scan.conn_cfg_tag     = APP_BLE_CONN_CFG_TAG;
    //��ʼ��ɨ����
    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);
}

//��ʼ��ָʾ��
static void leds_init(void)
{
    ret_code_t err_code;
    //��ʼ��BSPָʾ��
    err_code = bsp_init(BSP_INIT_LEDS, NULL);
    APP_ERROR_CHECK(err_code);
}

//����״̬�����������û�й������־��������˯��ֱ����һ���¼���������ϵͳ
static void idle_state_handle(void)
{
    //��������log
	  if (NRF_LOG_PROCESS() == false)
    {
        //���е�Դ�����ú�����Ҫ�ŵ���ѭ������ִ��
			  nrf_pwr_mgmt_run();
    }
}


static uint16_t   m_ble_uarts_max_data_len = BLE_GATT_ATT_MTU_DEFAULT - 3;            /**< Maximum length of data (in bytes) that can be transmitted to the peer by the Nordic UART service module. */


#define BLE_UARTS_MAX_DATA_LEN (64)


//�����¼��ص����������ڳ�ʼ��ʱע�ᣬ�ú������ж��¼����Ͳ����д���
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
//��������
void uart_config(void)
{
	uint32_t err_code;
	
	//���崮��ͨѶ�������ýṹ�岢��ʼ��
  const app_uart_comm_params_t comm_params =
  {
    RX_PIN_NUMBER,//����uart��������
    TX_PIN_NUMBER,//����uart��������
    RTS_PIN_NUMBER,//����uart RTS���ţ����عرպ���Ȼ������RTS��CTS���ţ����������������ԣ������������������ţ����������Կ���ΪIOʹ��
    CTS_PIN_NUMBER,//����uart CTS����
    APP_UART_FLOW_CONTROL_DISABLED,//�ر�uartӲ������
    false,//��ֹ��ż����
    NRF_UART_BAUDRATE_115200//uart����������Ϊ115200bps
  };
  //��ʼ�����ڣ�ע�ᴮ���¼��ص�����
  APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_event_handle,
                         APP_IRQ_PRIORITY_LOWEST,
                         err_code);

  APP_ERROR_CHECK(err_code);
	
}

//��ʼ��BLEЭ��ջ
static void ble_stack_init(void)
{
    ret_code_t err_code;
    //����ʹ��SoftDevice���ú����л����sdk_config.h�ļ��е�Ƶʱ�ӵ����������õ�Ƶʱ��
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);
    
    //���屣��Ӧ�ó���RAM��ʼ��ַ�ı���
    uint32_t ram_start = 0;
	  //ʹ��sdk_config.h�ļ���Ĭ�ϲ�������Э��ջ����ȡӦ�ó���RAM��ʼ��ַ�����浽����ram_start
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    //ʹ��BLEЭ��ջ
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);
}

//��ʼ����Դ����ģ��
static void power_management_init(void)
{
    ret_code_t err_code;
	  //��ʼ����Դ����
    err_code = nrf_pwr_mgmt_init();
	  //��麯�����صĴ������
    APP_ERROR_CHECK(err_code);
}

static void log_init(void)
{
    //��ʼ��log����ģ��
	  ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);
    //����log����նˣ�����sdk_config.h�е�������������ն�ΪUART����RTT��
    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

//��ʼ��APP��ʱ��ģ��
static void timers_init(void)
{
    //��ʼ��APP��ʱ��ģ��
    ret_code_t err_code = app_timer_init();
	  //��鷵��ֵ
    APP_ERROR_CHECK(err_code);

    //���봴���û���ʱ����Ĵ��룬�����û���ʱ���� 
}


//������
int main()
{
		//��ʼ��log����Ϊ����ʹ�ô������ɨ�赽���豸��Ϣ�����log�в�Ҫ����UARTΪ����ն�
		log_init();
		timers_init();
	  //��ʼ������
		uart_config();
	
	  //��ʼ��ָʾ��
	  leds_init();
    //��ʼ����Դ����
    power_management_init();
	  //��ʼ��Э��ջ
    ble_stack_init();

    //��ʼ��ɨ��
    scan_init();

    printf("Scanner started.\r\n");
    NRF_LOG_INFO("Log test \r\n");
	  //����ɨ��
    scan_start();
	
	  for (;;)
    {
        //��������LOG�����е�Դ����
			  idle_state_handle();
    }
}

