
 <b>ESP32S3运行MCP, 小智AI用语音让其播放音乐。</b>   
 
 <b>  必备条件</b><br>
   1.有一台小智AI硬件，安装有小智固件并能进行语音对话。<br>
   2.有一台搭建了 https://github.com/OmniX-Space/MeowMusicServer 的音乐服务器URL地址，供该硬件使用，运行在公网或内网均可。<br>
     如果没有该地址，就没必要继续看本项目了，这是本项目的运行基础.<br>
   3.满足本项目的ESP32S3硬件，见下文。<br>
   开发MeowMusicServer项目的团队同时提供了小智用其音乐服务器播放音乐的项目。但似乎不太会修改小智共享项目，小智播放在线音乐代码没优化，编译出来的小智固件源码解析播放在线音乐效果很差劲。要么播不到半分钟停工，声音卡顿，播放中复位重启，几乎不能用。<br>
   
<b>一.硬件：</b><br/>
1.ESP32S3<br/>
  Flash大小>=4MB , 必须带PSRAM <br/>
  播放mp3用的ESP32-audioI2S库必须用到PSRAM， <br/>
  如果想在不带PSRAM的更多ESP32系列芯片上运行，可以把ESP32-audioI2S这个音乐播放库换成这个库: https://github.com/earlephilhower/ESP8266Audio，该库不要求使用PSRAM, 源码进行少量调整即可。我项目中没用这个库的原因是因为想用PSRAM, 在播放网络音乐能对音乐数据进行缓存，减少声音卡顿机率。<br/>
2.MAX98357A<br/> 
MAX98357A   ==>ESP32S3引脚<br/>
I2S_OUT_BCLK     15<br/>
I2S_OUT_LRC      16<br/>
I2S_OUT_DOUT     7<br/>
3.喇叭<br/>
连接到MAX98357A<br/>
我用的是淘宝上给小智设计的开发板，用 "小智AI语音对话机器人 MINI ESP32-S3-N16R8开发板" 可查到. <br/>
该板有几个功能按钮，还带MAX98357A，省去了单独外接。<br/>
<img src= 'https://github.com/lixy123/xiaozhi_mcp_play_music/blob/main/pic/hard.jpg?raw=true' /> <br/>

小智AI所用硬件一般是ESP32S3,ESP32C3等芯片。本程序是烧录在另一个ESP32S3芯片上运行的，不是烧录在小智所用硬件上的，不要弄混淆了。

<b>二.软件：</b><br/>
 <b>1. IDE：</b><br/>
  arduino: 2.3.5<br/>

  <b>2. 库版本：</b><br/>
  WebSocketMCP 1.0.0 https://github.com/toddpan/xiaozhi-esp32-mcp 注：不要用0.0.10版本，无法用<br/>
            如果下载不到最新的，用这个地址的库：https://github.com/lixy123/xiaozhi_mcp_play_music/blob/main/lib/xiaozhi-mcp.rar<br/>
  ESPAsyncWebServer 3.10.0 https://github.com/ESP32Async/ESPAsyncWebServer<br/>
  AsyncTCP  3.4.10 https://github.com/dvarrel/AsyncTCP    注：被ESPAsyncWebServer调用，版本不能过低<br/>
  ArduinoJson 7.4.2 https://github.com/bblanchon/ArduinoJson.git<br/>
  arduinoWebSockets  2.7.2 https://github.com/Links2004/arduinoWebSockets<br/>
  ESP32-audioI2S 3.4.4 https://github.com/schreibfaul1/ESP32-audioI2S<br/>

 <b>2.编译设置：</b><br/>
    开发板： esp32s3<br/>
    USB cdc on boot: enabled<br/>
    psram选择： OPI PSRAM<br/>
    flash size: 16MB<br/>
    partiation scheme: huge app(3MB NO OTA/1MB SPIFFS)<br/>

 <b>4.烧录</b><br/>
 方式1: 源码烧录<br/>
A.打开arduino, 打开源码，导入上面的lib库，如果版本不一致，有一定机率编译有问题，<br/>
B.arduino进行编译设置，试编译.<br/>
C.插入ESP32S3等<br/>
D.arduino选择端口<br/>
E.arduino点击烧录按钮。 (注：有些ESP32S3板需要先按boot+reset两键进入烧录模式，有些可不用)<br/>
F.烧录完后重新上电 (注：有些板烧录完毕能自动重启，不必重新上电)<br/>

方式2:  固件烧录 （无需准备源码环境，最省事的方法)<br>
A.下载 flash_download_tool软件<br>
B.下载编译好的固件: target.bin （已上传至：Releases) <br>
注：源码是用esp32s3编译的，该固件只能烧录在esp32s3 <br>
C. target.bin烧录到地址 0x0<br>
注： flash_download_tool 使用技巧参考：<br>
https://my.feishu.cn/wiki/Zpz4wXBtdimBrLk25WdcXzxcnNS<br>

编译好的固件烧录界面:<br/>
<img src= 'https://github.com/lixy123/xiaozhi_mcp_play_music/blob/main/pic/shaolv.jpg?raw=true' /> <br/>

<b> 三. 配置 </b> <br/>
上电后有两种需要配置相关参数：<br/>
<b> >>>小智 后台配置： </b><br/>
1.登陆小智 网站https://xiaozhi.me/<br/>
2.配置角色介绍<br/>
进入 >控制台>对应的智能体>配置角色><br/>
在角色介绍中加入如下文字<br/>
所有用户播放音乐的请求，我会调用play_online_music的MCP服务，并提醒用户请稍等，马上为你播放！<br/>
当用户要求音乐停止时，我会调用stop_online_music的MCP服务，并提醒用户音乐已停止！<br/>
所有用户调整音乐音量请求，我会调用set_online_music_volume的MCP服务，并提醒用户音乐音量已设置！<br/>
所有用户查询音乐音量请求，我会调用get_online_music_volume的MCP服务！<br/>
注：配置完需要重启小智生效。以后不要重新查看MCP地址，再次进入页面查看MCP地址时，地址会变化，会需要重新配置ESP32S3参数<br/>
3.拷出MCP地址<br/>
进入>控制台>对应的智能体>配置角色>MCP设置>获取MCP接入点<br/>
拷出MCP接入点URL地址,下一步要用.<br/>

<b> >>>ESP32S3配置：</b> <br/>
1.首次运行，ESP32S3会创建一个AP.<br/>
2.用手机或电脑连接此AP<br/>
3.浏览器输入网址 http://192.168.4.1<br/>
4.填入配置参数<br/>
<img src= 'https://github.com/lixy123/xiaozhi_mcp_play_music/blob/main/pic/set.jpg?raw=true' /> <br/>

参数说明：<br/>
WIFI_SSID 所连接的路由器 AP账号<br/>
WIFI_PASS 所连接的路由器 AP密码<br/>
mcpEndpoint 上一步得到的小智MCP地址<br/>
Music Server Address 搭建了 https://github.com/OmniX-Space/MeowMusicServer 的音乐服务器URL地址<br/>
i2s_out_bclk ESP32开发板连接数字功放（MAX98357A）的3个引脚, 配置错误则无法发出语音<br/>
i2s_out_lrc<br/>
i2s_out_dout<br/>
led_pin ESP32   LED灯引脚，AP配置模式会亮灯 （-1表示不使用）<br/>
reset_pin ESP32 重置参数引脚，按住超5秒，下次上电后进入AP设置模式，默认0， 也就是boot按钮<br/>
volume up_pin   增加音量引脚 （-1表示不使用）<br/>
volume down_pin 减少音量引脚 （-1表示不使用）<br/>
注：所有参数必须填上，否则不让保存。<br/>

<b>四.使用</b><br/>
语音指令：<br/>
1.唤醒小智后，对着小智说:"我要听张惠妹的听海"。 小智会提示“马上为您播放”，几秒钟后，音乐会从烧录有本项目固件的ESP32S3上播放出来<br/>
2.对着小智说:"停止音乐"。停止音乐播放<br/>
3.对着小智说:"调整音乐音量到50"。调整音乐播放音量<br/>

硬件按钮：<br/>
1.按下39, 40 引脚增加，减少音乐播放音量。<br/>
2.长按0 引脚5秒以后，本设备重启并进入配置参数模式。<br/>

<b>五.问题</b><br/>
试听了几天，大部分音乐都能正常播放，偶有音乐卡顿，但基本上均能正常播放完。<br/>
1.偶有音乐播放卡顿，原因是音乐服务器传输问题，毕竟是网上的免费音乐，稳定性不容易保证。<br/>
2.偶有音乐播放不出。<br/>
3.极少数机率播放中途重启。<br/>

<b>六.后续改进计划</b><br/>
1.增加听歌稳定性。<br/>
2.提升播放音质。<br/>

