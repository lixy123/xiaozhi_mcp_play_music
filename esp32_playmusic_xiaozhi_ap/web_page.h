
const char configPage[] PROGMEM = R"=====(
<!Doctype html>
<html >
<head>
  <meta charset="UTF-8">
</head>
<body>
        <h1>参数设置</h1>
        <form action="/save" id="myForm" method="POST">
            WIFI_SSID: <input type="text" name="ssid"  value="<<ssid>>">
           
            WIFI_Password: <input type="password" name="pwd"  value="<<pwd>>">
            <br>
            
            mcpEndpoint: <input type="text" name="mcpEndpoint"  style="width:600px"  value="<<mcpEndpoint>>">   
            <br>       
            Music Server Address: <input type="text" name="musicServerUrl" style="width:600px"   value="<<musicServerUrl>>">
            <br> 
            reset pin: <input type="text" name="reset_pin"  style="width:50px"   value="<<reset_pin>>">            
            led pin: <input type="text" name="led_pin"  style="width:50px"   value="<<led_pin>>">
            volume up_pin: <input type="text" name="vol_up_pin"  style="width:50px"   value="<<vol_up_pin>>">            
            volume down pin: <input type="text" name="vol_down_pin"  style="width:50px"   value="<<vol_down_pin>>">
            <br> 
            i2s_out_bclk:<input type="text" name="i2s_out_bclk"    style="width:50px"  value="<<i2s_out_bclk>>">
            i2s_out_lrc:<input type="text" name="i2s_out_lrc"  style="width:50px"  value="<<i2s_out_lrc>>">                   
            i2s_out_dout:<input type="text" name="i2s_out_dout"   style="width:50px"   value="<<i2s_out_dout>>">           
            <br>
            <input type="submit" value="保存">
            <br>
            <<span>>
        </form>

       
<script>

 
// 获取表单元素并添加事件监听器，监听submit事件
document.getElementById('myForm').addEventListener('submit', function (event) {

    if (!validateRequired()) {
       // alert("参数检查未通过!")
        event.preventDefault() // 阻止表单提交
    }
})

// 校验参数
function validateRequired() {
    // 获取表单中文本的输入元素
    var ssid = document.getElementsByName('ssid')[0]
 
    var musicServerUrl = document.getElementsByName('musicServerUrl')[0]
    var mcpEndpoint = document.getElementsByName('mcpEndpoint')[0]

    var reset_pin = document.getElementsByName('reset_pin')[0]
    var led_pin = document.getElementsByName('led_pin')[0]
    var vol_up_pin = document.getElementsByName('vol_up_pin')[0]
    var vol_down_pin = document.getElementsByName('vol_down_pin')[0]

    var i2s_out_bclk = document.getElementsByName('i2s_out_bclk')[0]
    var i2s_out_lrc = document.getElementsByName('i2s_out_lrc')[0]
    var i2s_out_dout = document.getElementsByName('i2s_out_dout')[0]

    if (ssid.value.trim() === "") {
        alert("ssid不能为空！")
        return false
    }

    if (musicServerUrl.value.trim() === "") {
        alert("musicServerUrl不能为空！")
        return false
    }
    if (mcpEndpoint.value.trim() === "") {
        alert("mcpEndpoint不能为空！")
        return false
    }
    if (reset_pin.value.trim() === "") {
        alert("reset_pin不能为空, -1表示不使用！")
        return false
    }
    if (led_pin.value.trim() === "") {
        alert("led_pin不能为空, -1表示不使用！")
        return false
    }

    if (vol_up_pin.value.trim() === "") {
        alert("vol_up_pin不能为空, -1表示不使用！")
        return false
    }

    if (vol_down_pin.value.trim() === "") {
        alert("vol_down_pin不能为空, -1表示不使用！")
        return false
    }

    if (isNaN(reset_pin.value.trim())) {
        alert("reset_pin不是数值！")
        return false
    }

    if (isNaN(led_pin.value.trim())) {
        alert("led_pin不是数值！")
        return false
    }

   if (isNaN(vol_up_pin.value.trim())) {
        alert("vol_up_pin不是数值！")
        return false
    }

   if (isNaN(vol_down_pin.value.trim())) {
        alert("vol_down_pin不是数值！")
        return false
    }
   
    
    if (i2s_out_bclk.value.trim() === "") {
        alert("i2s_out_bclk不能为空！")
        return false
    }
    if (i2s_out_lrc.value.trim() === "") {
        alert("i2s_out_lrc不能为空！")
        return false
    }
    if (i2s_out_dout.value.trim() === "") {
        alert("i2s_out_dout不能为空！")
        return false
    }
  
  if (isNaN(i2s_out_bclk.value.trim())) {
        alert("i2s_out_bclk不是数值！")
        return false
    }
    if (isNaN(i2s_out_lrc.value.trim())) {
        alert("i2s_out_lrc不是数值！")
        return false
    }
    if (isNaN(i2s_out_dout.value.trim())) {
        alert("i2s_out_dout不是数值！")
        return false
    }
  
    return true
    console.log("验证通过！")
}
</script>
       
</body>
</html>

)=====";
