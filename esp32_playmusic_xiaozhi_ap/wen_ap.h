#include <Arduino.h>
//根据开发板不同进行调整


//根据开发板调整, 改成 -1 表示禁用
//定义成0, 有时会判断出错
//#define RESET_WIFI_BTN 39

extern bool apstate;  //是否AP配网模式
extern String ssid;   //wifi账号
extern String pwd;    //wifi密码

extern String musicServerUrl;   //用https://github.com/OmniX-Space/MeowMusicServer 源码项目创建的音乐服务地址
extern String mcpEndpoint;      //小智mcp地址

extern String reset_pin;  //重置参数引脚，必须有
extern String led_pin;    //AP模式下LED亮灯状态， -1 表示不用

extern String vol_up_pin; 
extern String vol_down_pin; 

extern String i2s_out_bclk;
extern String i2s_out_lrc;
extern String i2s_out_dout; 


//放在主程序中可能一直死循环的，非loop()函数之外的地方
void pin_reset();
void ap_serve();
void ap_init();
