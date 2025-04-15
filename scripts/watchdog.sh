#!/bin/bash

# -----------------------------------------------------------------------------
# ENV + FUNC
# -----------------------------------------------------------------------------

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
# .env
ENV_FILE="${SCRIPT_DIR}/../.env"
# timestamp
TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')

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

"${SCRIPT_DIR}/../program"
EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    send_telegram "[$TIMESTAMP] The program shutdown (exit code: $EXIT_CODE)"
else
    send_telegram "[$TIMESTAMP] The program shutdown"
fi
