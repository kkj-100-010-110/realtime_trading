# realtime_trading

**This program displays real-time tickers and orderbooks for three cryptocurrencies and allows trading.**

- **Language** : C
- **Libraries**: jansson, curl, libwebsockets, ncurses

## Requirements
	1. '.env' file in the root directory which has 
        UPBIT_API_KEY=your_api_key
        UPBIT_SECRET_KEY=your_secret_key
        TELEGRAM_BOT_TOKEN=your_bot_token
        TELEGRAM_CHAT_ID=your_chat_id
	
	2. Select three cryptocurrencies and update their symbols in:
        - config/market.json
        - config/symbol.txt,
        - include/account_handler.h
        - include/symbol_handler.h

## How to run
    compile : make
    execute : ./program or use make run_sh (sends notifications on IP change/program exit).

## Youtube 
→ https://www.youtube.com/watch?v=7no4F9D1Vy8
