/**
 * CT117E-M4 / GPIO - I2C
*/

#include "i2c_hal.h"

#define DELAY_TIME	20

// SDA设置为输入模式
void SDA_Input_Mode()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = GPIO_PIN_7;
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
}

// SDA设置为输出模式
void SDA_Output_Mode()
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = GPIO_PIN_7;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
}

// SDA输出电平
void SDA_Output( uint16_t val )
{
    if ( val )
    {
        GPIOB->BSRR |= GPIO_PIN_7;
    }
    else
    {
        GPIOB->BRR |= GPIO_PIN_7;
    }
}

// SCL输出电平
void SCL_Output( uint16_t val )
{
    if ( val )
    {
        GPIOB->BSRR |= GPIO_PIN_6;
    }
    else
    {
        GPIOB->BRR |= GPIO_PIN_6;
    }
}

// 读取SDA输入电平
uint8_t SDA_Input(void)
{
	if(HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_SET){
		return 1;
	}else{
		return 0;
	}
}

// 延时函数
static void delay1(unsigned int n)
{
    uint32_t i;
    for ( i = 0; i < n; ++i);
}

// I2C起始信号
void I2CStart(void)
{
    SDA_Output(1);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SDA_Output(0);
    delay1(DELAY_TIME);
    SCL_Output(0);
    delay1(DELAY_TIME);
}

// I2C停止信号
void I2CStop(void)
{
    SCL_Output(0);
    delay1(DELAY_TIME);
    SDA_Output(0);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SDA_Output(1);
    delay1(DELAY_TIME);

}

// 等待ACK应答
unsigned char I2CWaitAck(void)
{
    unsigned short cErrTime = 5;
    SDA_Input_Mode();
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    while(SDA_Input())
    {
        cErrTime--;
        delay1(DELAY_TIME);
        if (0 == cErrTime)
        {
            SDA_Output_Mode();
            I2CStop();
            return ERROR;
        }
    }
    SCL_Output(0);
    SDA_Output_Mode();
    delay1(DELAY_TIME);
    return SUCCESS;
}

// 发送ACK应答
void I2CSendAck(void)
{
    SDA_Output(0);
    delay1(DELAY_TIME);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SCL_Output(0);
    delay1(DELAY_TIME);

}

// 发送非ACK应答
void I2CSendNotAck(void)
{
    SDA_Output(1);
    delay1(DELAY_TIME);
    delay1(DELAY_TIME);
    SCL_Output(1);
    delay1(DELAY_TIME);
    SCL_Output(0);
    delay1(DELAY_TIME);

}

// 发送一个字节
void I2CSendByte(unsigned char cSendByte)
{
    unsigned char  i = 8;
    while (i--)
    {
        SCL_Output(0);
        delay1(DELAY_TIME);
        SDA_Output(cSendByte & 0x80);
        delay1(DELAY_TIME);
        cSendByte += cSendByte;
        delay1(DELAY_TIME);
        SCL_Output(1);
        delay1(DELAY_TIME);
    }
    SCL_Output(0);
    delay1(DELAY_TIME);
}

// 接收一个字节
unsigned char I2CReceiveByte(void)
{
    unsigned char i = 8;
    unsigned char cR_Byte = 0;
    SDA_Input_Mode();
    while (i--)
    {
        cR_Byte += cR_Byte;
        SCL_Output(0);
        delay1(DELAY_TIME);
        delay1(DELAY_TIME);
        SCL_Output(1);
        delay1(DELAY_TIME);
        cR_Byte |=  SDA_Input();
    }
    SCL_Output(0);
    delay1(DELAY_TIME);
    SDA_Output_Mode();
    return cR_Byte;
}

// I2C初始化
void I2CInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = GPIO_PIN_7 | GPIO_PIN_6;
    GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStructure.Pull = GPIO_PULLUP;
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void I2CStart(void);
void I2CStop(void);
unsigned char I2CWaitAck(void);
void I2CSendAck(void);
void I2CSendNotAck(void);
void I2CSendByte(unsigned char cSendByte);
unsigned char I2CReceiveByte(void);
void I2CInit(void);


/**
 * @brief 向EEPROM写入数据
 *
 * @param EEPROM_String 指向要写入EEPROM的数据的指针
 * @param addr EEPROM内存中的起始地址
 * @param num 要写入的数据字节数
 */
void eeprom_write(uint8_t *EEPROM_String, uint8_t addr, uint8_t num)
{
    I2CStart();               // 启动I2C通信
    I2CSendByte(0xa0);        // 发送EEPROM写命令（器件地址 + 写操作位）
    I2CWaitAck();             // 等待EEPROM的ACK信号

    I2CSendByte(addr);        // 发送要写入的EEPROM起始地址
    I2CWaitAck();             // 等待EEPROM的ACK信号

    while(num--)              // 逐字节写入数据
    {
        I2CSendByte(*EEPROM_String++);  // 发送当前字节的数据
        I2CWaitAck();                   // 等待EEPROM的ACK信号
        delay1(200);                    // 短暂延时，确保数据写入
    }
    I2CStop();                // 停止I2C通信
    HAL_Delay(5);             // 延时5ms，确保数据完全写入EEPROM
}

/**
 * @brief 从EEPROM读取数据
 *
 * @param EEPROM_String 指向存储读取数据的缓冲区的指针
 * @param addr EEPROM内存中的起始地址
 * @param num 要读取的数据字节数
 */
void eeprom_read(uint8_t *EEPROM_String, uint8_t addr, uint8_t num)
{
    I2CStart();               // 启动I2C通信
    I2CSendByte(0xa0);        // 发送EEPROM写命令（器件地址 + 写操作位）
    I2CWaitAck();             // 等待EEPROM的ACK信号

    I2CSendByte(addr);        // 发送要读取的EEPROM起始地址
    I2CWaitAck();             // 等待EEPROM的ACK信号

    I2CStart();               // 再次启动I2C通信，准备读数据
    I2CSendByte(0xa1);        // 发送EEPROM读命令（器件地址 + 读操作位）
    I2CWaitAck();             // 等待EEPROM的ACK信号

    while(num--)              // 逐字节读取数据
    {
        *EEPROM_String++ = I2CReceiveByte();  // 接收当前字节的数据
        if(num)
            I2CSendAck();      // 如果还需要继续读取，发送ACK信号
        else
            I2CSendNotAck();   // 如果已读取完毕，发送非ACK信号
    }
    I2CStop();                // 停止I2C通信
}

/**
 * @brief 向MCP4017写入数据
 *
 * @param data 要写入MCP4017的数值
 */
void mcp4017_write(uint8_t data)
{
    I2CStart();               // 启动I2C通信
    I2CSendByte(0x5e);        // 发送MCP4017的写命令（器件地址 + 写操作位）
    I2CWaitAck();             // 等待MCP4017的ACK信号
    I2CSendByte(data);        // 发送要写入的数据
    I2CWaitAck();             // 等待MCP4017的ACK信号
    I2CStop();                // 停止I2C通信
}

/**
 * @brief 从MCP4017读取数据
 *
 * @param data 指向存储读取数据的变量的指针
 */
void mcp4017_read(uint8_t *data)
{
    I2CStart();               // 启动I2C通信
    I2CSendByte(0x5f);        // 发送MCP4017的读命令（器件地址 + 读操作位）
    I2CWaitAck();             // 等待MCP4017的ACK信号
    *data = I2CReceiveByte(); // 接收来自MCP4017的数据
    I2CSendNotAck();          // 发送非ACK信号，表示读取完成
    I2CStop();                // 停止I2C通信
}
