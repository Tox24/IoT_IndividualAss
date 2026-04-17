#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define TAG "TAG"
#define PI 3.14

QueueHandle_t xSharedQueue;

#define TABLE_SIZE 256 //256 values in the queue
uint8_t sineTable[TABLE_SIZE];

const int amplitude = 100;
const int offset = 128;
const float signal_freq = 5.0; 
//int sampling_freq = 1000; //useless if using a lookup_table

const uint8_t DEFAULT_SENSOR_VALUE = 0;

/* This task samples from the queue at the desired rate*/
void vSamplerTask(void *args) {
    uint8_t static readValue;
    BaseType_t ret;

    for (;;) {
        ret = xQueuePeek(xSharedQueue, &readValue, portMAX_DELAY);

        if (ret == pdPASS) {
            printf(">dac:%d\n", readValue);
        } else {
            printf("Sampler TASK: <ERROR reading value from the queue>.\n");
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* A task running on another core wrt vSamplerTask.
   Waits for the complete calculation of the lookup-table storing sine values.

   Then it puts the value at the desidered rate in the queue.
*/
void vProducerTask(void *args) {
    BaseType_t ret;
    uint8_t *value;

    for (;;) {
        for (int i = 0; i < TABLE_SIZE; i++) {
            value = sineTable + i;
            ret = xQueueOverwrite(xSharedQueue, value);

            if (ret != pdPASS) {
                printf("Producer TASK: <An error occourred during value writing on the queue>.\n");
            }

            printf("Producer TASK: <Written value: %u>\n", *value);

            vTaskDelay(pdMS_TO_TICKS(10)); //the value will change every tot
        }
        /* Producer frequency:
        *   delay: 1s,
        *   operation delay: negligible,
        *   steps: 256,
        *   frequency = 1/256s = 0.0039 Hz = 3.9 mHz
        *   
        *   else
        * 
        *   delay: 0.01s,
        *   steps: 256,
        *   frequency = 1/2.56s
        */
    }
}

void app_main(void) {
    BaseType_t ret;

    xSharedQueue = xQueueCreate(1, sizeof(uint8_t)); //I'll simulate the pin with a shared queue containing the current status

    if (xSharedQueue == NULL) {
        printf("SETUP: <An error occourred while creating the queue>\n");
        while(1) {vTaskDelay(100);}
    }

    ret = xQueueSend(xSharedQueue, &DEFAULT_SENSOR_VALUE, 0); //Set the queue default value to 0

    if (ret != pdPASS) {
        printf("SETUP: <An error occourred setting the default queue value>\n");
        while(1) {vTaskDelay(100);}
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        sineTable[i] = (uint8_t) (offset + amplitude * sin(2.0 * M_PI * i / TABLE_SIZE) + amplitude/3 * sin(4.0 * M_PI * i / TABLE_SIZE)); //signal of the form s = O + A * sin(2*pi * freq * index)
    }

    printf("SETUP: <Sine table generated>\n");

    ret = xTaskCreatePinnedToCore(vProducerTask, "Producer", 2048, NULL, 1, NULL, 1);

    if (ret != pdPASS) {
        printf("SETUP: <Impossible to create Producer task: %d>\n", ret);
        while(1) {vTaskDelay(100);}
    }

    ret = xTaskCreatePinnedToCore(vSamplerTask, "Sampler", 2048, NULL, 1, NULL, 0);

    if (ret != pdPASS) {
        printf("SETUP: <Impossible to create Sampler task: %d>\n", ret);
        while(1) {vTaskDelay(100);}
    }
}
