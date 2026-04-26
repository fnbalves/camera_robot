#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_camera.h"
#include "img_converters.h"
#include <ESPAsyncWebServer.h>

const char* LANDING_PAGE = R"rawliteral(
<html>
    <head>
        <style>
            * {
                box-sizing: border-box;
                margin: 0;
                padding: 0;
            }

            body {
                display: flex;
                align-items: center;
                justify-content: center;
                width: 100dvw;
                height: 100dvh;
                touch-action: none;
                overflow: hidden;
                overscroll-behavior: none;
            }

            .control-panel {
                display: flex;
                flex-direction: column;
                gap: 1em;
                align-items: flex-start;
                justify-content: flex-start;
                height: 100dvh;
                width: 100dvw;
                
                .camera-view {
                    display: flex;
                    flex-direction: row;
                    align-items: center;
                    justify-content: center;

                    width: 100dvw;
                    height: 80dvh;

                    img {
                        width: 100%;
                        height: 100%;
                        border-style: solid;
                        border-width: 0.5em;
                        border-radius: 0.5em;
                        border-color: darkgray;
                        user-select: none;
                    }
                }

                .commands {
                    display: flex;
                    flex-direction: column;
                    gap: 1em;
                    align-items: center;
                    justify-content: center;

                    padding: 1em;

                    height: 0.1dvh;
                    width: 100dvw;

                    .status-box {
                        padding: 1em;
                        background-color: lightgray;
                        color: black;
                        border-style: solid;
                        border-width: 0.5em;
                        border-color: darkgray;
                        user-select: none;
                    }

                    .joystick {
                        position: relative;
                        background-color: aquamarine;
                        border-color: black;
                        border-style: solid;
                        min-width: 100px;
                        min-height: 100px;
                        height: 100px;

                        .cross {
                            top: 0%;
                            left: 0%;
                            width: 100%;
                            height: 100%;
                            user-select: none;
                            position: absolute;
                        }

                        .joystick-thumb {
                            cursor: pointer;
                            position: absolute;
                            width: 20px;
                            height: 20px;
                            border-radius: 10px;
                            background-color: black;

                            top: calc(50% - 10px);
                            left: calc(50% - 10px);
                        }
                    }
                }
            }
        </style>
        <script language='javascript'>
            let joyClicked = false;
            

            document.addEventListener('DOMContentLoaded', () => {
                console.log('initialized');

                const espIp = '192.168.1.6';
                const espStreamPort = '8000';
                const websocketPort = '9000';
                const streamUrl = `http://${espIp}:${espStreamPort}/image\\`; 
                const websocketUrl = `ws://${espIp}:${websocketPort}/socket`;
                const mainPadding = 5;

                console.log('stream url', streamUrl);

                const thumb = document.getElementById('joystick-thumb');
                const thumbContainer = document.getElementById('joystick');
                const camera = document.getElementById('camera');
                const status = document.getElementById('status-box');

                camera.setAttribute('src', streamUrl);

                let websocket = null;
                let websocketOpened = false;

                const setStatus = (text) => {
                    status.innerHTML = text;
                }

                const openWebsocket = () => {
                    try{
                        setStatus('Status: conectando....');
                        websocket = new WebSocket(websocketUrl);
                        
                        websocket.onopen = (event) => {
                            setStatus('Status: websocket conectado');
                            websocketOpened = true;
                        };

                        // This triggers if the connection fails or an error occurs
                        websocket.onerror = (error) => {
                            setStatus('Status: erro no websocket');
                            websocketOpened = false;
                        };
                    }catch(err) {
                        console.log('Websocket failed!');
                        websocket = null;
                        websocketOpened = false;
                    }
                };

                const setVerticalView = () => {
                    console.log('SET VERTICAL VIEW');
                    const panel = document.getElementById('control-panel');
                    panel.style.setProperty('flex-direction', 'column');
                    panel.style.setProperty('width', `${100 - mainPadding}dvw`);
                    panel.style.setProperty('height', `${100 - mainPadding}dvh`);

                    const camera = document.getElementById('camera-view');
                    camera.style.setProperty('width', `${100 - mainPadding}dvw`);
                    camera.style.setProperty('height', `${70 - mainPadding}dvh`);

                    const controls = document.getElementById('commands');
                    controls.style.setProperty('width', `${100 - mainPadding}dvw`);
                    controls.style.setProperty('height', `${30 - mainPadding}dvw`);

                    thumbContainer.style.setProperty('width', '15dvw');
                    thumbContainer.style.setProperty('height', '15dvw');
                    thumbContainer.style.setProperty('max-width', '15dvw');
                    thumbContainer.style.setProperty('max-height', '15dvw');

                    thumb.style.setProperty('width', '5dvw');
                    thumb.style.setProperty('height', '5dvw');
                    thumb.style.setProperty('border-radius', '2.5dvw');

                    resetThumbPosition();
                }

                const setHorizontalView = () => {
                    console.log('SET HORIZONTAL VIEW');
                    const panel = document.getElementById('control-panel');
                    panel.style.setProperty('flex-direction', 'row');
                    panel.style.setProperty('width', `${100 - mainPadding}dvw`);
                    panel.style.setProperty('height', `${100 - mainPadding}dvh`);

                    const camera = document.getElementById('camera-view');
                    camera.style.setProperty('width', `${70 - mainPadding}dvw`);
                    camera.style.setProperty('height', `${100 - mainPadding}dvh`);

                    const controls = document.getElementById('commands');
                    controls.style.setProperty('width', `${30 - mainPadding}dvw`);
                    controls.style.setProperty('height', `${100 - mainPadding}dvh`);

                    thumbContainer.style.setProperty('width', '15dvw');
                    thumbContainer.style.setProperty('height', '15dvw');
                    thumbContainer.style.setProperty('max-width', '15dvw');
                    thumbContainer.style.setProperty('max-height', '15dvw');

                    thumb.style.setProperty('width', '5dvw');
                    thumb.style.setProperty('height', '5dvw');
                    thumb.style.setProperty('border-radius', '2.5dvw');

                    resetThumbPosition();
                }

                const sendWebsocket = (data) => {
                    console.log('Sending', data);
                    if (websocketOpened) {
                        try{
                            websocket.send(data);
                        }catch(err) {
                            openWebsocket();
                        }
                    }
                }


                const resetThumbPosition = () => {
                    const rect = thumbContainer.getBoundingClientRect();
                    const thumbRect = thumb.getBoundingClientRect();

                    thumb.style.setProperty('top', `calc(50% - ${thumbRect.width/2}px)`);
                    thumb.style.setProperty('left', `calc(50% - ${thumbRect.height/2}px)`);

                    sendWebsocket('X0Y0');
                };

                const vectorNorm = (vectX, vectY) => {
                    return Math.sqrt(vectX*vectX + vectY*vectY);
                }

                const updatePosition = (eventX, eventY) => {
                    const rect = thumbContainer.getBoundingClientRect();
                    const thumbRect = thumb.getBoundingClientRect();
                    const contX = rect.left + rect.width/2;
                    const contY = rect.top + rect.height/2;

                    const width = rect.width;
                    const height = rect.height;

                    let vectX = eventX - contX;
                    let vectY = eventY - contY;

                    const norm = vectorNorm(vectX, vectY);
                    const maxNorm = Math.min(rect.height/2, rect.width/2);


                    if (norm > maxNorm) {
                        vectX = vectX*maxNorm/norm;
                        vectY = vectY*maxNorm/norm;
                    }

                    const vectPercentX = vectX/maxNorm;
                    const vectPercentY = vectY/maxNorm;

                    if (joyClicked) {
                        const strTop = `calc(50% ${vectY > 0 ? '+': '-'} ${Math.abs(vectY)}px - ${thumbRect.height/2}px)`;
                        const strLeft = `calc(50% ${vectX > 0 ? '+': '-'} ${Math.abs(vectX)}px - ${thumbRect.width/2}px`;
                        console.log('VECT PERCENT', vectPercentX, vectPercentY);

                        thumb.style.setProperty('top', strTop);
                        thumb.style.setProperty('left', strLeft);

                        sendWebsocket(`X${vectPercentX.toFixed(2)}Y${vectPercentY.toFixed(2)}`);
                    }
                }

                document.addEventListener('pointerdown', (event) => {
                    const elem = document.elementFromPoint(event.clientX, event.clientY);
                    if (elem === thumb) {
                        joyClicked = true;
                    }
                });
                document.addEventListener('pointerup', (event) => {
                    joyClicked = false;
                    resetThumbPosition();
                });
                document.addEventListener('pointermove', (event) => {
                    updatePosition(event.clientX, event.clientY);
                });

                adjustOrientations = () => {
                    if (!screen.orientation.type.includes('landscape')) {
                        setVerticalView();
                    } else {
                        setHorizontalView();
                    }
                }
                screen.orientation.addEventListener('change', () => {
                    console.log(`Current orientation: ${screen.orientation.type}`);
                    adjustOrientations();
                });

                adjustOrientations();
                openWebsocket();
                resetThumbPosition();
            });
        </script>
    </head>
    <body>
        <div class='control-panel' id='control-panel'>
            <div class='camera-view' id='camera-view'>
                <img id='camera'>
            </div>
            <div class='commands' id='commands'>
                <div class='status-box' id='status-box'>

                </div>

                <div class='joystick' id='joystick'>
                    <svg class='cross' viewBox='0 0 100 100' style='background-color: white'>
                        <circle cx='50' cy='50' r='50' style='stroke: red; fill: white'></circle>
                        <line x1='0' y1='50' x2='100' y2='50' style='stroke: red'></line>
                        <line x1='50' y1='0' x2='50' y2='100' style='stroke: red'></line>
                    </svg>
                    <div class='joystick-thumb' id='joystick-thumb'>

                    </div>
                </div>
            </div>
        </div>
    </body>
</html>
)rawliteral";


#define SERVER_PORT 8000
#define WEBSOCKET_SERVER_PORT 9000
#define IMAGE_QUALITY 40
#define MAX_CLIENTS 10

//CAMERA PINS
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

//MOTOR CONTROL PINS
#define MOTOR1_PIN1 -1
#define MOTOR1_PIN2 -1
#define MOTOR2_PIN1 -1
#define MOTOR2_PIN2 -1
#define MAX_PWM 100

enum ResourceRequested{
  UNDEFINED,
  STREAM,
  COMMAND
};
typedef enum ResourceRequested ResourceRequested;

struct ClientPool {
  WiFiClient clients[MAX_CLIENTS];
  ResourceRequested resource_requested[MAX_CLIENTS];
  String latest_line[MAX_CLIENTS];
};
typedef struct ClientPool ClientPool;

void acceptClient(ClientPool *pool, WiFiServer *server) {
  for(int i=0; i<MAX_CLIENTS; i++) {
    if (!pool->clients[i]) {
      pool->clients[i] = server->available();
      if (pool->clients[i]) {
        pool->resource_requested[i] = UNDEFINED;
        pool->latest_line[i] = "";
      }
      break;
    }
  }  
}

void removeClient(ClientPool *pool, int index) {
  if(pool->clients[index]) {
    Serial.println("REMOVING AT");
    Serial.println(index);
    pool->clients[index].stop();
    pool->resource_requested[index] = UNDEFINED;
    pool->latest_line[index] = "";
  }
}

const char* SSID = "roraima_robot";
const char* PASSWORD = "test_password";
int OWN_IP[4] = {192,168,1,6};
WiFiServer server(SERVER_PORT);
ClientPool CLIENT_POOL;


AsyncWebServer wsserver(WEBSOCKET_SERVER_PORT);
AsyncWebSocket ws("/socket");

void setup_access_points(const char* ssid, const char* password) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println("Access point created");
  IPAddress Ip(OWN_IP[0], OWN_IP[1], OWN_IP[2], OWN_IP[3]);
  IPAddress NMask(255, 255, 255, 0);
  WiFi.softAPConfig(Ip, Ip, NMask);
}

void setup_camera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG; 
  
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 40;
  config.fb_count = 2;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("Camera init failed...");
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  if (s != NULL) {
    s->set_exposure_ctrl(s, 1);
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
  }

  Serial.println("Camera initialized");
}

inline void send_html_http_header(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println();
}

inline void send_start_stream(WiFiClient client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=--frame");
  client.println("Cache-Control: no-cache");
  client.println(); // End of headers
}

inline int send_frame_stream(WiFiClient client) {
  camera_fb_t *fb = NULL;
  uint8_t *_jpg_buf = NULL;
  size_t _jpg_buf_len = 0;
  char *part_buf[64];

  fb = esp_camera_fb_get();
  if (!fb) return 0;
  if (fb->format != PIXFORMAT_JPEG) {
    bool converted = frame2jpg(fb, IMAGE_QUALITY, &_jpg_buf, &_jpg_buf_len);
    esp_camera_fb_return(fb);
    fb = NULL;
    if (!converted) {
      return 0;
    }
  } else {
    _jpg_buf_len = fb->len;
    _jpg_buf = fb->buf;
    esp_camera_fb_return(fb);
  }

  //client send data
  client.print("\r\n--frame\r\n");
  client.print("Content-Type: image/jpeg\r\n");
  client.print("Content-Length: ");
  client.print(_jpg_buf_len);
  client.print("\r\n\r\n");
  client.write(_jpg_buf, _jpg_buf_len);
  
  delay(30);
  return 1;
}

void server_loop() {
  acceptClient(&CLIENT_POOL, &server);

  for(int i=0; i<MAX_CLIENTS; i++) {
    WiFiClient client = CLIENT_POOL.clients[i];
    if (client && client.connected()) {
      //Serial.print("CLIENT CONNECTED AT ");
      //Serial.println(i);

      char c;

      switch(CLIENT_POOL.resource_requested[i]) {
        case STREAM:
          //Serial.println("STREAM");
          send_frame_stream(client);
          break;
        case COMMAND:
          if (!client.available())break;
          while(client.available()) {
            c = client.read();  
            //Serial.print("FROM COMMAND>");
            Serial.println(c);
          }
          break;
        default:
          if (!client.available())break;
          while(client.available()) {
            c = client.read();
            CLIENT_POOL.latest_line[i] += c;
            if (c == '\n') {
              if (CLIENT_POOL.latest_line[i].indexOf("/image") > 0) {
                //Serial.println("IS STREAM");
                CLIENT_POOL.resource_requested[i] = STREAM;
                send_start_stream(client);
                break;
              } else if (CLIENT_POOL.latest_line[i].indexOf("/command") > 0) {
                CLIENT_POOL.resource_requested[i] = UNDEFINED;
              } else  if(CLIENT_POOL.latest_line[i].indexOf("/index") > 0){
                send_html_http_header(client);
                client.println(LANDING_PAGE);
                removeClient(&CLIENT_POOL, i);
              }
            }
          }
          //Serial.println(CLIENT_POOL.latest_line[i]);
          
      }
    } else{
      /*Serial.print("DISCONNECTING");
      Serial.println(i);*/
      removeClient(&CLIENT_POOL, i);
    }
  }
}

void setup_motor_pins() {
  if (MOTOR1_PIN1 > 0 && MOTOR1_PIN2 > 0 && MOTOR2_PIN1 > 0 && MOTOR2_PIN2 > 0) {
    pinMode(MOTOR1_PIN1, OUTPUT);
    pinMode(MOTOR1_PIN2, OUTPUT);
    pinMode(MOTOR2_PIN1, OUTPUT);
    pinMode(MOTOR2_PIN2, OUTPUT);
  }
}

void set_motor1_speed(float speed) {
  int pwm = (int)(speed*255.0);
  if (pwm > 0) {
    analogWrite(MOTOR1_PIN1, pwm);
    analogWrite(MOTOR1_PIN2, 0);
  } else {
    analogWrite(MOTOR1_PIN1, 0);
    analogWrite(MOTOR1_PIN2, pwm);
  }
}

void set_motor2_speed(float speed) {
  int pwm = (int)(speed*255.0);
  if (pwm > 0) {
    analogWrite(MOTOR2_PIN1, pwm);
    analogWrite(MOTOR2_PIN2, 0);
  } else {
    analogWrite(MOTOR2_PIN1, 0);
    analogWrite(MOTOR2_PIN2, pwm);
  }
}

void setup_motor_speed(float speedX, float speedY) {
  float motor_1 = speedY;
  float motor_2 = speedY;
  if (speedY > 0) {
    if (speedX > 0) {
      motor_1 += speedX;
    } else {
      motor_2 += -speedX;
    }
  } else {
    if (speedX > 0) {
      motor_2 -= speedX;
    } else {
      motor_1 += speedX;
    }
  }
  set_motor1_speed(motor_1);
  set_motor2_speed(motor_2); 
}

void read_websocket_message(char* message) {
  float vectX = 0;
  float vectY = 0;
  sscanf(message, "X%fY%f", &vectX, &vectY);

  Serial.println("Received vector data");
  Serial.print("VECT X: ");
  Serial.println(vectX, DEC);
  Serial.print("VECT Y: ");
  Serial.println(vectY, DEC);

  setup_motor_speed(vectX, vectY);
}

void on_ws_event(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("Client connected: %u\n", client->id());
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("Client disconnected: %u\n", client->id());
  } else if (type == WS_EVT_DATA) {
    String msg = "";
    for(size_t i=0; i < len; i++) {
      msg += (char)data[i];
    }
    read_websocket_message((char*)msg.c_str());
  }
}

void setup_websocket_server() {
  ws.onEvent(on_ws_event);
  wsserver.addHandler(&ws);
  wsserver.begin();
}

void setup_http_server() {
  server.begin();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing camera...");
  setup_camera();
  
  Serial.println("Creating access point...");
  setup_access_points(SSID, PASSWORD);

  Serial.println("ROBOT IP");
  Serial.println(WiFi.softAPIP());

  setup_http_server();
  setup_websocket_server();
  setup_motor_pins();
}

void loop() {
  server_loop();
}
