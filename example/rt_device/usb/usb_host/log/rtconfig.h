/*Auto generated configuration*/
#ifndef RT_CONFIG_H
#define RT_CONFIG_H

#define KCONFIG_V2 1
#define BSP_USING_RTTHREAD 1
#define RT_USING_COMPONENTS_INIT 1
#define RT_USING_USER_MAIN 1
#define RT_MAIN_THREAD_STACK_SIZE 2048
#define RT_MAIN_THREAD_PRIORITY 10
#define RT_USING_FINSH 1
#define FINSH_THREAD_NAME "tshell"
#define FINSH_USING_HISTORY 1
#define FINSH_HISTORY_LINES 5
#define FINSH_USING_SYMTAB 1
#define FINSH_USING_DESCRIPTION 1
#define FINSH_THREAD_PRIORITY 20
#define FINSH_THREAD_STACK_SIZE 4096
#define FINSH_CMD_SIZE 80
#define FINSH_USING_MSH 1
#define FINSH_USING_MSH_DEFAULT 1
#define FINSH_ARG_MAX 10
#define RT_USING_DFS 1
#define DFS_USING_WORKDIR 1
#define DFS_FILESYSTEMS_MAX 2
#define DFS_FILESYSTEM_TYPES_MAX 2
#define DFS_FD_MAX 16
#define RT_USING_DFS_ELMFAT 1
#define RT_DFS_ELM_CODE_PAGE 437
#define RT_DFS_ELM_WORD_ACCESS 1
#define RT_DFS_ELM_USE_LFN_3 1
#define RT_DFS_ELM_USE_LFN 3
#define RT_DFS_ELM_MAX_LFN 255
#define RT_DFS_ELM_DRIVES 2
#define RT_DFS_ELM_MAX_SECTOR_SIZE 4096
#define RT_DFS_ELM_REENTRANT 1
#define RT_DFS_ELM_DHARA_ENABLED 1
#define RT_USING_DFS_DEVFS 1
#define RT_USING_DEVICE_IPC 1
#define RT_PIPE_BUFSZ 512
#define RT_USING_SERIAL 1
#define RT_SERIAL_USING_DMA 1
#define RT_SERIAL_RB_BUFSZ 64
#define RT_SERIAL_DEFAULT_BAUDRATE 1000000
#define RT_USING_HWTIMER 1
#define RT_USING_I2C 1
#define RT_USING_I2C_BITOPS 1
#define RT_USING_PIN 1
#define RT_USING_ADC 1
#define RT_USING_PWM 1
#define RT_USING_MTD_NAND 1
#define RT_USING_RTC 1
#define RT_USING_SPI 1
#define RT_USING_AUDIO 1
#define RT_USING_USB_HOST 1
#define RT_USBH_RNDIS 1
#define RT_USBH_RNDIS_DEV 1
#define RT_USBD_THREAD_STACK_SZ 4096
#define RT_USING_LIBC 1
#define RT_USING_POSIX 1
#define RT_USING_NETDEV 1
#define NETDEV_USING_IFCONFIG 1
#define NETDEV_USING_PING 1
#define NETDEV_USING_NETSTAT 1
#define NETDEV_USING_AUTO_DEFAULT 1
#define RT_USING_LWIP 1
#define RT_USING_LWIP202 1
#define RT_LWIP_IGMP 1
#define RT_LWIP_ICMP 1
#define RT_LWIP_DNS 1
#define RT_LWIP_DHCP 1
#define IP_SOF_BROADCAST 1
#define IP_SOF_BROADCAST_RECV 1
#define RT_LWIP_IPADDR "192.168.1.30"
#define RT_LWIP_GWADDR "192.168.1.1"
#define RT_LWIP_MSKADDR "255.255.255.0"
#define RT_LWIP_UDP 1
#define RT_LWIP_TCP 1
#define RT_LWIP_RAW 1
#define RT_MEMP_NUM_NETCONN 8
#define RT_LWIP_PBUF_NUM 16
#define RT_LWIP_RAW_PCB_NUM 4
#define RT_LWIP_UDP_PCB_NUM 4
#define RT_LWIP_TCP_PCB_NUM 4
#define RT_LWIP_TCP_SEG_NUM 40
#define RT_LWIP_TCP_SND_BUF 8196
#define RT_LWIP_TCP_WND 8196
#define RT_LWIP_TCPTHREAD_PRIORITY 10
#define RT_LWIP_TCPTHREAD_MBOX_SIZE 8
#define RT_LWIP_TCPTHREAD_STACKSIZE 1024
#define RT_LWIP_ETHTHREAD_PRIORITY 12
#define RT_LWIP_ETHTHREAD_STACKSIZE 1024
#define RT_LWIP_ETHTHREAD_MBOX_SIZE 8
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK 1
#define SO_REUSE 1
#define LWIP_SO_RCVTIMEO 1
#define LWIP_SO_SNDTIMEO 1
#define LWIP_SO_RCVBUF 1
#define LWIP_NETIF_LOOPBACK 0
#define RT_LWIP_USING_PING 1
#define RT_LWIP_USING_HTTP 1
#define RT_NAME_MAX 8
#define RT_ALIGN_SIZE 4
#define RT_THREAD_PRIORITY_32 1
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 1000
#define RT_USING_OVERFLOW_CHECK 1
#define IDLE_THREAD_STACK_SIZE 512
#define RT_USING_TIMER_SOFT 1
#define RT_TIMER_THREAD_PRIO 4
#define RT_TIMER_THREAD_STACK_SIZE 2048
#define RT_DEBUG 1
#define RT_USING_SEMAPHORE 1
#define RT_USING_MUTEX 1
#define RT_USING_EVENT 1
#define RT_USING_MAILBOX 1
#define RT_USING_MESSAGEQUEUE 1
#define RT_USING_MEMPOOL 1
#define RT_USING_SMALL_MEM 1
#define RT_USING_HEAP 1
#define RT_USING_DEVICE 1
#define RT_USING_CONSOLE 1
#define RT_CONSOLEBUF_SIZE 256
#define RT_CONSOLE_DEVICE_NAME "uart1"
#define RT_VER_NUM 0x30103
#define BSP_USING_FULL_ASSERT 1
#define ROM_ATT_BUF_SIZE 3084
#define ROM_LOG_SIZE 4096
#define BT_DUAL_HOST_MEM 1
#define MB_PORT 1
#define UART_PORT1 1
#define UART_PORT1_PORT "uart4"
#define USING_PARTITION_TABLE 1
#define OPT_LEVEL_SIZE 1
#define OPT_LEVEL "-Oz"
#define KCONFIG_BOARD_V2 1
#define SOC_SF32LB52X 1
#define BF0_HCPU 1
#define CORE "HCPU"
#define CPU "Cortex-M33"
#define LXT_FREQ 32768
#define LXT_LP_CYCLE 200
#define BT_TX_POWER_VAL 1
#define BT_TX_POWER_VAL_MAX 10
#define BT_TX_POWER_VAL_MIN 0
#define BT_TX_POWER_VAL_INIT 0
#define SF32LB52X_REV_AUTO 1
#define CFG_SUPPORT_NON_OTP 1
#define BSP_USING_GPIO 1
#define BSP_GPIO_HANDLE 2
#define BSP_USING_DMA 1
#define BSP_USING_UART 1
#define BSP_USING_UART1 1
#define BSP_UART1_RX_USING_DMA 1
#define BSP_USING_SPI 1
#define BSP_USING_I2C 1
#define BSP_USING_I2C1 1
#define BSP_USING_I2C4 1
#define BSP_USING_TIM 1
#define BSP_USING_BTIM1 1
#define BSP_USING_LPTIM1 1
#define BSP_USING_PWM 1
#define BSP_USING_PWM3 1
#define BSP_USING_ADC 1
#define BSP_USING_MPI 1
#define BSP_USING_SPI_FLASH 1
#define BSP_ENABLE_MPI1 1
#define BSP_ENABLE_QSPI1 1
#define BSP_MPI1_MODE_3 1
#define BSP_QSPI1_MODE 3
#define BSP_USING_PSRAM1 1
#define BSP_QSPI1_MEM_SIZE 8
#define BSP_ENABLE_MPI2 1
#define BSP_ENABLE_QSPI2 1
#define BSP_MPI2_MODE_1 1
#define BSP_QSPI2_MODE 1
#define BSP_QSPI2_USING_DMA 1
#define BSP_USING_NAND_FLASH2 1
#define BSP_QSPI2_MEM_SIZE 128
#define BSP_USING_SPI_NAND 1
#define BSP_USING_EXT_DMA 1
#define BSP_USING_HW_AES 1
#define BSP_USING_PSRAM 1
#define MEMCPY_NON_DMA 1
#define PSRAM_CACHE_WB 1
#define BSP_USING_USBH 1
#define BSP_USING_ONCHIP_RTC 1
#define BSP_RTC_PPM 0
#define BSP_USING_EPIC 1
#define BSP_USING_LCDC 1
#define BSP_USING_PINMUX 1
#define BSP_USING_LCPU_PATCH 1
#define BSP_ENABLE_AUD_PRC 1
#define BSP_AUDPRC_TX0_DMA 1
#define BSP_AUDPRC_RX0_DMA 1
#define BSP_ENABLE_AUD_CODEC 1
#define BSP_USING_LCD 1
#define LCD_GC9B71_VSYNC_ENABLE 1
#define LCD_USING_ED_LB5XSPI18501 1
#define LCD_HOR_RES_MAX 320
#define LCD_VER_RES_MAX 386
#define LCD_DPI 315
#define BSP_USING_TOUCHD 1
#define TOUCH_DEVICE_NAME "i2c1"
#define TSC_USING_CST816 1
#define LCD_USING_GC9B71 1
#define BSP_LCDC_USING_QADSPI 1
#define LCD_USING_PWM_AS_BACKLIGHT 1
#define BSP_USING_RECT_TYPE_LCD 1
#define BSP_USING_KEY1 1
#define BSP_KEY1_PIN 34
#define BSP_KEY1_ACTIVE_HIGH 1
#define PA_USING_AW8155 1
#define CONSOLE_UART1 1
#define BSP_USING_LED1 1
#define BSP_LED1_PIN 25
#define BSP_LED1_ACTIVE_HIGH 1
#define BSP_USING_BOARD_EH_LB525XXX 1
#define ASIC 1
#define TOUCH_IRQ_PIN 43
#define LCD_PWM_BACKLIGHT_INTERFACE_NAME "pwm3"
#define LCD_PWM_BACKLIGHT_CHANEL_NUM 4
#define LCD_BACKLIGHT_CONTROL_PIN 1
#define CUSTOM_MEM_MAP 1
#endif
