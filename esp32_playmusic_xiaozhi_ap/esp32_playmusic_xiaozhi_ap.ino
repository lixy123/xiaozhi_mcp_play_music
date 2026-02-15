#include <WiFi.h>
#include <WebSocketMCP.h>
#include "music_client.h"
#include "music_play.h"

// 缓冲区管理
#define MAX_INPUT_LENGTH 1024
char inputBuffer[MAX_INPUT_LENGTH];
int inputBufferIndex = 0;

// 连接状态
bool wifiConnected = false;
bool mcpConnected = false;

// 创建WebSocketMCP实例
WebSocketMCP mcpClient;

// 创建音乐客户端
MusicClient musicClient;
//声音播放器
//Audio.h 这个库有bug, 初始化后一定时间内不能再调用setPinout，否则会异常重启，所以要用 new 类创建
//PlayMusicClient * playMusicClient;
PlayMusicClient playMusicClient;

int g_volume = 5;
String g_play_song = "";

/********** 函数声明 ***********/
void onMcpOutput(const String& message);
void onMcpError(const String& error);
void onMcpConnectionChange(bool connected);
void processSerialCommands();
void registerMcpTools();



/*
  功能： ESP32S3 播放网上音乐
  
  一. 硬件：
    必须是带PSRAM的ESP32/ESP32S3等

  二。软件：

  IDE：
  arduino: 2.3.5

  库版本：
  WebSocketMCP 1.0.0 https://github.com/toddpan/xiaozhi-esp32-mcp
  ESPAsyncWebServer 3.10.0 https://github.com/ESP32Async/ESPAsyncWebServer
  AsyncTCP  3.4.10 https://github.com/dvarrel/AsyncTCP    注：被ESPAsyncWebServer调用，版本不能过低
  ArduinoJson 7.4.2 https://github.com/bblanchon/ArduinoJson.git
  arduinoWebSockets  2.7.2 https://github.com/Links2004/arduinoWebSockets
  ESP32-audioI2S 3.4.4 https://github.com/schreibfaul1/ESP32-audioI2S

  编译设置：
    USB cdc on boot: enabled
    psram选择： OPI PSRAM
    flash size: 16MB
    partiation scheme: huge app(3MB NO OTA/1MB SPIFFS

    编译大小: 1.9MB
    电流：
      待机：    60-80ma
      播放音乐：90-120ma
*/

/**
   MCP输出回调函数(stdout替代)
*/
void onMcpOutput(const String& message) {
  Serial.print("[MCP输出] ");
  Serial.println(message);
}


/**
   MCP连接状态变化回调函数
*/
void onMcpConnectionChange(bool connected) {
  mcpConnected = connected;
  if (connected) {
    Serial.println("[MCP] 已连接到MCP服务器");
    // 连接成功后注册工具
    registerMcpTools();
  } else {
    Serial.println("[MCP] 与MCP服务器断开连接");
  }
}

// 注册MCP工具
void registerMcpTools() {

  //播放音乐
  mcpClient.registerTool(
    "play_online_music",
    "播放音乐",
    "{\"type\":\"object\",\"properties\":{\"song_name\":{\"type\":\"string\"}}, \
        \"required\":[\"song_name\"]}",
    [](const String& args) {
      Serial.println("play_online_music:" + args);

      String result = "完毕";
      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, args);

      if (error) {
        // 返回错误响应
        WebSocketMCP::ToolResponse response("{\"success\":false,\"error\":\"无效的参数格式\"}", true);
        return response;
      }

      String song_name = doc["song_name"].as<String>();
      Serial.println("play_online_music:" + song_name);
      g_play_song = song_name;
      return WebSocketMCP::ToolResponse(String("{\"success\":true,\"result\":\"") + result + "\"}");
    });

  mcpClient.registerTool(
    "stop_online_music",
    "停止播放音乐",
    "{\"type\":\"object\",\"properties\":{}}",
    [](const String& args) {
      Serial.println("stop_online_music:" + args);
      playMusicClient.stopSong();
      String result = "完毕";
      return WebSocketMCP::ToolResponse(String("{\"success\":true,\"result\":\"") + result + "\"}");
    });


  mcpClient.registerTool(
    "set_online_music_volume",
    "调整音乐音量",
    "{\"type\":\"object\",\"properties\":{\"Volume\":{\"type\":\"integer\",\"minimum\":0,\"maximum\":100}}, \
       \"required\":[\"Volume\"]}",
    [](const String& args) {
      Serial.println("set_online_music_Volume:" + args);

      DynamicJsonDocument doc(512);
      DeserializationError error = deserializeJson(doc, args);

      if (error) {
        // 返回错误响应
        WebSocketMCP::ToolResponse response("{\"success\":false,\"error\":\"无效的参数格式\"}", true);
        return response;
      }
      int Volume = doc["Volume"].as<int>();

      //为印射方便，设置0-20 ==> 0-100
      g_volume = map(Volume, 0, 100, 0, 20);
      Serial.printf("setVolume: %d, %d\n", Volume, g_volume);
      playMusicClient.setVolume(g_volume);
      String result = "";

      return WebSocketMCP::ToolResponse(String("{\"success\":true,\"result\":\"") + result + "\"}");
    });

  mcpClient.registerTool(
    "get_online_music_volume",
    "查询音乐音量",
    "{\"type\":\"object\",\"properties\":{}}",
    [](const String& args) {
      Serial.println("get_online_music_volume:" + args);
      //为印射方便，设置0-20 ==> 0-100
      int Volume = map(g_volume, 0, 20, 0, 100);
      Serial.printf("getVolume: %d, %d\n", Volume, g_volume);
      String result = String(Volume);

      return WebSocketMCP::ToolResponse(String("{\"success\":true,\"result\":\"") + result + "\"}");
    });

  Serial.println("[MCP] 注册完成");
}


/**
   MCP错误回调函数(stderr替代)
*/
void onMcpError(const String& error) {
  Serial.print("[MCP错误] ");
  Serial.println(error);
}

/**
   处理来自串口的命令
*/
void processSerialCommands() {
  // 检查是否有串口数据
  while (Serial.available() > 0) {
    char inChar = (char)Serial.read();

    // 处理回车或换行
    if (inChar == '\n' || inChar == '\r') {
      if (inputBufferIndex > 0) {
        // 添加字符串结束符
        inputBuffer[inputBufferIndex] = '\0';

        // 处理命令
        String command = String(inputBuffer);
        command.trim();

        if (command.length() > 0) {
          //设置音量
          if (command.startsWith("vol:")) {
            command.replace("vol:", "");
            if (command.toInt() >= 0 && command.toInt() <= 20) {
              g_volume = command.toInt();
              playMusicClient.setVolume(command.toInt());
              Serial.printf("volume=%d\n", g_volume);
            } else
              Serial.println("数值区间在:0-20");
          }
          //停止播放
          if (command.startsWith("stop:")) {
            playMusicClient.stopSong();
          }
          //url播放
          else if (command.startsWith(musicServerUrl)) {
            command.replace(musicServerUrl, "");
            playMusicClient.play_url(command.c_str());
          }
          //音乐名称搜音乐，并进行播放
          else {

            /*
            不稳定，一旦一首歌可不了，所有歌都听不了，并会复位！
            String fn = Search_Music_stream_pcm(command);
            fn.replace(musicServerUrl, "");
            //查询歌曲时，后台数据会开始缓存，需要延时一点时间
            delay(8000);
            playMusicClient.play_url(fn.c_str());

            */


            String fn = Search_Music(command);

            //检查文件是否转换完成，完成后进行播放！
            //实测，5-50秒左右均有！
            Serial.println("############");
            String fn_txt = fn + ".txt";
            bool ret_check = false;
            //120秒尝试...
            for (int loop1 = 0; loop1 < 24; loop1++) {
              delay(5000);
              Serial.printf("loop=%d", loop1);
              ret_check = musicClient.Check_Music(fn_txt);
              if (ret_check) break;
            }
            Serial.println("############");

            if (ret_check) {
              //delay(2000);
              playMusicClient.play_url(fn.c_str());
            } else
              Serial.println("skip play");
          }
        }

        // 重置缓冲区
        inputBufferIndex = 0;
      }
    }
    // 处理退格键
    else if (inChar == '\b' || inChar == 127) {
      if (inputBufferIndex > 0) {
        inputBufferIndex--;
        Serial.print("\b \b");  // 退格、空格、再退格
      }
    }
    // 处理普通字符
    else if (inputBufferIndex < MAX_INPUT_LENGTH - 1) {
      inputBuffer[inputBufferIndex++] = inChar;
      Serial.print(inChar);  // Echo
    }
  }
}


/**
 * 查找歌曲，返回第一首歌曲地址，下一步进行播放
   注：服务器会自动转换歌曲至适合esp32播放的MP3

   同一首歌，第一次播放可能慢。如果重复播放，则会从缓存读，速度快
 */
String Search_Music(const String& keyword) {
  Serial.println("\n========================================");
  Serial.println("Search_Music: " + keyword);
  Serial.println("========================================");

  String songName, filePath;

  // 调用一键下载接口
  bool success = musicClient.Search_Music(
    keyword,
    &songName,
    &filePath);

  if (success) {
    Serial.println("Download Complete!");
  } else {
    Serial.println("Download failed!");
  }
  return filePath;
}

/*
该接口播放音乐不稳定，经常不播放，重启！
String Search_Music_stream_pcm(const String& keyword) {
  Serial.println("\n========================================");
  Serial.println("Search_Music: " + keyword);
  Serial.println("========================================");

  String songName, filePath;

  // 调用一键下载接口
  bool success = musicClient.Search_Music_stream_pcm(
    keyword,
    &songName,
    &filePath);

  if (success) { 
    Serial.println("Download Complete!");
  } else {
    Serial.println("Download failed!");
  }
  return filePath;
}
*/

void loop() {

  //处理配网模式必要逻辑
  ap_serve();

  if (apstate)
    return;

  //处理mcp的音乐播放信号
  if (g_play_song.length() > 0) {

    /*
    String fn = Search_Music_stream_pcm(g_play_song);
    g_play_song = "";  //清空信息
    fn.replace(musicServerUrl, "");
    delay(5000);
    playMusicClient.play_url(fn.c_str());
    */

    String fn = Search_Music(g_play_song);
    g_play_song = "";  //清空信息
    //检查文件是否转换完成，完成后进行播放！
    //实测，5-50秒左右均有！
    Serial.println("############");
    String fn_txt = fn + ".txt";
    bool ret_check = false;
    //120秒尝试...
    for (int loop1 = 0; loop1 < 24; loop1++) {
      delay(5000);
      Serial.printf("loop=%d", loop1);
      ret_check = musicClient.Check_Music(fn_txt);
      if (ret_check) break;
    }
    Serial.println("############");

    if (ret_check) {
      delay(2000);
      playMusicClient.play_url(fn.c_str());
    } else
      Serial.println("skip play");
  }

  // 处理MCP客户端事件
  mcpClient.loop();

  // 处理来自串口的命令
  processSerialCommands();

  //声音服务处理，必须有此句
  playMusicClient.audio_loop();
  vTaskDelay(1);


  //加
  if (vol_down_pin.toInt() != -1) {
    if (!digitalRead(vol_up_pin.toInt())) {
      delay(50);
      if (!digitalRead(vol_up_pin.toInt())) {
        if (g_volume < 20) {
          if (g_volume == 19)
            g_volume = g_volume + 1;
          else
            g_volume = g_volume + 2;
          playMusicClient.setVolume(g_volume);
          Serial.printf("volume=%d\n", g_volume);
        } else
          Serial.println("已最高，不能增加了");
      }
    }
  }

  //减
  if (vol_down_pin.toInt() != -1) {
    if (!digitalRead(vol_down_pin.toInt())) {
      delay(50);
      if (!digitalRead(vol_down_pin.toInt())) {
        if (g_volume >= 1) {
          if (g_volume == 1)
            g_volume = 0;
          else
            g_volume = g_volume - 2;
          playMusicClient.setVolume(g_volume);
          Serial.printf("volume=%d\n", g_volume);
        } else
          Serial.println("已最低，不能减少了");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  //必须加上，判断是否进入配网模式
  ap_init();

  //非AP配网模式，正常流程处理...
  if (apstate) {
    Serial.println("AP 配网模式...");
    return;
  }

  wifiConnected = true;

  if (vol_up_pin.toInt() != -1)
    pinMode(vol_up_pin.toInt(), INPUT_PULLUP);

  if (vol_down_pin.toInt() != -1)
    pinMode(vol_down_pin.toInt(), INPUT_PULLUP);


  musicClient.set_ServerUrl(musicServerUrl);

  playMusicClient.set_ServerUrl(musicServerUrl);
  playMusicClient.setPinout(i2s_out_bclk.toInt(), i2s_out_lrc.toInt(), i2s_out_dout.toInt());
  playMusicClient.setVolume(g_volume);


  // 初始化MCP客户端
  if (mcpClient.begin(mcpEndpoint.c_str(), onMcpConnectionChange)) {
    Serial.println("[ESP32 MCP客户端] 初始化成功，尝试连接到MCP服务器...");
  } else {
    Serial.println("[ESP32 MCP客户端] 初始化失败!");
  }
}
