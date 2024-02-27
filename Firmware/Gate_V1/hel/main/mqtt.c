/**
 * @file mqtt.c
 * @author Vanperdung (dung.nv382001@gmail.com)
 * @brief
 * @version 0.1
 * @date 2023-02-09
 *
 * @copyright Copyright (c) 2023
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"

#include "nvs_flash.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_smartconfig.h"
#include "mqtt_client.h"
#include "esp_spiffs.h"
#include "esp_http_client.h"
#include "esp_attr.h"
#include "esp_http_server.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/spi_master.h"

#include "sx1278.h"
#include "uart_user.h"
#include "button.h"
#include "common.h"
#include "led.h"
#include "mqtt.h"
#include "smartcfg.h"
#include "wifi_sta.h"
#include "wifi_ap.h"
#include "webserver.h"

extern Data_Struct_t Node_Table[MAX_NODE];
extern gateway_mode_t mode;
extern gateway_cfg_mode_t cfg_mode;
static const char *TAG = "MQTT";
int i = 0;
#define REJECT "{"method":"reject_node","params":{}}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
    {
        // mode = NORMAL;
        if (i == 0)
        {
            char data1[50],data2[50];
            for (int l = 0; l < MAX_NODE; l++)
            {
                sprintf(data1, "{\"queue%d\":%02X}", l, 0x00);
                esp_mqtt_client_publish(client, TOPIC, data1, 0, 1, 0);
            }
            for (int l = 0; l < MAX_NODE; l++)
            {
                sprintf(data2, "{\"list%d\":%02X}", l, 0x00);
                esp_mqtt_client_publish(client, TOPIC, data2, 0, 1, 0);
            }
        i++;
        }
        esp_mqtt_client_subscribe(client, "v1/devices/me/attributes", 1);
        esp_mqtt_client_subscribe(client, "v1/devices/me/rpc/request/+", 1);
        //ESP_LOGI(TAG, "MQTT event connected");
        break;
    }
    case MQTT_EVENT_DISCONNECTED:
        mode = LOCAL;
        ESP_LOGI(TAG, "MQTT event disconnected");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT event subcribed, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT event unsubcribed, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        //ESP_LOGI(TAG, "MQTT event published, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
    {
        memset(Handler, '\0', sizeof(Handler));

        //ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        //ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        memcpy(Handler, event->data, event->data_len);
       // ESP_LOGI(TAG, "DATA: %s", Handler);
        int index = 0;
        char str[11];
        for (int i = 11; i <= 21; i++)
        {
            str[index] = Handler[i];
            index++;
        }
      //  ESP_LOGI("HELLO", "%s", str);
        if (strcmp(str, "accept_node") == 0)
        {
           // ESP_LOGI("DONE", "DONE ACCEPT");
           // ESP_LOGI("DONE", "%d", sizeof_node());
            Node_Table[sizeof_node()].Link.Node_ID = xQueue[Sizeof_Queue_Current(xQueue) - 1].data;
           // ESP_LOGI("DONE", "%02X", Node_Table[0].Link.Node_ID);
            Pop_Queue(xQueue);
            int i;
            char data2[50],data1[50];
            // for (i = 0; i < sizeof_node(); i++)
            // {
               // ESP_LOGI(TAG, "PUSH LIST");
                sprintf(data2, "{\"list%d\":%02X}", sizeof_node()-1, Node_Table[sizeof_node()-1].Link.Node_ID);
                esp_mqtt_client_publish(client, TOPIC, data2, 0, 1, 0);
            // }

            // for (int k = sizeof_node(); k < MAX_NODE; k++)
            // {
            //     sprintf(data2, "{\"list%d\":%02X}", k, 0x00);
            //     esp_mqtt_client_publish(client, TOPIC, data2, 0, 1, 0);
            // }
            // int j;
            // for (j = 0; j < Sizeof_Queue_Current(xQueue); j++)
            // {
            //   //  ESP_LOGI(TAG, "PUSH LIST");
            //     sprintf(data1, "{\"queue%d\":%02X}", j, xQueue[j].data);
            //     esp_mqtt_client_publish(client, TOPIC, data1, 0, 1, 0);
            // }

            // for (int l = Sizeof_Queue_Current(xQueue); l < MAX_NODE; l++)
            // {
                sprintf(data1, "{\"queue%d\":%02X}",Sizeof_Queue_Current(xQueue), 0x00);
                esp_mqtt_client_publish(client, TOPIC, data1, 0, 1, 0);
            // }
            // ResponsePacket_t resp = {PACKET_ID_2, link_payload.Node_ID, NORMAL_MODE, 180U, (uint16_t)(((LINK_ACCEPT << 8) & 0xFF00) | LINK_ACK)};
            // if (listen_before_talk() == false)
            // {
            //     sx1278_start_recv_data();
            //     break;
            // }
            // sx1278_send_data(&resp, sizeof(ResponsePacket_t));
            // ESP_LOGI("HELLO", "Done2!\n");
            // flagmqtt = 0 ;
        }
        else if( strcmp(str, "reject_node") == 0 )
        {
            //ESP_LOGI("DONE", "DONE REJECT");
            Push_Queue(xReject_Queue,0,xQueue[Sizeof_Queue_Current(xQueue)-1].data,0);
          //  ESP_LOGI(TAG,"%02X    %d",xQueue[Sizeof_Queue_Current(xQueue)-1].data,Sizeof_Queue_Current(xQueue));
           // ESP_LOGI(TAG,"QUEUE_REJ %02X : %d",xReject_Queue[Sizeof_Queue_Current(xReject_Queue)-1].data,Sizeof_Queue_Current(xReject_Queue));
            Pop_Queue(xQueue);
            int j;
            char data1[50];
            // for (j = 0; j < Sizeof_Queue_Current(xQueue); j++)
            // {
            //    // ESP_LOGI(TAG, "PUSH LIST");
            //     sprintf(data1, "{\"queue%d\":%02X}", j, xQueue[j].data);
            //     esp_mqtt_client_publish(client, TOPIC, data1, 0, 1, 0);
            // }

            // for (int l = Sizeof_Queue_Current(xQueue); l < MAX_NODE; l++)
            // {
                sprintf(data1, "{\"queue%d\":%02X}",Sizeof_Queue_Current(xQueue), 0x00);
                esp_mqtt_client_publish(client, TOPIC, data1, 0, 1, 0);
            // }
        }
        else if (  strcmp(str,"rejlst_node") == 0U )
        {
            Node_Table[sizeof_node()-1].Link.Node_ID = 0U;
            int i;
            char data2[50],data1[50];
            // for (i = 0; i < sizeof_node(); i++)
            // {
            //     //ESP_LOGI(TAG, "PUSH LIST");
            //     sprintf(data2, "{\"list%d\":%02X}", i, Node_Table[i].Link.Node_ID);
            //     esp_mqtt_client_publish(client, TOPIC, data2, 0, 1, 0);
            // }

            // for (int k = sizeof_node(); k < MAX_NODE; k++)
            // {
                sprintf(data2, "{\"list%d\":%02X}",sizeof_node() , 0x00);
                esp_mqtt_client_publish(client, TOPIC, data2, 0, 1, 0);
            // }
        }
        break;
    }
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT event error");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_task(void *param)
{
    char *mess_recv = NULL;
    size_t mess_size = 0;
    mqtt_obj_t mqtt_obj;
    while (1)
    {
        mess_recv = (char *)xRingbufferReceive(mqtt_ring_buf, &mess_size, portMAX_DELAY);
        if (mess_recv)
        {
            mess_recv[mess_size] = '\0';
            ESP_LOGI(TAG, "Recv payload: %s", mess_recv);
            memset(&mqtt_obj, 0, sizeof(mqtt_obj));
            mqtt_parse_data(mess_recv, &mqtt_obj);

            vRingbufferReturnItem(mqtt_ring_buf, (void *)mess_recv);
        }
    }
}

void mqtt_client_sta(void)
{
    uint8_t broker[50] = {0};
    ESP_LOGI(TAG, "MQTT init");
    // ESP_LOGI(TAG, "Broker: %s", MQTT_BROKER);
    // sprintf((char *)broker, "mqtt://%s", MQTT_BROKER);
    // mqtt_ring_buf = xRingbufferCreate(4096, RINGBUF_TYPE_NOSPLIT);
    // if (mqtt_ring_buf == NULL)
    //     ESP_LOGE(TAG, "Failed to create ring buffer");
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = Server,
        //    .keepalive = 60,
        .username = USERNAME,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    // xTaskCreate(&mqtt_task, "mqtt_task", 4096, NULL, 9, NULL);
}
