# Smart Medicine Box 💊

An ESP32-based smart medicine box that reminds users to take their medication on time, verifies whether medicine has actually been removed from the compartment, and sends real-time alerts via Telegram. Built with a keypad interface, LCD display, force sensor, and cloud connectivity through Blynk.

---

## ✨ Features

- **Scheduled alarms** — set medication times through a 4x4 keypad; a buzzer sounds when it's time to take medicine.
- **Take-verification** — an FSR (force sensor) detects whether the medicine has actually been removed from the compartment before silencing the alarm.
- **Telegram notifications** — real-time messages sent to your phone when an alarm triggers or medicine is taken/missed.
- **Blynk IoT dashboard** — monitor and control the box remotely over WiFi.
- **LCD status display** — 16x2 LCD (I2C) shows the clock, alarms, and system messages.
- **Password protection** — secure access to settings with a changeable PIN.
- **NTP time sync** — keeps accurate time over WiFi.

---

## 🔧 Hardware

| Component | Purpose |
|-----------|---------|
| ESP32 Dev Board | Main microcontroller (WiFi + processing) |
| 4x4 Membrane Keypad | User input (set alarms, enter password) |
| 16x2 LCD + I2C backpack | Display clock, alarms, messages |
| FSR 402 Force Sensor | Detect presence/removal of medicine |
| Passive Buzzer | Audible alarm |
| 10 kΩ Resistor | Pull-down for FSR analog reading |
| USB Power Bank *(or 18650 + TP4056 + MT3608)* | Power supply |

### Wiring notes

- **FSR** wired as: `3.3V → FSR leg 1`, `FSR leg 2 → GPIO34 + top of 10kΩ resistor`, `resistor bottom → GND`.
- **GPIO34 is input-only** on the ESP32 — used here for the analog FSR reading.
- FSR logic: reading of **0 = medicine absent** (buzzer activates); reading **≥ 2500 = medicine present** (buzzer stops).

See [`hardware/`](hardware/) for the full circuit diagram.

---

## 📁 Repository Structure

```
Smart-Medicine-Box/
├── firmware/          # ESP32 Arduino sketch (.ino) + config template
├── hardware/          # Circuit diagram and wiring notes
├── 3d-Models/         # Enclosure models — STL + SolidWorks source
│   ├── Base.STL / Base.SLDPRT
│   ├── Cap.STL  / Cap.SLDPRT
│   ├── Cover.STL / Cover.SLDPRT
│   └── Assem1.SLDASM        # Full assembly
├── docs/              # report, poster
├── images/            # Build and assembly photos, flow chart
├── LICENSE            # MIT
└── README.md
```

---

## 🚀 Getting Started

### 1. Prerequisites
- [Arduino IDE](https://www.arduino.cc/en/software) with the **ESP32 board package** installed
- Libraries: `WiFiClientSecure`, `Blynk`, `LiquidCrystal_I2C`, `Keypad`, and an NTP client

### 2. Configure your secrets
Copy the config template and fill in your own credentials:

```bash
cp firmware/config_example.h firmware/config.h
```

Edit `firmware/config.h`:

```cpp
#define WIFI_SSID       "your_wifi_name"
#define WIFI_PASSWORD   "your_wifi_password"
#define BLYNK_AUTH      "your_blynk_token"
#define TELEGRAM_TOKEN  "your_bot_token"
#define TELEGRAM_CHAT_ID "your_chat_id"
```

> ⚠️ **Never commit `config.h`** — it contains secrets and is git-ignored. Only `config_example.h` (with blanks) belongs in the repo.

### 3. Set up the Telegram bot
1. Message [@BotFather](https://t.me/BotFather) on Telegram and create a new bot to get your **token**.
2. Send your new bot a message, then get your **chat ID** (e.g. via [@userinfobot](https://t.me/userinfobot)).
3. Paste both into `config.h`.

### 4. Flash the board
Open `firmware/smart_medicine_box.ino` in the Arduino IDE, select your ESP32 board and port, and click **Upload**.

---

## 📖 Usage

- **Clock screen** is the default view (shows current time from NTP).
- **Set an alarm** using the keypad menu.
- When an alarm fires, the **buzzer sounds** and a **Telegram alert** is sent.
- Remove the medicine — the **FSR detects removal** and stops the buzzer.
- **Change password**: press `#` on the clock screen → `D` → enter old password → enter new password.

See [`docs/user_manual`](docs/) for the full keypad reference and LCD message guide.

---

## 🛠️ Troubleshooting

- **Telegram SSL `-5` error / `401 Unauthorized`** — usually an invalid bot token. Regenerate the token with BotFather and update `config.h`. The firmware uses `WiFiClientSecure` with `setInsecure()` to bypass certificate validation.
- **FSR reads erratically** — confirm the 10 kΩ pull-down resistor is wired correctly and you're using GPIO34.
- **LCD blank** — check the I2C address (commonly `0x27` or `0x3F`) and SDA/SCL wiring.

---

## 📄 License

Released under the [MIT License](LICENSE).

---

## 🙌 Acknowledgements

Built by Kunj as a hardware + software IoT project integrating ESP32, Telegram Bot API, and Blynk.
