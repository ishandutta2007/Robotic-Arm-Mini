#include "soft_i2c.h"

I2C_HandleTypeDef hi2c0 =
    {
        .Instance = I2C_SOFT
    };

void delay_xus(__IO uint32_t nTime)
{
    int old_val, new_val, val;

    if (nTime > 900)
    {
        for (old_val = 0; old_val < nTime / 900; old_val++)
        {
            delay_xus(900);
        }
        nTime = nTime % 900;
    }

    old_val = SysTick->VAL;
    new_val = old_val - CPU_FREQUENCY_MHZ * nTime;
    if (new_val >= 0)
    {
        do
        {
            val = SysTick->VAL;
        } while ((val < old_val) && (val >= new_val));
    } else
    {
        new_val += CPU_FREQUENCY_MHZ * 1000;
        do
        {
            val = SysTick->VAL;
        } while ((val <= old_val) || (val > new_val));

    }
}

//--------------------------------------------
void SDA_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = MYI2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MYI2C_SDA_PORT, &GPIO_InitStruct);
}


void SDA_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = MYI2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MYI2C_SDA_PORT, &GPIO_InitStruct);
}

void SCL_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = MYI2C_SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MYI2C_SCL_PORT, &GPIO_InitStruct);
}


void SCL_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = MYI2C_SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(MYI2C_SCL_PORT, &GPIO_InitStruct);
}


void Soft_I2C_Init(void)
{
    SCL_Output();
    SDA_Output();
    SCL_Dout_HIGH();
    SDA_Dout_HIGH();
}


// Generate IIC start signal

void Soft_I2C_Start(void)
{
    SDA_Output();
    SDA_Dout_HIGH();
    SCL_Dout_HIGH();
    Delay_us(4);
    SDA_Dout_LOW();
    Delay_us(4);
    SCL_Dout_LOW();
}


// Generate IIC stop signal

void Soft_I2C_Stop(void)
{
    SDA_Output();
    SCL_Dout_LOW();
    SDA_Dout_LOW();
    Delay_us(4);
    SCL_Dout_HIGH();
    SDA_Dout_HIGH();
    Delay_us(4);
}

uint8_t Soft_I2C_Wait_Ack(void)
{
    uint8_t ucErrTime = 0;
    SDA_Input();
    SDA_Dout_HIGH();
    Delay_us(1);
    SCL_Dout_HIGH();
    Delay_us(1);
    while (SDA_Data_IN())
    {
        ucErrTime++;
        if (ucErrTime > 250)
        {
            Soft_I2C_Stop();
            return 1;
        }
    }
    SCL_Dout_LOW();// Clock output 0
    return 0;
}

// Generate ACK response

void Soft_I2C_Ack(void)
{
    SCL_Dout_LOW();
    SDA_Output();
    SDA_Dout_LOW();
    Delay_us(2);
    SCL_Dout_HIGH();
    Delay_us(2);
    SCL_Dout_LOW();
}

// Generate NACK response

void Soft_I2C_NAck(void)
{
    SCL_Dout_LOW();
    SDA_Output();
    SDA_Dout_HIGH();
    Delay_us(2);
    SCL_Dout_HIGH();
    Delay_us(2);
    SCL_Dout_LOW();
}

// IIC sends a byte
// Returns whether the slave responds
// 1, responded
// 0, no response
void Soft_I2C_Send_Byte(uint8_t txd)
{
    uint8_t t;
    // Pull down the clock to start data transmission
    SDA_Output();
    SCL_Dout_LOW();
    for (t = 0; t < 8; t++)
    {
        SDA_Write((txd & 0x80) >> 7);
        txd <<= 1;
        Delay_us(5);   // For TEA5767, these three delays are necessary
        SCL_Dout_HIGH();
        Delay_us(5);
        SCL_Dout_LOW();
        //Delay_us(2);
    }
}

// Read 1 byte, when ack=1, send ACK, when ack=0, send nACK
uint8_t Soft_I2C_Read_Byte(uint8_t ack)
{
    unsigned char i, receive = 0;
    // SDA set as input
    SDA_Input();
    for (i = 0; i < 8; i++)
    {
        SCL_Dout_LOW();
        Delay_us(5);
        SCL_Dout_HIGH();
        receive <<= 1;
        if (SDA_Data_IN())receive++;
        Delay_us(5);
    }
    if (!ack)Soft_I2C_NAck();// Send nACK
    else Soft_I2C_Ack(); // Send ACK

    return receive;
}


void SOFT_I2C_Master_Transmit(uint8_t daddr, uint8_t *buff, uint8_t len)
{
    Soft_I2C_Start();
    Soft_I2C_Send_Byte(daddr);
    Soft_I2C_Wait_Ack();

    for (int i = 0; i < len; i++)
    {
        Soft_I2C_Send_Byte(*(buff + i));
        Soft_I2C_Wait_Ack();
    }

    Soft_I2C_Stop();
}