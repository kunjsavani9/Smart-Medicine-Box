# AUSHADH — Smart Medicine Box: Wiring & Connections

Wiring reference for the ESP32-based smart medicine box. All logic runs at **3.3 V** — do **not** feed 5 V into any ESP32 GPIO.

> ⚠️ **Pin numbers below are the typical mapping.** Confirm they match the `#define` / pin declarations at the top of your Arduino sketch and adjust either the wiring or the code so they agree.

---

## Components

| # | Component | Interface |
|---|-----------|-----------|
| 1 | ESP32 Dev Board | Main controller (Wi-Fi + logic) |
| 2 | 4×4 Membrane Keypad | 8 digital I/O |
| 3 | 16×2 LCD + I2C backpack | I2C (SDA/SCL) |
| 4 | Passive Buzzer | 1 digital I/O |
| 5 | FSR 402 Force Sensor | 1 analog input + 10 kΩ resistor |
| 6 | Power source | USB / battery (see below) |

---

## 1. FSR 402 Force Sensor (pill detection)

Wired as a voltage divider. **GPIO34 is input-only** — correct for analog reading.

| FSR / Divider | Connects to |
|---------------|-------------|
| FSR leg 1 | **3.3 V** |
| FSR leg 2 | **GPIO34** *and* top of 10 kΩ resistor |
| 10 kΩ resistor (bottom) | **GND** |

```
3.3V ──[ FSR ]──┬── GPIO34  (analog in)
                │
             [ 10kΩ ]
                │
               GND
```

**Logic:** reading `0` = medicine absent → buzzer activates; reading `≥ 2500` = medicine present → buzzer stops.

---

## 2. 16×2 LCD (I2C backpack)

| LCD backpack | ESP32 |
|--------------|-------|
| VCC | **5 V (VIN)** *(most I2C backpacks need 5 V; the SDA/SCL lines stay 3.3 V-safe)* |
| GND | GND |
| SDA | **GPIO21** |
| SCL | **GPIO22** |

> If your LCD stays blank, check the I2C address in code (commonly `0x27` or `0x3F`) and confirm SDA/SCL aren't swapped.

---

## 3. 4×4 Membrane Keypad

Eight ribbon pins — 4 rows + 4 columns. From pin 1 (leftmost) to pin 8:

| Keypad pin | Signal | ESP32 GPIO |
|------------|--------|------------|
| 1 | Row 1 | **GPIO13** |
| 2 | Row 2 | **GPIO14** |
| 3 | Row 3 | **GPIO27** |
| 4 | Row 4 | **GPIO26** |
| 5 | Col 1 | **GPIO25** |
| 6 | Col 2 | **GPIO33** |
| 7 | Col 3 | **GPIO32** |
| 8 | Col 4 | **GPIO4** |

No external resistors needed — the `Keypad` library uses the ESP32's internal pull-ups.

---

## 4. Passive Buzzer

| Buzzer | ESP32 |
|--------|-------|
| + (signal) | **GPIO23** |
| − | GND |

> A passive buzzer needs a driven tone signal (e.g. `tone()` / PWM). If you have an **active** buzzer instead, a plain HIGH/LOW toggle is enough.

---

## 5. Power

**Option A — USB power bank (simplest):** plug into the ESP32's USB port. Onboard regulator supplies 3.3 V. Recommended for demos.

**Option B — Rechargeable battery pack:**

```
18650 cell ──> TP4056 (charge/protect) ──> MT3608 (boost to 5V) ──> ESP32 5V (VIN)
```

- **TP4056**: safe Li-ion charging + protection.
- **MT3608**: boost the ~3.7 V cell up to 5 V; set the output with its trim pot **before** connecting the ESP32.

---

## Common Ground

**All GNDs must be tied together** — ESP32, LCD, keypad, buzzer, FSR divider, and power source share one common ground. Missing common ground is the most frequent cause of erratic readings.

---

## Quick Pin Summary

| ESP32 Pin | Used by |
|-----------|---------|
| GPIO34 | FSR analog in (input-only) |
| GPIO21 / GPIO22 | LCD I2C SDA / SCL |
| GPIO13, 14, 27, 26 | Keypad rows 1–4 |
| GPIO25, 33, 32, 4 | Keypad cols 1–4 |
| GPIO23 | Buzzer |
| 3.3 V | FSR leg 1 |
| 5 V (VIN) | LCD VCC, boost-converter output |
| GND | Common ground (all modules) |

*Avoid GPIO 0, 2, 5, 12, 15 for I/O — they are strapping/boot pins and can prevent the ESP32 from booting if pulled the wrong way.*
