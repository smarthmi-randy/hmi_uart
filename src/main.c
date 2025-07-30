#include <zephyr/kernel.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include "hmi_uart.h" // 包含新的 HMI UART 模組頭文件

LOG_MODULE_REGISTER(main_app, LOG_LEVEL_DBG);

#if DT_HAS_ALIAS(uart30) //在這邊使用實際Device Tree所定義的別名
#define HMI_UART_NODE30 DT_ALIAS(uart30)
#else
#error "請在設備樹中定義 uart30 別名！"
#endif

#if DT_HAS_ALIAS(uart21) //在這邊使用實際Device Tree所定義的別名
#define HMI_UART_NODE21 DT_ALIAS(uart21)
#else
#error "請在設備樹中定義 uart21 別名！"
#endif

//宣告 HMI UART 實例的數據結構和消息隊列
K_MSGQ_DEFINE(my_uart_rx_msgq1, HMI_UART_RX_MSG_SIZE, 5, 4); // 為此實例定義自己的消息佇列
struct hmi_uart_data my_uart_instance_data1 = {.dev = DEVICE_DT_GET(HMI_UART_NODE30), .rx_msgq = &my_uart_rx_msgq1};

K_MSGQ_DEFINE(my_uart_rx_msgq2, HMI_UART_RX_MSG_SIZE, 5, 4); // 為此實例定義自己的消息佇列
struct hmi_uart_data my_uart_instance_data2 = {.dev = DEVICE_DT_GET(HMI_UART_NODE21), .rx_msgq = &my_uart_rx_msgq2};

void command_parser_thread(void *p1, void *p2, void *p3)
{
    struct k_msgq *rx_msgq = my_uart_instance_data1.rx_msgq;
    char rx_buf[HMI_UART_RX_MSG_SIZE];

    while (true) {
        if (k_msgq_get(rx_msgq, rx_buf, K_MSEC(100)) == 0) {
            LOG_INF("從 UART 接收到數據: \"%s\"", rx_buf);
            // 這裡可以添加您的命令解析邏輯
            if (strcmp(rx_buf, "hello\r") == 0) {
                hmi_uart_send(my_uart_instance_data1.dev, (const uint8_t *)"World!\r\n", strlen("World!\r\n"));
            } else if (strcmp(rx_buf, "info\r") == 0) {
                hmi_uart_send(my_uart_instance_data1.dev, (const uint8_t *)"Zephyr HMI UART Example\r\n", strlen("Zephyr HMI UART Example\r\n"));
            } else {
                hmi_uart_send(my_uart_instance_data1.dev, (const uint8_t *)"Unknown command.\r\n", strlen("Unknown command.\r\n"));
            }
        }
    }
}

// 定義一個 K_THREAD_STACK_DEFINE 以避免在函數中分配堆疊
K_THREAD_STACK_DEFINE(command_parser_stack, 1024);
struct k_thread command_parser_thread_data;

int main(void)
{
    int ret;

    // 初始化 HMI UART 實例，傳遞設備指針、波特率、實例數據和消息佇列
    ret = hmi_uart_init_instance(&my_uart_instance_data1, 115200);
    if (ret != 0) {
        LOG_ERR("HMI UART:%s 實例初始化失敗，錯誤碼: %d", my_uart_instance_data1.dev->name, ret);
        return 1;
    }

    // 初始化 HMI UART 實例，傳遞設備指針、波特率、實例數據和消息佇列
    ret = hmi_uart_init_instance(&my_uart_instance_data2, 115200);
    if (ret != 0) {
        LOG_ERR("HMI UART:%s 實例初始化失敗，錯誤碼: %d", my_uart_instance_data1.dev->name, ret);
        return 1;
    }

    // 創建命令解析線程
    k_thread_create(&command_parser_thread_data, command_parser_stack,
                    K_THREAD_STACK_SIZEOF(command_parser_stack),
                    command_parser_thread, NULL, NULL, NULL,
                    K_PRIO_COOP(7), 0, K_NO_WAIT);

    LOG_INF("HMI UART 範例應用程式啟動！");
    return 0;
}