# Proyecto final


## How to use

### Configuring the project

```bash
alias get_idf='. $HOME/esp/esp-idf/export.sh'
get_idf
```

### Compile

```bash
idf.py build
```

### Deploy to ESP32 card

```bash 
idf.py -b 115200 -p /dev/ttyS11 flash monitor
```


