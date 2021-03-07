#include "stdlib.h"
#include "oled.h"
#include "oledfont.h"

extern I2C_HandleTypeDef hi2c4;


#define Max_Column  128
#define Max_Row     64
#define Reverse    1
#define No_Reverse 0

#define OLED_ADDRESS 0x78
#define OLED_CMD  0x00 //写命令
#define OLED_DATA 0x40 //写数据


rt_uint8_t OLED_Init_CMD[] =
{0xAE,0x00,0x10,0x40,0xB0,0x81,0xFF,0xA1,0xA6,0xA8,\
0x3F,0xC8,0xD3,0x38,0xD5,0x80,0xD8,0x05,0xD9,0xF1,\
0xDA,0x12,0xDB,0x30,0x8D,0x14,0xAF,0x20,0x00
};




rt_uint8_t OLED_Frame_Buffer[8][128]={0};
rt_uint8_t Is_Init = 0x10; // 高四位 是 是否进入刷新

int I2C_WriteReg( rt_uint8_t* WriteBuf, rt_uint8_t SlaveAddr, rt_uint8_t WriteAddr, rt_uint16_t NumByte )
{
    HAL_StatusTypeDef ret = HAL_I2C_Mem_Write(&hi2c4,SlaveAddr,WriteAddr,I2C_MEMADD_SIZE_8BIT,WriteBuf,NumByte,100);
    if (ret != 0x00)  { //I2C 故障处理
    HAL_I2C_DeInit(&hi2c4); // 释放 IO 口为 GPIO ，复位句柄状态标志
    HAL_I2C_Init(&hi2c4); // 这句重新初始化 I2C 控制器
    }
}

void OLED_Init(void)
{ // OLED初始化函数， 主要是初始化一些参数
    rt_uint8_t i;
    Is_Init = 0x10;
    for(i=0;i<29;i++)
    {   // 顺序写入这些命令完成初始化函数
        I2C_WriteReg(&OLED_Init_CMD[i], 0x78,0x00,1); // 初始化OLED
    }
    Is_Init = 0x00;
}

void OLED_Refresh(void)
{   // OLED刷新函数, 手动的将 1024个byte传递给GDDRAM
    I2C_WriteReg(OLED_Frame_Buffer[0], 0x78,0x40,1024);
}

void OLED_ShowChar(rt_uint8_t x,rt_uint8_t y,rt_uint8_t chr,rt_uint8_t SIZE,rt_uint8_t Is_Reverse)
{//字符显示函数，这里是依次的将字符的字库信息转存到显示Buffer里面
    unsigned char c=0,i=0;
    extern rt_uint8_t OLED_Frame_Buffer[8][128];
    extern const rt_uint8_t F6x8[][6];
    c=chr-' ';//得到偏移后的值
    if(SIZE ==8)
    {
        if(Is_Reverse==0)
        {
            for(i=0;i<6;i++)
            OLED_Frame_Buffer[y][x+i] = F6x8[c][i] ;
        }else
        {
            for(i=0;i<6;i++)
            OLED_Frame_Buffer[y][x+i] = ~F6x8[c][i] ;
            }
    }else if(SIZE == 16) // 8*16
    {
        if(Is_Reverse == 0)
        {
            for(i=0;i<8;i++)
            {
                OLED_Frame_Buffer[y  ][x+i] = F8X16[c*16+i  ];
                OLED_Frame_Buffer[y+1][x+i] = F8X16[c*16+i+8];
            }
        }else
        {
            for(i=0;i<8;i++)
            {
                OLED_Frame_Buffer[y  ][x+i] = ~F8X16[c*16+i  ];
                OLED_Frame_Buffer[y+1][x+i] = ~F8X16[c*16+i+8];
            }
        }

    }
    else if ( SIZE == 32)
    {
        for(int j=0; j<4 ;j++)
        {
//            OLED_Set_Pos(x, y + j);
            for (i = 0; i < 16 ; i++)
            {
//                OLED_WR_Byte(F16X32[c * 64 + i + j*16], OLED_DATA);
                OLED_Frame_Buffer[y+j][x+i] = F16X32[c*64+i+j*16];
//                OLED_Frame_Buffer[x+i][y+j] = F16X32[c*64+i+j*16];


            }
        }
    }
    else
    {
        rt_kprintf("Unsupported font size...., please set Char_size to 16 or 32.");
        return;
    }
}

void OLED_Draw_Rectangle(rt_uint8_t x1, rt_uint8_t y1, rt_uint8_t x2, rt_uint8_t y2)
{// 画矩形的函数， 本质就是画4条线
    OLED_Draw_Line(x1,y1,x2,y1);
    OLED_Draw_Line(x1,y1,x1,y2);
    OLED_Draw_Line(x1,y2,x2,y2);
    OLED_Draw_Line(x2,y1,x2,y2);
}


void OLED_Draw_Circle(rt_uint16_t x0,rt_uint16_t y0,rt_uint8_t r)
{ //画圆的函数, 引自正点原子的官方例程
    int a,b;
    int di;
    a=0;b=r;
    di=3-(r<<1);             //判断下个点位置的标志
    while(a<=b)
    {
        OLED_Draw_Dot(x0+a,y0-b);             //5
        OLED_Draw_Dot(x0+b,y0-a);             //0
        OLED_Draw_Dot(x0+b,y0+a);             //4
        OLED_Draw_Dot(x0+a,y0+b);             //6
        OLED_Draw_Dot(x0-a,y0+b);             //1
        OLED_Draw_Dot(x0-b,y0+a);
        OLED_Draw_Dot(x0-a,y0-b);             //2
    OLED_Draw_Dot(x0-b,y0-a);             //7
        a++;
        //使用Bresenham算法画圆
        if(di<0)di +=4*a+6;
        else
        {
            di+=10+4*(a-b);
            b--;
        }
    }
}

void OLED_Draw_Line(rt_uint8_t x1,rt_uint8_t y1,rt_uint8_t x2,rt_uint8_t y2)
{ //画线的函数， 引自正点原子的官方例程
    rt_uint16_t t;
    int xerr=0,yerr=0,delta_x,delta_y,distance;
    int incx,incy,uRow,uCol;
    delta_x=x2-x1; //计算坐标增量
    delta_y=y2-y1;
    uRow=x1;
    uCol=y1;
    if(delta_x>0)incx=1; //设置单步方向
    else if(delta_x==0)incx=0;//垂直线
    else {incx=-1;delta_x=-delta_x;}
    if(delta_y>0)incy=1;
    else if(delta_y==0)incy=0;//水平线
    else{incy=-1;delta_y=-delta_y;}
    if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴
    else distance=delta_y;
    for(t=0;t<=distance+1;t++ )//画线输出
    {
        OLED_Draw_Dot(uRow,uCol);//画点
        xerr+=delta_x ;
        yerr+=delta_y ;
        if(xerr>distance)
        {
            xerr-=distance;
            uRow+=incx;
        }
        if(yerr>distance)
        {
            yerr-=distance;
            uCol+=incy;
        }
    }
}
void OLED_Draw_Dot(rt_uint8_t x,rt_uint8_t y)
{ // 画点函数 ，自己写的 本质就是将点的坐标转换成数组里面 指定数据的指定位
    OLED_Frame_Buffer[y/8][x] |= 1<<(y%8);
}
void OLED_Clear(void)
{ // 清屏函数，将数组元素全部写0
    memset(OLED_Frame_Buffer,0,sizeof(OLED_Frame_Buffer));
    OLED_Refresh();
}

unsigned int oled_pow(rt_uint8_t m,rt_uint8_t n)
{
    unsigned int result=1;
    while(n--)result*=m;
    return result;
}

void OLED_Show_Num(rt_uint8_t x, rt_uint8_t y,int Num,rt_uint8_t len,rt_uint8_t SIZE)
{ // 显示数字函数， 判断正负号以后 将剩下的正整数部分反向逐步解算出来并且将 新数据转存到数组里
    if(SIZE <=8)
    {
        if(Num<0)
        {
            OLED_ShowChar(x,y,'-',8,No_Reverse);
            Num *=-1;// 相较于官方的代码 添加了显示负数的支持
        }
        while (len)
        {
                OLED_ShowChar(x+6*len,y,(Num%10)+'0',8,No_Reverse);
                Num/=10;// 此处直接反向更新数据相比于官方的计算方案
                len--;  // 减少了一定的运算量,缺点是第一位如果是0 也会显示
                      //
        }
    }else
    {
        if(Num<0)
        {
            OLED_ShowChar(x,y,'-',16,No_Reverse);
            Num *=-1;
        }
        while (len)
        {
                OLED_ShowChar(x+8*len,y,(Num%10)+'0',16,No_Reverse);
                Num/=10;
                len--;
        }
    }
}


//不能直接显示负数，但是空位不显示0
void OLED_ShowNum(uint8_t x,uint8_t y,unsigned int num,uint8_t len,uint8_t size2)
{
    uint8_t t,temp;
    uint8_t enshow=0;
    for(t=0;t<len;t++)
    {
        temp=(num/oled_pow(10,len-t-1))%10;
        if(enshow==0&&t<(len-1))
        {
            if(temp==0)
            {
                OLED_ShowChar(x+(size2/2)*t,y,' ',size2,0);
                continue;
            }else enshow=1;

        }
        OLED_ShowChar(x+(size2/2)*t,y,temp+'0',size2,0);
    }
}

void OLED_Show_Num_Reverse(rt_uint8_t x, rt_uint8_t y,int Num,rt_uint8_t len,rt_uint8_t SIZE)
{ //反白显示函数， 相比较于上一个函数，写入的数字按位取反了
    if(SIZE <=8)
    {
        if(Num<0)
        {
            OLED_ShowChar(x,y,'-',8,Reverse);
            Num *=-1;
        }
        while (len)
        {
                OLED_ShowChar(x+6*len,y,(Num%10)+'0',8,Reverse);
                Num/=10;
                len--;
        }
    }else
    {
        if(Num<0)
        {
            OLED_ShowChar(x,y,'-',16,0);
            Num *=-1;
        }
        while (len)
        {
                OLED_ShowChar(x+8*len,y,(Num%10)+'0',16,Reverse);
                Num/=10;
                len--;
        }
    }
}

void OLED_ShowString(rt_uint8_t x,rt_uint8_t y,rt_uint8_t* str,rt_uint8_t SIZE)
{ // 字符串显示函数，依次写入每一个字符
    rt_uint8_t i=0;
    if(SIZE <=8)
    {
        while (str[i]!='\0')
        {       OLED_ShowChar(x,y,str[i],8,No_Reverse);
                x+=6;
                i++;
        }
    }else if(SIZE == 16)
    {
        while (str[i]!='\0')
        {       OLED_ShowChar(x,y,str[i],16,No_Reverse);
                x+=8;
                i++;
        }
    }
    else if(SIZE == 32){
        while (str[i]!='\0')
        {       OLED_ShowChar(x,y,str[i],32,No_Reverse);
                x+=16;
                i++;
        }
    }

}

void OLED_ShowCHinese(uint8_t x,uint8_t y,uint8_t no)
{
    uint8_t t;

    for(t=0;t<16;t++)
        {
        OLED_Frame_Buffer[y][x+t]=Hzk[2*no][t];
     }
    for(t=0;t<16;t++)
            {
        OLED_Frame_Buffer[y+1][x+t]=Hzk[2*no+1][t];
      }
}




void OLED_WR_CMD(rt_uint8_t CMD)
{ // OLED 写指令 的操作方式 : 依次发送 0x78 0x40 CMD
    I2C_WriteReg(&CMD, 0x78,0x00,1);
}
void OLED_WR_Dat(rt_uint8_t CMD)
    { // OLED 写数据的方式 : 依次发送 0x78 0x00 dat1 dat2 .... datn
    I2C_WriteReg(&CMD, 0x78,0x40,1);
}
void OLED_Set_Y(rt_uint8_t y) // 行刷新控制
{ // 单行刷新是的程序  用不到了
    OLED_WR_CMD(0xb0+y);
}




