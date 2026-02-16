/**
 * ESP32 音乐服务器客户端
 * 用于调用 music-server API
 */

#ifndef MUSIC_CLIENT_H
#define MUSIC_CLIENT_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

class MusicClient {
public:
  MusicClient() {}

  void set_ServerUrl(String url) {
    _serverUrl = url;
  }

  /**
     * 搜索音乐，返回音乐名称，音乐路径
     * @param keyword   输入：搜索关键词（歌名/歌手）
     * @param songName  指针输出: 歌曲名
     * @param audio_url 指针输出: 文件路径

     * @return true 成功
  */


  //整体播放有时需要等待30多秒，反应慢！
  bool Search_Music(const String& keyword, String* songName = nullptr, String* filePath = nullptr) {
    HTTPClient http;
    String url = _serverUrl + "/api/search?query=" + urlEncode(keyword);

    Serial.printf("url:%s\n", url.c_str());
    http.begin(url);
    http.setTimeout(120000);  // 120秒超时（下载转码需要时间）

    int httpCode = http.GET();
    bool success = false;

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.printf("payload:%s\n", payload.c_str());

      ///api/search 需要去头，去尾 '[',  ']'
      //stream_pcm?song 不用
      payload = payload.substring(1, payload.length() - 2);  // 截取子字符串

      if (payload.length() > 0) {

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);

        if (!error) {

          if (songName) {
            if (doc.containsKey("artist") && doc.containsKey("title"))
              *songName = doc["artist"].as<String>() + "-" + doc["title"].as<String>();
          }

          if (filePath) {
            if (doc.containsKey("url"))
              *filePath = doc["url"].as<String>();
          }

          success = true;
          Serial.printf("[Music] Downloaded:\nsongName: %s\nurl: %s\n",
                        songName->c_str(),
                        filePath->c_str());
        } else {
          Serial.printf("[Music] Error: \n");
        }
      } else
        Serial.println("json is null");
    } else {
      Serial.printf("[Music] HTTP Error: %d\n", httpCode);
    }

    http.end();
    return success;
  }

  /*
播放音乐不稳定，播放不出音乐机率大，重启机会大
bool Search_Music_stream_pcm(const String& keyword, String* songName = nullptr, String* filePath = nullptr) {
    HTTPClient http;
    String url = _serverUrl + "/stream_pcm?song=" + urlEncode(keyword);

    Serial.printf("url:%s\n", url.c_str());
    http.begin(url);
    http.setTimeout(60000);  // 60秒超时

    int httpCode = http.GET();
    bool success = false;

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();

      Serial.printf("payload:%s\n", payload.c_str());
     

      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, payload);

      if (!error) {

        if (songName) {
          if (doc.containsKey("artist") && doc.containsKey("title"))
            *songName = doc["artist"].as<String>() + "-" + doc["title"].as<String>();
        }

        if (filePath) {
          if (doc.containsKey("audio_url"))
            *filePath = doc["audio_url"].as<String>();
        }

        success = true;
        Serial.printf("[Music] Downloaded:\nsongName: %s\nurl: %s\n",
                      songName->c_str(),
                      filePath->c_str());
      } else {
        Serial.printf("[Music] Error: \n");
      }
    } else {
      Serial.printf("[Music] HTTP Error: %d\n", httpCode);
    }

    http.end();
    return success;
  }
*/


  bool Check_Music(const String& url) {
    HTTPClient http;
    String url_check = _serverUrl + url;
    //Serial.printf("url:%s\n", url_check.c_str());
    http.begin(url_check);

    int httpCode = http.GET();
    bool success = false;

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      //Serial.printf("payload:%s\n", payload.c_str());

      if (payload == "ok")
        success = true;
    } else {
      Serial.printf("[Music] HTTP Error: %d\n", httpCode);
    }

    http.end();
    return success;
  }

private:
  String _serverUrl;

  String urlEncode(const String& str) {
    String encoded = "";
    char c;
    char code0, code1;
    for (size_t i = 0; i < str.length(); i++) {
      c = str.charAt(i);
      if (isalnum(c)) {
        encoded += c;
      } else if (c == ' ') {
        encoded += '+';
      } else {
        code0 = (c >> 4) & 0xF;
        code1 = c & 0xF;
        encoded += '%';
        encoded += "0123456789ABCDEF"[code0];
        encoded += "0123456789ABCDEF"[code1];
      }
    }
    return encoded;
  }
};

#endif  // MUSIC_CLIENT_H
