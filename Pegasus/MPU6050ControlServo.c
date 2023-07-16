/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <unistd.h>
#include <hi_io.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "iot_gpio.h"
#include "iot_gpio_ex.h"
#include "iot_watchdog.h"
#include "iot_pwm.h"
#include "hi_time.h"
#include "mpu6050.h"

#define IOT_GPIO_VERTICAL 6
#define IOT_GPIO_HORIZENTAL 7
#define OLED_I2C_BAUDRATE (400 * 1000) // 400k
#define MPU6050_RA_WHO_AM_I 0x75
#define MPU6050_ADDRESS 0x68
#define STACK_SIZE 1024

struct args_to_child_thread {
    int angle;
};
static struct args_to_child_thread args_child;

typedef struct {
    /** Pointer to the buffer storing data to send */
    unsigned char *sendBuf;
    /** Length of data to send */
    unsigned int  sendLen;
    /** Pointer to the buffer for storing data to receive */
    unsigned char *receiveBuf;
    /** Length of data received */
    unsigned int  receiveLen;
} IotI2cData;

/***************************************************************
 * 函数功能: 通过I2C读取一段寄存器内容存放到指定的缓冲区内
 * 输入参数: Addr：I2C设备地址
 *           Reg：目标寄存器
 *           RegSize：寄存器尺寸(8位或者16位)
 *           pBuffer：缓冲区指针
 *           Length：缓冲区长度
 * 返 回 值: HAL_StatusTypeDef：操作结果
 * 说    明: 无
 **************************************************************/
static int MPU6050ReadBuffer(uint8_t Reg, uint8_t *pBuffer, uint16_t Length)
{
    uint32_t ret = 0;
    IotI2cData mpu6050_i2c_data = { 0 };
    uint8_t buffer[1] = { Reg };
    mpu6050_i2c_data.sendBuf = buffer;
    mpu6050_i2c_data.sendLen = 1;
    mpu6050_i2c_data.receiveBuf = pBuffer;
    mpu6050_i2c_data.receiveLen = Length;
    ret = hi_i2c_writeread(0, (MPU6050_ADDRESS << 1) | 0x00, &mpu6050_i2c_data);
    if (ret != 0) {
        printf("===== Error: I2C writeread ret = 0x%x! =====\r\n", ret);
        return -1;
    }
    return 0;
}

/***************************************************************
 * 函数功能: 从MPU6050寄存器读取数据
 * 输入参数: 无
 * 返 回 值: 无
 * 说    明: 无
 ***************************************************************/
static int MPU6050ReadData(uint8_t reg_add, unsigned char *read, uint8_t num)
{
    return MPU6050ReadBuffer(reg_add, read, num);
}

/***************************************************************
 * 函数功能: 读取MPU6050的ID
 * 输入参数: 无
 * 返 回 值: 无
 * 说    明: 无
 ***************************************************************/
int MPU6050ReadID(void)
{
    unsigned char Re = 0;
    MPU6050ReadData(MPU6050_RA_WHO_AM_I, &Re, 1); // 读器件地址
    printf("%p\r\n", Re);
    if (Re != 0x68) {
        printf("MPU6050 dectected error!\r\n");
        return -1;
    } else {
        return 0;
    }
}

/***************************************************************
 * 函数功能: 通过I2C写入一个值到指定寄存器内
 * 输入参数: Addr：I2C设备地址
 *           Reg：目标寄存器
 *           Value：值
 * 返 回 值: 无
 * 说    明: 无
 **************************************************************/
static int MPU6050WriteData(uint8_t Reg, uint8_t Value)
{
    uint32_t ret;
    uint8_t send_data[MPU6050_DATA_2_BYTE] = { Reg, Value };
    ret = IoTI2cWrite(0, (MPU6050_ADDRESS << 1) | 0x00, send_data, sizeof(send_data));
    if (ret != 0) {
        printf("===== Error: I2C write ret = 0x%x! =====\r\n", ret);
        return -1;
    }
    return 0;
}

/***************************************************************
 * 函数功能: 写数据到MPU6050寄存器
 * 输入参数: 无
 * 返 回 值: 无
 * 说    明: 无
 ***************************************************************/
static void MPU6050WriteReg(uint8_t reg_add, uint8_t reg_dat)
{
    MPU6050WriteData(reg_add, reg_dat);
}

/***************************************************************
 * 函数功能: 初始化MPU6050芯片
 * 输入参数: 无
 * 返 回 值: 无
 * 说    明: 无
 ***************************************************************/
void MPU6050Init(void)
{
    MPU6050WriteReg(MPU6050_RA_PWR_MGMT_1, 0X80); // 复位MPU6050
    usleep(RESET_DELAY_US);
    MPU6050WriteReg(MPU6050_RA_PWR_MGMT_1, 0X01); // 唤醒MPU6050
    MPU6050WriteReg(MPU6050_RA_INT_ENABLE, 0X00); // 关闭所有中断
    MPU6050WriteReg(MPU6090_RA_GYRO_CONFIG, 0x18); // 配置陀螺仪量程
    MPU6050WriteReg(MPU6050_RA_DIV_FREQUENCY, 0x00); // 配置陀螺仪采样率分频寄存器
    MPU6050WriteReg(MPU6050_RA_USER_CTRL, 0X00);  // I2C主模式关闭
    MPU6050WriteReg(MPU6050_RA_FIFO_EN, 0X00);    // 关闭FIFO
    MPU6050WriteReg(MPU6050_RA_CONFIG, 0x04);       // 配置外部引脚采样和DLPF数字低通滤波器
}

/***************************************************************
 * 函数功能: 读取MPU6050的角速度数据
 * 输入参数: 无
 * 返 回 值: 无
 * 说    明: 无
 ***************************************************************/
static int MPU6050ReadGyro(short *gyroData)
{
    int ret;
    uint8_t buf[ACCEL_DATA_LEN];
    ret = MPU6050ReadData(MPU6050_GYRO_OUT, buf, ACCEL_DATA_LEN);
    if (ret != 0) {
        return -1;
    }
    gyroData[ACCEL_X_AXIS] = (buf[ACCEL_X_AXIS_LSB] << SENSOR_DATA_WIDTH_8_BIT) | buf[ACCEL_X_AXIS_MSB];
    gyroData[ACCEL_Y_AXIS] = (buf[ACCEL_Y_AXIS_LSB] << SENSOR_DATA_WIDTH_8_BIT) | buf[ACCEL_Y_AXIS_MSB];
    gyroData[ACCEL_Z_AXIS] = (buf[ACCEL_Z_AXIS_LSB] << SENSOR_DATA_WIDTH_8_BIT) | buf[ACCEL_Z_AXIS_MSB];
    return 0;
}

/***************************************************************
 * 函数功能: 将MPU6050传感器Z轴数据转换为水平舵机控制角度
 * 输入参数: MPU6050 Z轴数据
 * 返 回 值: 水平舵机控制角度
 * 说    明: 无
 ***************************************************************/
int trans_Z_horizental(short mpu6050_Z_data)
{
    int interval = 32767 / 1000;
    return mpu6050_Z_data / interval;
}

/***************************************************************
 * 函数功能: 将MPU6050传感器Y轴数据转换为竖直舵机控制角度
 * 输入参数: MPU6050 Y轴数据
 * 返 回 值: 竖直舵机控制角度
 * 说    明: 无
 ***************************************************************/
int trans_Y_vertical(short mpu6050_Y_data)
{
    int interval = 32767 / 1000;
    return mpu6050_Y_data / interval;
}

/***************************************************************
 * 函数功能: 控制水平舵机转动角度
 * 输入参数: 水平角度
 * 返 回 值: 无
 * 说    明: 无
 ***************************************************************/
void serve_control_horizental(int horizental_angle) 
{
    IoTGpioSetOutputVal(IOT_GPIO_HORIZENTAL, 1);
    hi_udelay(1500 + horizental_angle);
    IoTGpioSetOutputVal(IOT_GPIO_HORIZENTAL, 0);
    hi_udelay(18500 - horizental_angle);
}

/***************************************************************
 * 函数功能: 控制竖直舵机转动角度
 * 输入参数: 竖直角度
 * 返 回 值: 无
 * 说    明: 无
 ***************************************************************/
void serve_control_vertical(int vertical_angle) 
{
    IoTGpioSetOutputVal(IOT_GPIO_VERTICAL, 1);
    hi_udelay(1500 + vertical_angle);
    IoTGpioSetOutputVal(IOT_GPIO_VERTICAL, 0);
    hi_udelay(18500 - vertical_angle);
}

/***************************************************************
 * 函数功能: 读取MPU6050传感器位置数据，控制舵机旋转
 * 输入参数: 无
 * 返 回 值: 无
 * 说    明: 无
 ***************************************************************/
void MPU6050_control_main() {
    int status = 0;
    short gyroData[3] = {0};
    int vertical_angle = 0;
    int horizental_angle = 0;

    serve_control_horizental(0); // 初始化水平和竖直方向舵机
    serve_control_vertical(0);

    while (1) {
        status = MPU6050ReadGyro(gyroData);
        if (status != 0) {
            printf("mpu6050 read gyro data fail!\r\n");
        }

        horizental_angle = trans_Z_horizental(gyroData[ACCEL_Z_AXIS]); // 将MPU6050测量的数据转换为舵机水平和竖直控制角度
        vertical_angle = trans_Y_vertical(gyroData[ACCEL_Y_AXIS]);

        serve_control_horizental(horizental_angle); // 控制舵机旋转方向
        serve_control_vertical(vertical_angle);

        hi_udelay(100000); // 暂停0.1s
    }
}


/***************************************************************
 * 函数功能: 初始化相关设备
 * 输入参数: 无
 * 返 回 值: 无
 * 说    明: 无
 ***************************************************************/
static void StartMPU6050ControlTask(void)
{
    IoTGpioInit(IOT_GPIO_VERTICAL);
    IoSetFunc(IOT_GPIO_VERTICAL, 0); /* 将GPIO定义为普通GPIO功能*/
    IoTGpioSetDir(IOT_GPIO_VERTICAL, IOT_GPIO_DIR_OUT); /* 设置GPIO方向为输出*/
    IoTGpioSetOutputVal(IOT_GPIO_VERTICAL, IOT_GPIO_VALUE0); /* GPIO输出初始值为0*/

    IoTGpioInit(IOT_GPIO_HORIZENTAL);
    IoSetFunc(IOT_GPIO_HORIZENTAL, 0); /* 将GPIO定义为普通GPIO功能*/
    IoTGpioSetDir(IOT_GPIO_HORIZENTAL, IOT_GPIO_DIR_OUT); /* 设置GPIO方向为输出*/
    IoTGpioSetOutputVal(IOT_GPIO_HORIZENTAL, IOT_GPIO_VALUE0); /* GPIO输出初始值为0*/

    IoTGpioInit(13);
    IoSetFunc(13, 6); /* gpio13复用I2C0_SDA */
    IoTGpioSetDir(13, IOT_GPIO_DIR_OUT);
    IoTGpioInit(14); /* 初始化gpio14 */
    IoSetFunc(14, 6); /* gpio14复用I2C0_SCL */
    IoTGpioSetDir(14, IOT_GPIO_DIR_OUT);

    IoTI2cInit(0, OLED_I2C_BAUDRATE);
    IoTI2cSetBaudrate(0, OLED_I2C_BAUDRATE);

    MPU6050ReadID();  

    MPU6050Init();

    osThreadAttr_t attr;
    attr.name = "MPU6050Control_thread_main";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = STACK_SIZE;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)MPU6050_control_main, NULL, &attr) == NULL) {
        printf("[ThreadTestTask] Failed to create rtosv2_thread_main!\n");
    }

}

APP_FEATURE_INIT(StartMPU6050ControlTask);
