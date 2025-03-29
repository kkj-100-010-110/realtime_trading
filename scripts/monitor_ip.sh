#!/bin/bash

# -----------------------------------------------------------------------------
# ENV + FUNC
# -----------------------------------------------------------------------------

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
# .env
ENV_FILE="${SCRIPT_DIR}/../.env"
# IP
IP_FILE="/tmp/last_ip.txt"

# function to send telegram message
send_telegram() {
	local MESSAGE="$1"
    RESPONSE=$(curl -s -X POST "https://api.telegram.org/bot$TELEGRAM_BOT_TOKEN/sendMessage" \
        -d "chat_id=$TELEGRAM_CHAT_ID" \
        -d "text=$MESSAGE" 2>/dev/null)
    [[ "$RESPONSE" != *"\"ok\":true"* ]] && echo "Telegram API Error: $RESPONSE"
}

# -----------------------------------------------------------------------------
# MAIN LOGIC
# -----------------------------------------------------------------------------

# load .env
if [ -f "$ENV_FILE" ]; then
    export $(grep -v '^#' "$ENV_FILE" | xargs)
else
    echo "Failed to load .env"
    exit 1
fi

# check IP
NEW_IP=$(curl -s ifconfig.me)
OLD_IP=$(cat "$IP_FILE" 2>/dev/null || echo "")

if [ "$NEW_IP" != "$OLD_IP" ]; then
    echo "IP changed: ($OLD_IP -> $NEW_IP)"
	send_telegram $'IP changed: '"$OLD_IP -> $NEW_IP"\
		$'\nUpdate UPbit API IP: '"$NEW_IP"\
		$'\nhttps://upbit.com/mypage/open_api_management'
    echo "You better change the public IP as '$NEW_IP' for UPbit API"
    # change new IP
    echo "$NEW_IP" > "$IP_FILE"
	echo "$(date "+%Y-%m-%d %H:%M:%S") - IP changed: $OLD_IP -> $NEW_IP"

else
	send_telegram "IP hasn't changed: $OLD_IP"
fi
