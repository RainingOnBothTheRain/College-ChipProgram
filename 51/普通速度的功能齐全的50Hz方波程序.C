//8位数模转换,SW1设DA数字量,ON为0,OFF为1,SW6拨左边,用万用表测T1、T6间的电压
#include <reg52.H>              //包含头文件reg52.H
#include <absacc.h>             //包含头文件absacc.h
#define D_port XBYTE [0x7FFF]   //共阴数码管段码锁存器U2端口地址
#define B_port XBYTE [0xBFFF]   //共阴数码管位码锁存器U3端口地址
#define DA     XBYTE [0xEFFF]   //DA转换芯片U8数据输入端口地址
typedef unsigned char u8;       //无符号字符型变量新表示方法定义
typedef unsigned int u16;       //无符号整型变量新表示方法定义
u8   r_kT,Ax;                   //DA数据暂存量和中间量定义
u8   buf[8];                    //显示缓冲区定义
char m_kT;                      //DA电压的10倍,-110～110,对应-11.0～11.0V
u16  temp1;                     //计算时的中间变量1
int  temp2;                     //计算时的中间变量2
float VT1;                      //DA输出电压的10倍变量定义-110.0～110.0V
u8   M=0,i=0;                   //扫描显示位计数器及自有计数器定义
u8 code Stab[16]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x77,
                  0x7c,0x39,0x5e,0x79,0x71};//0～F的共阴段码表
u8 code Btab[8]={0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};//共阴位码表
/***********T0T1初始化***********/
void  T0T1_init(void)           //8个数码管,每个显示时间为扫描显示周期的1/8
{ TMOD=0x21;                    //T0为16位定时、T1为8位自动重装定时
  SCON=0x40;                    //8位UART,波特率可变(2^1*T1溢出率/32)
  PCON=0x80;                    //使能波特率倍速位SMOD
  TH0=0xfb;                     //高字节,DA刷新周期10ms的时间常数fb80H
  TL0=0x80;                     //低字节,(65536-fb80H)/(11.0592/12)=1250微秒
  TH1=0xfd;                     //T1定时值,(2^SMOD)*307200/32=19200bps
  TL1=0xfd;                     //波特率=2*307200/32=19200bps
  EA=1;                         //允许中断
  ET0=1;                        //允许T0中断
  TR0=1;                        //启动T0定时
  TR1=1;                        //启动T1定时
}
/****DA数据的显示段码信息更新****/
void  disp_g(void)              //DA数据的显示段码信息更新
{ buf[7]=0x00;                  //符号位送正号段码
  buf[4]=0x76;                  //后缀位送十六进制后缀H的段码
  Ax=r_kT&0x0f;                 //取要显示的物理量
  buf[5]=Stab[Ax];              //8位DA数据的低4位对应十六进制数段码
  Ax=(r_kT&0xf0)>>4;            //取要显示的物理量
  buf[6]=Stab[Ax];              //8位DA数据的高4位对应十六进制数段码
}
void  disp_f(void)              //DA输出电压理论值的显示段码信息更新
{ if(m_kT<0)                    //如果是负数
  { Ax=-m_kT;                   //取绝对值
    buf[3]=0x40;                //符号位送负号段码
  }
  else                          //否则就是正数
  { Ax=m_kT;                    //取要显示的物理量
    buf[3]=0x00;                //符号位送正号段码
  }
  buf[0]=Stab[Ax%10];           //电压小数位段码信息更新
  buf[1]=Stab[Ax/10%10]|0x80;   //电压个位段码携小数点信息更新
  buf[2]=Stab[Ax/100];          //电压十位段码信息更新
}
/*************主程序*************/
void  main(void)
{ DA=0x80;                      //设置理论上DA输出电压为零的DA值
  T0T1_init();                  //T0、T1初始化程序(11.0592MHz)
  do
  { if(M>7)                     //8数码管全显了,扫描显示周期到,该DA数据刷新
    { i++;                      //i加1
      if((i&0x01)==0)r_kT=0x00; //i是偶数，D/A数据是0
      else           r_kT=0xff; //i是偶数，D/A数据是FFH
      DA=r_kT;                  //DA数字量N刷新,Iout1=(Vref/R)(N/256)
      temp1=(u16)r_kT<<1;       //计算2N,  Vs5=-Vref(N/256)
      temp2=(int)temp1;         //         VT1=10[-(R4/R2)Vs5-(R4/R3)Vref]
      temp2-=256;               //计算2N-256,=10[(220/50)5(N/256)-(220/100)5]
      VT1=(float)temp2*0.4296875;//计算0.4296875(2N-256),=110[(2N-256)/256]
      m_kT=(char)VT1;           //则N=0,VT1=-11.0V;N=255,VT1=10.914V
      SBUF=r_kT;                //发送给定值
      disp_g();                 //DA数据的显示段码信息更新
      disp_f();                 //DA输出电压理论值的显示段码信息更新
      M=0;                      //扫描显示位计数器清0
    }
  }while(1);
}
/*********T0中断服务程序*********/
void t0(void) interrupt 1 using 1//1.25ms中断1次，每次均要进行显示处理
{ TH0=0xfb;                     //T0时间常数高字节重装
  TL0=0x80;                     //T0时间常数低字节重装
  B_port=0xff;                  //关闭显示
  D_port=buf[M];                //对应数码管的段码值送给段码端口U2
  B_port=Btab[M];               //对应数码管的位码值送给位码端口U3
  M++;                          //修改到下一次要显示的数码管
}