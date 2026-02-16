package main

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"os"
	"path/filepath"
	"regexp"
	"strconv"
	"strings"
	"sync"
	"time"
)

type YuafengAPIFreeResponse struct {
	Data struct {
		Song      string `json:"song"`
		Singer    string `json:"singer"`
		Cover     string `json:"cover"`
		AlbumName string `json:"album_name"`
		Music     string `json:"music"`
		Lyric     string `json:"lyric"`
	} `json:"data"`
}

// 枫雨API response handler with multiple API fallback
func YuafengAPIResponseHandler(sources, song, singer string) MusicItem {
	fmt.Printf("[Info] Fetching music data for %s by %s\n", song, singer)

	// API hosts to try in order
	apiHosts := []string{
		"https://api.yuafeng.cn",
		"https://api-v2.yuafeng.cn",
		"https://api.yaohud.cn",
	}

	var pathSuffix string
	switch sources {
	case "kuwo":
		pathSuffix = "/API/ly/kwmusic.php"
	case "netease":
		pathSuffix = "/API/ly/wymusic.php"
	case "migu":
		pathSuffix = "/API/ly/mgmusic.php"
	case "baidu":
		pathSuffix = "/API/ly/bdmusic.php"
	default:
		return MusicItem{}
	}

	var fallbackItem MusicItem // 保存第一个有音乐但没歌词的结果

	// Try each API host - 尝试所有API直到找到歌词
	for i, host := range apiHosts {
		fmt.Printf("[Info] Trying API %d/%d: %s\n", i+1, len(apiHosts), host)
		item := tryFetchFromAPI(host+pathSuffix, song, singer)
		if item.Title != "" {
			// 如果有歌词，立即返回
			if item.LyricURL != "" {
				fmt.Printf("[Success] ✓ Found music WITH lyrics from %s\n", host)
				return item
			}
			// 如果没有歌词但有音乐，保存作为fallback并继续尝试
			if fallbackItem.Title == "" {
				fallbackItem = item
				fmt.Printf("[Info] ○ Got music WITHOUT lyrics from %s, saved as fallback, continuing...\n", host)
			} else {
				fmt.Printf("[Info] ○ Got music WITHOUT lyrics from %s, trying next API...\n", host)
			}
		} else {
			fmt.Printf("[Warning] × API %s failed, trying next...\n", host)
		}
	}

	// 所有API都试完了
	if fallbackItem.Title != "" {
		fmt.Println("[Info] ▶ All 3 APIs tried - no lyrics found, returning music without lyrics")
		return fallbackItem
	}

	fmt.Println("[Error] ✗ All 3 APIs failed completely")
	return MusicItem{}
}

// tryFetchFromAPI attempts to fetch music data from a single API endpoint
func tryFetchFromAPI(APIurl, song, singer string) MusicItem {
	resp, err := http.Get(APIurl + "?msg=" + song + "&n=1")
	if err != nil {
		fmt.Println("[Error] Error fetching the data from API:", err)
		return MusicItem{}
	}
	defer resp.Body.Close()
	body, err := io.ReadAll(resp.Body)
	if err != nil {
		fmt.Println("[Error] Error reading the response body:", err)
		return MusicItem{}
	}

	// Check if response is HTML (starts with < character)
	bodyStr := string(body)
	if len(bodyStr) > 0 && bodyStr[0] == '<' {
		fmt.Println("[Warning] API returned HTML instead of JSON")
		fmt.Printf("[Debug] Saving HTML response to debug.html for inspection\n")

		// Save HTML to file for debugging
		os.WriteFile("debug_api_response.html", body, 0644)
		fmt.Println("[Info] HTML response saved to debug_api_response.html")

		// Try to extract JSON from HTML if embedded
		// Look for common patterns where JSON might be embedded
		if strings.Contains(bodyStr, `"song"`) && strings.Contains(bodyStr, `"singer"`) {
			fmt.Println("[Info] Attempting to extract JSON from HTML...")
			// Try to find JSON block in HTML
			jsonStart := strings.Index(bodyStr, "{")
			jsonEnd := strings.LastIndex(bodyStr, "}")
			if jsonStart != -1 && jsonEnd != -1 && jsonEnd > jsonStart {
				jsonStr := bodyStr[jsonStart : jsonEnd+1]
				var response YuafengAPIFreeResponse
				err = json.Unmarshal([]byte(jsonStr), &response)
				if err == nil {
					fmt.Println("[Success] Extracted JSON from HTML successfully")
					body = []byte(jsonStr)
					goto parseSuccess
				}
			}
		}

		fmt.Println("[Error] Cannot parse HTML response - API may be unavailable")
		return MusicItem{}
	}

parseSuccess:
	var response YuafengAPIFreeResponse
	err = json.Unmarshal(body, &response)
	if err != nil {
		fmt.Println("[Error] Error parsing API response:", err)
		return MusicItem{}
	}


	// Create directory
	dirName := fmt.Sprintf("./files/cache/music/%s-%s", response.Data.Singer, response.Data.Song)
	
	// ############## 追加代码 如果有缓存文件，不重新下载音乐 ######################################
	// 获取封面扩展名
    
    //判断本音乐缓存目录 dirName是否已下载或正在下载中，如果存在就跳过，不用重新下载!
    info, err := os.Stat(dirName)
    if err != nil {
        if os.IsNotExist(err) {
            fmt.Println("目录不存在")
        } else {
            fmt.Println("发生错误:", err)
        }
    } else {
        if info.IsDir() {
			fmt.Println("目录存在，跳过下载")
			basePath := "/cache/music/" + url.QueryEscape(response.Data.Singer+"-"+response.Data.Song)

        	return MusicItem{
			Title:        response.Data.Song,
			Artist:       response.Data.Singer,
			CoverURL:     basePath + "/cover" + filepath.Ext(response.Data.Cover),
			LyricURL:     basePath + "/lyric.lrc",
			AudioFullURL: basePath + "/music.mp3", // 标准 .mp3 URL
			AudioURL:     basePath + "/music.mp3",
			M3U8URL:      basePath + "/music.m3u8",
			Duration:     0, // 时长后台获取，先返回0
			}
        } else {
            fmt.Println("存在但不是目录")
        }
    }
	// ############## 结束 ###################################################################################
	
	
	
	err = os.MkdirAll(dirName, 0755)
	if err != nil {
		fmt.Println("[Error] Error creating directory:", err)
		return MusicItem{}
	}



	if response.Data.Music == "" {
		fmt.Println("[Warning] Music URL is empty")
		return MusicItem{}
	}

    // 获取封面扩展名
	ext := filepath.Ext(response.Data.Cover)

	// 保存远程 URL 到文件，供 file.go 流式转码使用
	remoteURLFile := filepath.Join(dirName, "remote_url.txt")
	os.WriteFile(remoteURLFile, []byte(response.Data.Music), 0644)

	// ========== 关键优化：先返回，后台异步处理 ==========
	// 把下载、转码等耗时操作放到 goroutine 异步执行
	go func() {
		fmt.Printf("[Async] Starting background processing for: %s - %s\n", response.Data.Singer, response.Data.Song)
		var wg sync.WaitGroup

		// ========== 1. 歌词处理 ==========
		wg.Add(1)
		go func() {
			defer wg.Done()
			lyricData := response.Data.Lyric
			if lyricData == "获取歌词失败" {
				fetchLyricFromYaohu(response.Data.Song, response.Data.Singer, dirName)
			} else if !strings.HasPrefix(lyricData, "http://") && !strings.HasPrefix(lyricData, "https://") {
				// 直接写入歌词内容
				lines := strings.Split(lyricData, "\n")
				lyricFilePath := filepath.Join(dirName, "lyric.lrc")
				file, err := os.Create(lyricFilePath)
				if err == nil {
					timeTagRegex := regexp.MustCompile(`^\[(\d+(?:\.\d+)?)\]`)
					for _, line := range lines {
						match := timeTagRegex.FindStringSubmatch(line)
						if match != nil {
							timeInSeconds, _ := strconv.ParseFloat(match[1], 64)
							minutes := int(timeInSeconds / 60)
							seconds := int(timeInSeconds) % 60
							milliseconds := int((timeInSeconds-float64(seconds))*1000) / 100 % 100
							formattedTimeTag := fmt.Sprintf("[%02d:%02d.%02d]", minutes, seconds, milliseconds)
							line = timeTagRegex.ReplaceAllString(line, formattedTimeTag)
						}
						file.WriteString(line + "\r\n")
					}
					file.Close()
					fmt.Printf("[Async] Lyrics saved for: %s - %s\n", response.Data.Singer, response.Data.Song)
				}
			} else {
				downloadFile(filepath.Join(dirName, "lyric.lrc"), lyricData)
			}
		}()

		// ========== 2. 封面处理 ==========
		wg.Add(1)
		go func() {
			defer wg.Done()
			downloadFile(filepath.Join(dirName, "cover"+ext), response.Data.Cover)
		}()

		// ========== 3. 音频转码 ==========
		wg.Add(1)
		go func() {
			defer wg.Done()
			musicExt, err := getMusicFileExtension(response.Data.Music)
			if err != nil {
				fmt.Println("[Async Warning] Cannot identify music format, using default .mp3:", err)
				musicExt = ".mp3"
			}

			outputMp3 := filepath.Join(dirName, "music.mp3")
			err = streamConvertAudio(response.Data.Music, outputMp3)
			if err != nil {
				fmt.Println("[Async Error] Error stream converting audio:", err)
				// 备用方案
				err = downloadFile(filepath.Join(dirName, "music_full"+musicExt), response.Data.Music)
				if err == nil {
					compressAndSegmentAudio(filepath.Join(dirName, "music_full"+musicExt), dirName)
				}
			}
		}()

		wg.Wait() // 等待所有任务完成
		fmt.Printf("[Async] Background processing completed for: %s - %s\n", response.Data.Singer, response.Data.Song)
	}()

	// ========== 立即返回 JSON，使用标准 .mp3 URL ==========
	// 注意：返回标准的 .mp3 URL，file.go 会在文件不存在时自动触发流式转码
	basePath := "/cache/music/" + url.QueryEscape(response.Data.Singer+"-"+response.Data.Song)

	return MusicItem{
		Title:        response.Data.Song,
		Artist:       response.Data.Singer,
		CoverURL:     basePath + "/cover" + ext,
		LyricURL:     basePath + "/lyric.lrc",
		AudioFullURL: basePath + "/music.mp3", // 标准 .mp3 URL
		AudioURL:     basePath + "/music.mp3",
		M3U8URL:      basePath + "/music.m3u8",
		Duration:     0, // 时长后台获取，先返回0
	}
}

// YaohuQQMusicResponse 妖狐QQ音乐API响应结构
type YaohuQQMusicResponse struct {
	Code int    `json:"code"`
	Msg  string `json:"msg"`
	Data struct {
		Songname string `json:"songname"`
		Name     string `json:"name"`
		Picture  string `json:"picture"`
		Musicurl string `json:"musicurl"`
		Viplrc   string `json:"viplrc"` // VIP歌词URL链接
	} `json:"data"`
}

// YaohuLyricResponse 妖狐歌词API响应结构
type YaohuLyricResponse struct {
	Code int `json:"code"`
	Data struct {
		Lyric string `json:"lyric"`
	} `json:"data"`
}

// fetchLyricFromYaohu 从妖狐数据QQ音乐VIP API获取歌词（备用方案）
func fetchLyricFromYaohu(songName, artistName, dirPath string) bool {
	apiKey := "bXO9eq1pomwR1cyVhzX"
	apiURL := "https://api.yaohud.cn/api/music/qq"

	// 构建请求URL - QQ音乐VIP接口
	requestURL := fmt.Sprintf("%s?key=%s&msg=%s&n=1&size=hq",
		apiURL,
		apiKey,
		url.QueryEscape(songName))

	fmt.Printf("[Info] 🎵 Trying to fetch lyric from Yaohu QQ Music VIP API for: %s - %s\n", artistName, songName)

	// 创建带超时的HTTP客户端
	client := &http.Client{
		Timeout: 10 * time.Second,
	}

	resp, err := client.Get(requestURL)
	if err != nil {
		fmt.Printf("[Error] Yaohu QQ Music API request failed: %v\n", err)
		return false
	}
	defer resp.Body.Close()

	body, err := io.ReadAll(resp.Body)
	if err != nil {
		fmt.Printf("[Error] Failed to read API response: %v\n", err)
		return false
	}

	var qqResp YaohuQQMusicResponse
	err = json.Unmarshal(body, &qqResp)
	if err != nil {
		fmt.Printf("[Error] Failed to parse API response: %v\n", err)
		return false
	}

	// 检查响应状态
	if qqResp.Code != 200 {
		fmt.Printf("[Warning] API returned error (code: %d, msg: %s)\n", qqResp.Code, qqResp.Msg)
		return false
	}

	// 检查viplrc URL是否存在
	if qqResp.Data.Viplrc == "" {
		fmt.Printf("[Warning] No lyric URL available for: %s\n", songName)
		return false
	}

	fmt.Printf("[Info] 🔍 Found song: %s - %s\n", qqResp.Data.Songname, qqResp.Data.Name)
	fmt.Printf("[Info] 📝 Fetching lyric from: %s\n", qqResp.Data.Viplrc)

	// Step 2: 获取实际歌词内容
	resp2, err := client.Get(qqResp.Data.Viplrc)
	if err != nil {
		fmt.Printf("[Error] Failed to fetch lyric from viplrc URL: %v\n", err)
		return false
	}
	defer resp2.Body.Close()

	body2, err := io.ReadAll(resp2.Body)
	if err != nil {
		fmt.Printf("[Error] Failed to read lyric response: %v\n", err)
		return false
	}

	// viplrc URL直接返回LRC文本，不是JSON
	lyricText := string(body2)

	// 检查歌词内容
	if lyricText == "" || len(lyricText) < 10 {
		fmt.Printf("[Warning] No lyrics returned from viplrc URL\n")
		return false
	}

	// 将歌词写入文件
	lyricFilePath := filepath.Join(dirPath, "lyric.lrc")
	file, err := os.Create(lyricFilePath)
	if err != nil {
		fmt.Printf("[Error] Failed to create lyric file: %v\n", err)
		return false
	}
	defer file.Close()

	// 写入歌词内容（LRC文本格式）
	_, err = file.WriteString(lyricText)
	if err != nil {
		fmt.Printf("[Error] Failed to write lyric content: %v\n", err)
		return false
	}

	fmt.Printf("[Success] ✅ Lyric fetched from Yaohu QQ Music VIP API and saved to %s\n", lyricFilePath)
	return true
}

// getRemoteMusicURLOnly 只获取远程音乐URL，不下载不处理（用于实时流式播放）
func getRemoteMusicURLOnly(song, singer string) string {
	fmt.Printf("[Info] Getting remote music URL for: %s - %s\n", singer, song)

	// 尝试多个 API
	apiHosts := []string{
		"https://api.yuafeng.cn",
		"https://api-v2.yuafeng.cn",
	}

	sources := []string{"kuwo", "netease", "migu"}
	pathMap := map[string]string{
		"kuwo":    "/API/ly/kwmusic.php",
		"netease": "/API/ly/wymusic.php",
		"migu":    "/API/ly/mgmusic.php",
	}

	client := &http.Client{Timeout: 15 * time.Second}

	for _, host := range apiHosts {
		for _, source := range sources {
			path := pathMap[source]
			apiURL := fmt.Sprintf("%s%s?msg=%s-%s&n=1", host, path, url.QueryEscape(song), url.QueryEscape(singer))

			resp, err := client.Get(apiURL)
			if err != nil {
				continue
			}
			defer resp.Body.Close()

			body, err := io.ReadAll(resp.Body)
			if err != nil {
				continue
			}

			var response YuafengAPIFreeResponse
			if err := json.Unmarshal(body, &response); err != nil {
				continue
			}

			if response.Data.Music != "" {
				fmt.Printf("[Success] Got remote URL from %s: %s\n", source, response.Data.Music)
				return response.Data.Music
			}
		}
	}

	fmt.Println("[Error] Failed to get remote music URL from all APIs")
	return ""
}
