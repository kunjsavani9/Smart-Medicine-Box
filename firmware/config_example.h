/*
 * config_example.h  —  Credentials template for AUSHADH Smart Medicine Box
 *
 * HOW TO USE:
 *   1. Copy this file and rename the copy to  config.h
 *   2. Fill in your own values below.
 *   3. config.h is git-ignored, so your secrets stay off GitHub.
 *   4. In your sketch, add:  #include "config.h"
 *
 * DO NOT put real credentials in this file — it is committed to the repo.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ---- Wi-Fi ----
#define WIFI_SSID        "your_wifi_name"
#define WIFI_PASSWORD    "your_wifi_password"

// ---- Blynk IoT ----
#define BLYNK_TEMPLATE_ID    "your_template_id"
#define BLYNK_TEMPLATE_NAME  "your_template_name"
#define BLYNK_AUTH_TOKEN     "your_blynk_auth_token"

// ---- Telegram Bot ----
// Get the token from @BotFather; get your chat ID from @userinfobot
#define TELEGRAM_BOT_TOKEN   "your_bot_token"
#define TELEGRAM_CHAT_ID     "your_chat_id"

#endif  // CONFIG_H
