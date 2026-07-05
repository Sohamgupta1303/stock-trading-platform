# Stock Trading Platform

A client/server stock trading simulator written in C++20. A single server
process simulates a small stock market — random-walk prices, a
price/time-priority order book per stock, and simulated traders — while
any number of terminal `client` processes connect to it over TCP to check
prices, place buy/sell orders, and get risk-based stock recommendations.

## Tech stack

- C++20
- CMake (>= 3.10)
- POSIX sockets (`sys/socket.h`, `netinet/in.h`, `arpa/inet.h`) for TCP networking
- `pthread` (via `std::thread`) — one thread per stock price feed, one
  thread per connected client, and one thread that periodically logs
  market state
- `std::mutex` for thread safety
- Google Test (fetched automatically via CMake `FetchContent`) for unit tests

## Project structure

```
project/
  README.md
  server/
    CMakeLists.txt
    include/          # Stock, Order, OrderBook, Trader, Market, shared utils
    src/               # matching, matching engine, networking, main()
    tests/             # GoogleTest unit tests, one file per class
  client/
    CMakeLists.txt
    include/utils.h    # socket send/recv helpers
    src/main.cpp       # terminal client
```

## Building and running the server

```sh
cd server
cmake -B build
cmake --build build -j
./build/server
```

The server listens on TCP port `8000`, seeds 10 stocks, and prints a
market-state snapshot to the console roughly every 10 seconds. Press
`Ctrl-C` to shut it down cleanly.

### Running the server's unit tests

Building the server also builds a `server_tests` binary (GoogleTest, fetched
automatically by CMake — no local install needed):

```sh
cd server/build
ctest --output-on-failure
```

Pass `-DBUILD_SERVER_TESTS=OFF` to `cmake -B build` if you want to skip
fetching/building GoogleTest entirely.

## Building and running the client

In a separate terminal (with the server already running):

```sh
cd client
cmake -B build
cmake --build build -j
./build/client
```

The client connects to `127.0.0.1:8000`, then reads whitespace-delimited
tokens from stdin in a loop — type a command and press Enter (or pipe
several at once) and it prints the server's response for each one.

## Wire protocol

Plain text over TCP, one command per message:

```
GET_STOCKS                         -> returns your balance + holdings
BUY:SYMBOL:QUANTITY                -> e.g. BUY:AAPL:10
SELL:SYMBOL:QUANTITY                -> e.g. SELL:AAPL:10
RECOMMEND:RISK_TOLERANCE:COUNT     -> e.g. RECOMMEND:1:4 (1=conservative, 2=moderate, 3=aggressive)
```

Malformed requests return a clear `Error: ...` string instead of crashing
the server or silently ignoring the request. There is no client-chosen
limit price — orders execute at (or against) the stock's current price.

## Design notes

- **No stale liquidity.** Every stock has a synthetic market maker with a
  standing buy and sell quote, priced a small fixed spread around the
  stock's *current* price. Every time a stock's price updates, its market
  maker quotes are cancelled and re-added at the new price — so there is
  always a matchable counterparty near the live price, and a client order
  is never silently accepted without actually being able to trade against
  it.
- **Honest confirmations.** `BUY`/`SELL` responses report exactly what
  happened — `executed successfully`, `partially filled`, or `added but
  not yet filled` — based on the trader's actual balance/holdings change,
  never an unconditional "success."
- **Concurrency.** One thread per stock updates that stock's price on an
  independent random 1-15s interval; one thread prints the whole market
  state every ~10s; one thread per connected client handles that client's
  requests until it disconnects. The `Market` mutex guards all shared
  state (traders, order books); each `Stock` has its own separate mutex
  guarding just its price/history, so price ticks don't contend with
  order matching for other stocks.
