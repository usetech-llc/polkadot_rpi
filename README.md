# Polkadot GUI for Raspberry Pi

## Dependencies
```
sudo apt-get install libgtk-3-dev libboost-all-dev libcurl4-openssl-dev

git clone https://github.com/zaphoyd/websocketpp wpp
sudo rm -rf /usr/local/include/websocketpp
sudo mv wpp/websocketpp /usr/local/include/websocketpp
```

Also, please clone and build the sr25519-crust library as instructed in its repository:
https://github.com/Warchant/sr25519-crust.git

## Build and run
```
cmake .
make
bin/polkaui
```
