#!/bin/bash

# load .env
if [ -f .env ]; then
    export $(grep -v '^#' ../.env | xargs)
else
    echo "Failed to load .env"
    exit 1
fi

# IP
IP_FILE="/tmp/last_ip.txt"

# get the current public IP
NEW_IP=$(curl -s ifconfig.me)

# check if there is an old IP
if [ -f "$IP_FILE" ]; then
    OLD_IP=$(cat "$IP_FILE")
else
    OLD_IP=""
fi

# check if IP has changed
if [ "$NEW_IP" != "$OLD_IP" ]; then
    echo "IP changed: ($OLD_IP -> $NEW_IP)"
    echo "You better change the public IP as '$NEW_IP' for UPbit API"
    echo "https://upbit.com/mypage/open_api_management"

    # change new IP
    echo "$NEW_IP" > "$IP_FILE"

    # email-alarm
    echo "IP has changed: $NEW_IP" | mail -s "IP has changed. You better update IP for UPbit API" gjk.100.010.110@gmail.com

    # telegram-alarm
    curl -s -X POST "https://api.telegram.org/bot$TELEGRAM_BOT_TOKEN/sendMessage" \
        -d "chat_id=$TELEGRAM_CHAT_ID" \
        -d "text=IP has changed ($OLD_IP -> $NEW_IP)%0A https://upbit.com/mypage/open_api_management"

else
    echo "IP hasn't changed ($NEW_IP)"
fi
