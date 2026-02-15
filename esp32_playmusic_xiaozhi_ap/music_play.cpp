#include "music_play.h"

PlayMusicClient::PlayMusicClient()
{}


void PlayMusicClient::set_ServerUrl(String url) {
  _serverUrl = url;
}

void PlayMusicClient::setPinout(int BCLK, int LRC, int DOUT) {
  audio.setPinout(BCLK, LRC, DOUT);
}

//暂未用
bool PlayMusicClient::init_hard() {

 // audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
 // audio.setVolume(5);  // default 0...21
  return true;
}

void PlayMusicClient::stopSong() {

  audio.stopSong();
}

void PlayMusicClient::play_url(const char* url) {
  if (strlen(url) > 0) {
    String music_url = _serverUrl + url;
    Serial.printf("play_url:%s\n", music_url.c_str());
    audio.connecttohost(music_url.c_str());
  } else
    Serial.printf("url 为空\n");
}

void PlayMusicClient::setVolume(int vol) {
  audio.setVolume(vol);  // default 0...21
}

void PlayMusicClient::audio_loop() {
  audio.loop();
}
