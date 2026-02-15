/**
 * http播放网络音乐类
 */

#ifndef MUSIC_PLAY_CLIENT_H
#define MUSIC_PLAY_CLIENT_H

#include "Arduino.h"
#include "WiFi.h"
//audio.h库 运行中会用到 psram, 如无psram会无法运行
//说明如下：
//    https://github.com/schreibfaul1/ESP32-audioI2S
#include "Audio.h"

//引入WiFi配网包
#include "wen_ap.h"


class PlayMusicClient {
public:
  PlayMusicClient();
  void set_ServerUrl(String url);
  void setPinout(int BCLK, int LRC, int DOUT);
  void setVolume(int vol);

  void play_url(const char* url);
  void audio_loop();
  void stopSong();

private:
  String _serverUrl;
  Audio audio;
  bool init_hard();
};

#endif  // MUSIC_PLAY_CLIENT_H
