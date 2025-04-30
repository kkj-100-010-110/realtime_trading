#!/bin/bash

# -----------------------------------------------------------------------------
# ENV + FUNC
# -----------------------------------------------------------------------------

SCRIPT_DIR="$(dirname "$(realpath "$0")")"
ENV_FILE="${SCRIPT_DIR}/../.env"

# function to send telegram message
send_telegram() {
    local MESSAGE="$1"
    local RESPONSE=$(curl -s -X POST "https://api.telegram.org/bot${TELEGRAM_BOT_TOKEN}/sendMessage" \
        -d "chat_id=${TELEGRAM_CHAT_ID}" \
        -d "text=${MESSAGE}" 2>/dev/null)
    [[ "$RESPONSE" != *"\"ok\":true"* ]] && echo "Telegram API Error: $RESPONSE"
}

# -----------------------------------------------------------------------------
# TRAPS
# -----------------------------------------------------------------------------

# normal exit
trap '{
    send_telegram "[EXIT] Program exited normally."
}' EXIT

# SSH disconnection (SIGHUP)
trap '{
    send_telegram "[SIGHUP] SSH disconnected. Program shutdown."
    exit 1
}' SIGHUP

# Termination signal (SIGTERM)
trap '{
    send_telegram "[SIGTERM] Program terminated."
    exit 1
}' SIGTERM

# Interrupt (Ctrl+C) (SIGINT)
trap '{
    send_telegram "[SIGINT] Program interrupted by user (Ctrl+C)."
    exit 1
}' SIGINT

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

# run your program
"${SCRIPT_DIR}/../program"
EXIT_CODE=$?

# -----------------------------------------------------------------------------
# AFTER PROGRAM EXIT
# -----------------------------------------------------------------------------

# clear traps temporarily
trap - EXIT SIGHUP SIGTERM SIGINT

# handle abnormal exit
if [ $EXIT_CODE -ne 0 ]; then
    send_telegram "[ERROR] Program exited with error (exit code: $EXIT_CODE)."
else
    send_telegram "[INFO] Program completed successfully."
fi
