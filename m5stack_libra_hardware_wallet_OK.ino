#include <M5Stack.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

#define SERVICE_UUID        "c03e7090-7ce0-46f0-98dd-a2aba8367741"
#define CHARACTERISTIC_UUID "26e2b12b-85f0-4f3f-9fdd-91d114270e6e"

#define WIFI_STA_NAME "Ice iPhoneX"
#define WIFI_STA_PASS "ice123456"

String page;

//----- BLE Variables -------//
BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      M5.Lcd.println("Connect");
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      M5.Lcd.println("Disconnect");
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *pCharacteristic) {
    //M5.Lcd.println("Read");
    pCharacteristic->setValue("Hello World!");
  }
  
  void onWrite(BLECharacteristic *pCharacteristic) {
    //M5.Lcd.println("Write");
    std::string value = pCharacteristic->getValue();
    M5.Lcd.println(value.c_str());
  }
};
//-----------------------------------------------//

//----- Keyboard Variables -------//
char keymap[12] = {'<', '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '/'};

int csel = 0;
int ocsel = -1;
String keystring;
String okeystring;
//--------------------------------//

//---- EEPROM Variables -----//
int password_address = 0;       // Password LEN = 65
int wallet_address = 100;       // Wallet_address LEN = 65
int wallet_balance = 200;       // Wallet_balance
int wallet_key_address_len = 250;   // Wallet_key LEN = 153
int wallet_key_address = 300;   // Wallet_key LEN = 153
//---------------------------//

//---- SHA256 Variables -----//
char hex[256];
uint8_t data[256];
int start = 0;
int seconds = 0;
uint8_t hash[32];
String pin;
#define SHA256_BLOCK_SIZE 32

typedef struct {
  uint8_t data[64];
  uint32_t datalen;
  unsigned long long bitlen;
  uint32_t state[8];
} SHA256_CTX;

void sha256_init(SHA256_CTX *ctx);
void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len);
void sha256_final(SHA256_CTX *ctx, uint8_t hash[]);

#define ROTLEFT(a,b) (((a) << (b)) | ((a) >> (32-(b))))
#define ROTRIGHT(a,b) (((a) >> (b)) | ((a) << (32-(b))))

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTRIGHT(x,2) ^ ROTRIGHT(x,13) ^ ROTRIGHT(x,22))
#define EP1(x) (ROTRIGHT(x,6) ^ ROTRIGHT(x,11) ^ ROTRIGHT(x,25))
#define SIG0(x) (ROTRIGHT(x,7) ^ ROTRIGHT(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTRIGHT(x,17) ^ ROTRIGHT(x,19) ^ ((x) >> 10))

static const uint32_t k[64] = {
  0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
  0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
  0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
  0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
  0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
  0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
  0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
  0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
//--------------------------------//

void sha256_transform(SHA256_CTX *ctx, const uint8_t data[]) {
  uint32_t a, b, c, d, e, f, g, h, i, j, t1, t2, m[64];

  for (i = 0, j = 0; i < 16; ++i, j += 4)
    m[i] = ((uint32_t)data[j] << 24) | ((uint32_t)data[j + 1] << 16) | ((uint32_t)data[j + 2] << 8) | ((uint32_t)data[j + 3]);
  for ( ; i < 64; ++i)
    m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];

  a = ctx->state[0];
  b = ctx->state[1];
  c = ctx->state[2];
  d = ctx->state[3];
  e = ctx->state[4];
  f = ctx->state[5];
  g = ctx->state[6];
  h = ctx->state[7];

  for (i = 0; i < 64; ++i) {
    t1 = h + EP1(e) + CH(e,f,g) + k[i] + m[i];
    t2 = EP0(a) + MAJ(a,b,c);
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }

  ctx->state[0] += a;
  ctx->state[1] += b;
  ctx->state[2] += c;
  ctx->state[3] += d;
  ctx->state[4] += e;
  ctx->state[5] += f;
  ctx->state[6] += g;
  ctx->state[7] += h;
}

void sha256_init(SHA256_CTX *ctx)
{
  ctx->datalen = 0;
  ctx->bitlen = 0;
  ctx->state[0] = 0x6a09e667;
  ctx->state[1] = 0xbb67ae85;
  ctx->state[2] = 0x3c6ef372;
  ctx->state[3] = 0xa54ff53a;
  ctx->state[4] = 0x510e527f;
  ctx->state[5] = 0x9b05688c;
  ctx->state[6] = 0x1f83d9ab;
  ctx->state[7] = 0x5be0cd19;
}

void sha256_update(SHA256_CTX *ctx, const uint8_t data[], size_t len) {
  uint32_t i;

  for (i = 0; i < len; ++i) {
    ctx->data[ctx->datalen] = data[i];
    ctx->datalen++;
    if (ctx->datalen == 64) {
      sha256_transform(ctx, ctx->data);
      ctx->bitlen += 512;
      ctx->datalen = 0;
    }
  }
}

void sha256_final(SHA256_CTX *ctx, uint8_t hash[]) {
  uint32_t i;

  i = ctx->datalen;

  // Pad whatever data is left in the buffer.
  if (ctx->datalen < 56) {
    ctx->data[i++] = 0x80;
    while (i < 56)
      ctx->data[i++] = 0x00;
  }
  else {
    ctx->data[i++] = 0x80;
    while (i < 64)
      ctx->data[i++] = 0x00;
    sha256_transform(ctx, ctx->data);
    memset(ctx->data, 0, 56);
  }

  // Append to the padding the total message's length in bits and transform.
  ctx->bitlen += ctx->datalen * 8;
  ctx->data[63] = ctx->bitlen;
  ctx->data[62] = ctx->bitlen >> 8;
  ctx->data[61] = ctx->bitlen >> 16;
  ctx->data[60] = ctx->bitlen >> 24;
  ctx->data[59] = ctx->bitlen >> 32;
  ctx->data[58] = ctx->bitlen >> 40;
  ctx->data[57] = ctx->bitlen >> 48;
  ctx->data[56] = ctx->bitlen >> 56;
  sha256_transform(ctx, ctx->data);

  // Since this implementation uses little endian byte ordering and SHA uses big endian,
  // reverse all the bytes when copying the final state to the output hash.
  for (i = 0; i < 4; ++i) {
    hash[i]      = (ctx->state[0] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 4]  = (ctx->state[1] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 8]  = (ctx->state[2] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 12] = (ctx->state[3] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 16] = (ctx->state[4] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 20] = (ctx->state[5] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 24] = (ctx->state[6] >> (24 - i * 8)) & 0x000000ff;
    hash[i + 28] = (ctx->state[7] >> (24 - i * 8)) & 0x000000ff;
  }
}

char *btoh(char *dest, uint8_t *src, int len) {
  char *d = dest;
  while( len-- ) sprintf(d, "%02x", (unsigned char)*src++), d += 2;
  return dest;
}

String SHA256(String data) 
{
  uint8_t data_buffer[data.length()];
  
  for(int i=0; i<data.length(); i++)
  {
    data_buffer[i] = (uint8_t)data.charAt(i);
  }
  
  SHA256_CTX ctx;
  ctx.datalen = 0;
  ctx.bitlen = 512;
  
  sha256_init(&ctx);
  sha256_update(&ctx, data_buffer, data.length());
  sha256_final(&ctx, hash);
  return(btoh(hex, hash, 32));
}

String EEPROM_read(int index, int length) {
  String text = "";
  char ch = 1;
  
  for (int i = index; (i < (index + length)) && ch; ++i) {
    if (ch = EEPROM.read(i)) {
      text.concat(ch);
    }
  }
  return text;
}

int EEPROM_write(int index, String text) {
  for (int i = index; i < text.length() + index; ++i) {
    EEPROM.write(i, text[i - index]);
  }
  EEPROM.write(index + text.length(), 0);
  EEPROM.commit();
  
  return text.length() + 1;
}

void keyboard() {
  if ((csel != ocsel) || (keystring != okeystring))
  {
    M5.Lcd.fillRect(0, 120, 320, 32, BLACK);
    M5.Lcd.drawString(String(keystring), 110, 120, 4);

    int x, y;
    y = 180;
    M5.Lcd.fillRect(0, 0, 320, 50, BLACK);
    M5.Lcd.fillRect(0, y, 320, 32, BLACK);
    for (int c = 0; c < 12; c++)
    {
      x = (c * 26);
      if (csel == c)
      {
        //M5.Lcd.setTextColor(TFT_BLACK, TFT_WHITE);
        M5.Lcd.drawRect(x, y, 26, 32, MAGENTA);
        M5.Lcd.fillRect(x, y+25, 26, 7, WHITE);
      }
      else
      {
        M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLACK);
        M5.Lcd.drawRect(x, y, 26, 32, MAGENTA);
      }
      M5.Lcd.drawString(String(keymap[c]), x + 10, y + 7, 2);
    }
    ocsel = csel;
  }
}

void wallet() {

  if (EEPROM_read(wallet_address, 65).length() == 0) {
    String url = "https://libraservice2.kulap.io/createWallet";
    Serial.println();
    Serial.println("Creating Wallet.. " + url);
    M5.Lcd.drawString("** Creating Wallet **", 50, 120, 4);
     
    HTTPClient http;
    http.begin(url);
    int httpCode = http.POST("");
    if (httpCode == 200) {
      String response = http.getString();
  
      DynamicJsonDocument doc(2048);
      deserializeJson(doc, response);
  
      const String libra_address = doc["address"]; 
      const String libra_key = doc["mnemonic"]; 
      const String libra_balance = doc["balance"];
  
      EEPROM_write(wallet_key_address, libra_key);
      EEPROM_write(wallet_key_address_len, (String) libra_key.length());
      Serial.println(libra_key);
      Serial.println(libra_key.length());
      EEPROM_write(wallet_address, libra_address);
      EEPROM_write(wallet_balance, libra_balance);
      M5.Lcd.clear();
    } else {
      Serial.println("Fail. error code " + String(httpCode));
    }
    Serial.println("END");
  } else {
    const String libra_address = EEPROM_read(wallet_address, 65);
    const String libra_balance = EEPROM_read(wallet_balance, 100);
    M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLACK);
    M5.Lcd.drawString("Address:", 10, 50, 4);
    M5.Lcd.drawString(String(libra_address).substring(0,libra_address.length()/2), 10, 75, 2);
    M5.Lcd.drawString(String(libra_address).substring((libra_address.length()/2),libra_address.length()), 10, 90, 2);
    M5.Lcd.drawString("Balance:", 10, 120, 4);
    M5.Lcd.drawString(libra_balance, 10, 145, 2);
  }
}

void balance(String libra_address) {

  String url = "https://libraservice2.kulap.io/getBalance";
  Serial.println();
  Serial.println("Getting Balance.. " + url + "/" + libra_address);

  StaticJsonDocument<256> postData;
  JsonObject root = postData.to<JsonObject>();
  root["address"] = libra_address;
  char JsonMessage[100];
  serializeJsonPretty(root, JsonMessage);
  Serial.println(JsonMessage);
  
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpCode = http.POST(JsonMessage);
  if (httpCode == 200) {
    String response = http.getString();

    DynamicJsonDocument doc(2048);
    deserializeJson(doc, response);

    const String libra_balance = doc["balance"];
    EEPROM_write(wallet_balance, libra_balance);
  
  } else {
    Serial.println("Fail. error code " + String(httpCode));
  }
  M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLACK);
  const String libra_balance = EEPROM_read(wallet_balance, 100);
  M5.Lcd.drawString("Balance:", 10, 120, 4);
  M5.Lcd.drawString(libra_balance, 10, 145, 2);
}

void setup(void) {
  M5.begin();
  EEPROM.begin(512);
  
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(1);
  M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLACK);
  M5.Lcd.setBrightness(100);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_STA_NAME, WIFI_STA_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  BLEDevice::init("libra-hw-wallet");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY |
                                         BLECharacteristic::PROPERTY_INDICATE
                                       );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
  
  page = "login";
}

void loop() {

  if (page == "login") {
    M5.Lcd.drawString("Libra HW Wallet", 70, 20, 4);
    M5.Lcd.drawString("Please Enter PIN", 70, 50, 4);
    M5.Lcd.drawString("Left", 50, 220, 2);
    M5.Lcd.drawString("OK", 150, 220, 2);
    M5.Lcd.drawString("Right", 250, 220, 2);
  
    if (M5.BtnA.pressedFor(5000)) {
      keystring = "";
      M5.Lcd.clear();
    } else if (M5.BtnA.wasReleased()) {
      csel = csel - 1;
      if (csel < 0) {
        csel = 11;
      }
    }
    if (M5.BtnC.wasReleased()) {
      csel = csel + 1;
      if (csel > 11)
      {
        csel = 0;
      }
    } else if (M5.BtnC.pressedFor(20000)) {      
      EEPROM_write(password_address, "");
      EEPROM_write(wallet_address, "");
      EEPROM_write(wallet_key_address, "");
      Serial.println("Cleared");
      M5.Lcd.drawString("** CLEARED **", 80, 120, 4);
      M5.Lcd.clear();
      keystring = "";
      page = "login";
    }
    if (M5.BtnB.wasReleased()) {
      if (keymap[csel] == '<') {
        keystring = keystring.substring(0, keystring.length() -1);
      } else if (keystring.length() < 6 && keymap[csel] != '/') {
        keystring += keymap[csel];
      } else if (keystring.length() < 6 && keymap[csel] == '/') {
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
        M5.Lcd.drawString("** PIN is 6 digits **", 110, 100, 2);
      } else if (keymap[csel] == '/') {
        String sha = SHA256(keystring);
  
        if (EEPROM_read(password_address, 65).length() == 0) {
          M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
          M5.Lcd.drawString("** Creating PIN **", 50, 120, 4);
          EEPROM_write(password_address, sha);
          delay(2000);
          keystring = "";
          M5.Lcd.clear();
        } else {
          if (EEPROM_read(password_address, 65) == sha) {
            M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
            M5.Lcd.drawString("** Updating Wallet **", 50, 120, 4);
            const String libra_address = EEPROM_read(wallet_address, 65);
            balance(libra_address);
            page = "wallet";
            M5.Lcd.clear();
          } else {
            M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
            M5.Lcd.drawString("** Incorrect PIN **", 110, 100, 2);
            delay(2000);
            keystring = "";
            M5.Lcd.clear();
          }
        }
      }
    }
    
    keyboard();
    okeystring = keystring;
    
    delay(1);

  } else if (page == "wallet") {
    const String libra_address = EEPROM_read(wallet_address, 65);
    //const String libra_mnemonic = EEPROM_read(wallet_key_address, 160);
    wallet();
    
    M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLACK);
    M5.Lcd.drawString("Libra HW Wallet", 70, 20, 4);    
    M5.Lcd.drawString("Show QR", 40, 220, 2);
    M5.Lcd.drawString("Sign Trx", 140, 220, 2);
    M5.Lcd.drawString("Logout", 230, 220, 2);
    
    if (M5.BtnA.wasReleased()) {
      M5.Lcd.clear();
      M5.Lcd.qrcode(libra_address);
      page = "qrcode";
    }
    if (M5.BtnB.wasReleased()) {
      M5.Lcd.clear();
      M5.Lcd.setTextSize(2);
      M5.Lcd.println("Please connect BLE !!");
      M5.Lcd.drawString("Close", 40, 220, 2);
      page = "signtrx";
    }
    if (M5.BtnC.wasReleased()) {
      M5.Lcd.clear();
      keystring = "";
      page = "login";
    }
    if (M5.BtnC.pressedFor(20000)) {
      EEPROM_write(password_address, "");
      EEPROM_write(wallet_address, "");
      EEPROM_write(wallet_key_address, "");
      M5.Lcd.drawString("** CLEARED **", 80, 120, 4);
      Serial.println("Cleared");
      M5.Lcd.clear();
      keystring = "";
      page = "login";
    } 

  } else if (page == "qrcode") {

    M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLACK);
    M5.Lcd.drawString("Close", 40, 220, 2);
     if (M5.BtnA.wasReleased()) {
      M5.Lcd.clear();
      page = "wallet";
     }   
  } else if (page == "signtrx") {
   
    const String libra_address = EEPROM_read(wallet_address, 65);
    const String libra_mnemonic_len = EEPROM_read(wallet_key_address_len, 3);
    const String libra_mnemonic = EEPROM_read(wallet_key_address, libra_mnemonic_len.toInt());
    M5.Lcd.setTextColor(TFT_MAGENTA, TFT_BLACK);
    M5.Lcd.setTextSize(2);
    if (deviceConnected) {
      if(M5.BtnB.wasPressed()) {
        M5.Lcd.println("Wallet Connected !!");
        Serial.println(libra_mnemonic);
        String valuetoBLE = libra_address + "|" + libra_mnemonic;
        char dataValue[220];
        valuetoBLE.toCharArray(dataValue,valuetoBLE.length()+1);
        //M5.Lcd.println(valuetoBLE);
        pCharacteristic->setValue((uint8_t *)dataValue, sizeof(dataValue));
        pCharacteristic->notify();
      } 
    } else {
      if (M5.BtnA.wasReleased()) {
        M5.Lcd.clear();
        M5.Lcd.setTextSize(1);
        page = "wallet";
       }  
    }
  }

  M5.update();

}
