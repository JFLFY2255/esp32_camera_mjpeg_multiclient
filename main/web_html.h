#ifndef WEB_HTML_H
#define WEB_HTML_H

// 使用原始字符串字面量存储HTML内容, 分隔符修改为 "X"
const char html[] = R"X(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Camera Stream</title>
  <style>
    body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 20px; }
    #container { max-width: 800px; margin: 0 auto; }
    #stream { max-width: 100%; border: 1px solid #ccc; }
    #rgb565Stream { display: none; }
    canvas { max-width: 100%; border: 1px solid #ccc; }
    button { padding: 10px; margin: 10px; cursor: pointer; }
    #debug { font-family: monospace; font-size: 12px; text-align: left; white-space: pre; height: 100px; overflow: auto; border: 1px solid #ccc; padding: 5px; margin-top: 10px; }
  </style>
</head>
<body>
  <div id="container">
    <h1>Camera Stream Demo</h1>
    <div>
      <button onclick="showJPEG()">Show JPEG</button>
      <button onclick="showRGB565()">Show RGB565</button>
    </div>
    <img id="stream" src="/mjpeg/1" alt="JPEG Stream Error">
    <div id="rgb565Stream">
      <canvas id="canvas" width="128" height="128" alt="RGB565 Stream Error"></canvas>
    </div>
    <div id="debug"></div>
  </div>
  <script>
    let isRGB565Mode = false;
    let xhr = null;
    let canvas = document.getElementById('canvas');
    let ctx = canvas.getContext('2d');
    let debugDiv = document.getElementById('debug');
    
    function log(msg) {
      debugDiv.innerHTML += msg + '\n';
      debugDiv.scrollTop = debugDiv.scrollHeight;
    }
    
    function showJPEG() {
      if (isRGB565Mode) {
        if (xhr) xhr.abort();
        document.getElementById('stream').style.display = 'block';
        document.getElementById('rgb565Stream').style.display = 'none';
        document.getElementById('debug').style.display = 'none';
        isRGB565Mode = false;
      }
    }
    
    function showRGB565() {
      if (!isRGB565Mode) {
        document.getElementById('stream').style.display = 'none';
        document.getElementById('rgb565Stream').style.display = 'block';
        document.getElementById('debug').style.display = 'block';
        isRGB565Mode = true;
        startRGB565Stream();
      }
    }
    
    function startRGB565Stream() {
      log('Starting RGB565 stream...');
      if (xhr) xhr.abort();
      
      xhr = new XMLHttpRequest();
      xhr.open('GET', '/mrgb565/1', true);
      xhr.responseType = 'arraybuffer';
      
      let boundary = '--123456789000000000000987654321';
      let dataBuffer = new Uint8Array(0);
      
      xhr.onreadystatechange = function() {
        if (xhr.readyState >= 3) { // Process on LOADING (3) and DONE (4) states
          try {
            dataBuffer = new Uint8Array(xhr.response);
            log('Total data in XHR response: ' + dataBuffer.length + ' bytes');
            
            let textDecoder = new TextDecoder();
            let dataStr = textDecoder.decode(dataBuffer);

            let startMarker = 'Content-Type: image/rgb565\r\nContent-Length: ';
            let headerStart = dataStr.indexOf(startMarker);
            
            if (headerStart >= 0) {
              headerStart += startMarker.length;
              let headerEnd = dataStr.indexOf('\r\n\r\n', headerStart);
              
              if (headerEnd >= 0) {
                let frameSizeStr = dataStr.substring(headerStart, headerEnd);
                let frameSize = parseInt(frameSizeStr);
                let prefixStr = dataStr.substring(0, headerEnd + 4);
                let frameDataStartInBytes = new TextEncoder().encode(prefixStr).length;

                if (!isNaN(frameSize) && frameSize > 0 && dataBuffer.length >= frameDataStartInBytes + frameSize) {
                  let currentFrameData = dataBuffer.slice(frameDataStartInBytes, frameDataStartInBytes + frameSize);
                  log('Extracted frame: ' + frameSize + ' bytes');
                  
                  if (currentFrameData.length > 0) {
                    log('First bytes: ' + 
                        currentFrameData[0].toString(16) + ',' + 
                        currentFrameData[1].toString(16) + ',' + 
                        currentFrameData[2].toString(16) + ',' + 
                        currentFrameData[3].toString(16));
                    processRGB565Data(currentFrameData);
                  }
                } else {
                  log('Incomplete frame or invalid size. Waiting for more data.');
                }
              } else {
                log('Incomplete frame header. Waiting for more data.');
              }
            } else if (dataStr.length > 0 && !dataStr.startsWith("HTTP/")) {
                log('No valid frame header found in the current data chunk.');
            }
          } catch (e) {
            log('Error in onreadystatechange: ' + e.message + ' Stack: ' + e.stack);
          }
        }
      };
      
      xhr.onerror = function(e) {
        log('XHR error: ' + e);
      };
      
      xhr.send();
    }
    
    function processRGB565Data(data) {
      try {
        let width = 128;
        let height = 128;
        
        if (data.length < width * height * 2) {
          log('Warning: Data too small for specified dimensions, expected ' + (width * height * 2) + ' bytes, got ' + data.length + ' bytes');
        }
        
        if (canvas.width !== width || canvas.height !== height) {
            // canvas.width = width;
            // canvas.height = height;
        }

        let imgData = ctx.createImageData(width, height);
        let pixels = imgData.data;
        let k = 0;
        
        for (let y = 0; y < height; y++) {
          for (let x = 0; x < width; x++) {
            let i = (y * width + x) * 2;
            
            if (i + 1 < data.length) {
              let pixel = (data[i+1] << 8) | data[i];
              let r = (pixel >> 11) & 0x1F;
              let g = (pixel >> 5) & 0x3F;
              let b = pixel & 0x1F;
              pixels[k++] = r << 3;
              pixels[k++] = g << 2;
              pixels[k++] = b << 3;
              pixels[k++] = 255;
            } else {
              pixels[k++] = 0;
              pixels[k++] = 0;
              pixels[k++] = 0;
              pixels[k++] = 255;
            }
          }
        }
        ctx.putImageData(imgData, 0, 0);
      } catch (e) {
        log('Processing error in processRGB565Data: ' + e.message);
      }
    }
  </script>
</body>
</html>
)X";

#endif // WEB_HTML_H