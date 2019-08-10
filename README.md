# M5Stack Hardware Wallet

(First?) Libra Hardware wallet. Built on M5Stack devices: Core, Grey, Fire compatible using KULAP libra services

# Usages
1. Open m5stack_libra_hardware_wallet_OK.ino in Arduino IDE
2. Change SSID and Passphase for your WIFI (this is using for call libra services)
3. Upload codes into M5Stack
3. Enjoy !!

# Screens
## 1. Login page
  
  PIN is 6 digits and using SHA256 to encrypt PIN and store to EEPROM

  - Press 'A' keypad for shift PIN left
  - Press 'B' keypad for Enter PIN
  - Press 'C' keypad for shift PIN right
  - Hold 'C' keypad for 20 seconds to RESET all things  /* For test purpose only */..
  - Select '<' if you want to delete PIN
  - Select '/' once you have done enter PIN

## 2. Wallet Page

  Show Libra Address (Full address) with Balance (Offline storage in EEPROM)

  - Press 'A' keypad for show QR code to receive libra tokens
  - Press 'B' keypad for sign in or sign transaction with web wallet  /* Future function */..
  - Press 'C' keypad for logout
  - Hold 'C' keypad for 20 seconds to RESET all things  /* For test purpose only */..

# Libra Services

Libra Services from KULAP.io 
https://github.com/kulapio/libra-service

## Create wallet:
```POST /createWallet```

Headers: ```Content-Typeapplication/json```

Body: ```{}```

Example Request:
```
curl --location --request POST "https://libraservice2.kulap.io/createWallet" \
  --header "Content-Type: application/json" \
  --data "{}"
```

Example Output:
```
{
    "address": "5554d60c1af7592673f0ac012ce483b842c06de3d896029cfe957c348621d5b7",
    "balance": "100",
    "mnemonic": "parrot afraid always popular trade grape divide wave dawn web identify kangaroo equal suffer humor creek scan stove hip kingdom skin enable flush announce;1"
}
```
## Todo
- [X] Creating wallet and store keys in EEPROM
- [ ] Mnemonic phase Encryption with AES (or other techniques)
- [ ] BLE Connectivity and sign trx when login or transfer using web wallet
- [ ] WIFI Selector (now not user friendly need to fix SSID and key in the code)
- [ ] Offline mode without WIFI
- [ ] More functions to make it pure ledger nano / Trazor for Libra

# References:
- onScreenKyeboard: https://github.com/yellowelise/m5stack-onscreen-keyboard
- SHA256 lib: https://github.com/kamaljohnson/Arduino-SHA256
