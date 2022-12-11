From Zephyr's $ZEPHYR_BASE dir:

```
west build -t guiconfig -b heltec_wifi_lora32_v2 $HOME/[repo dir]/sensor-gateway/zephyr2 -DSHIELD=ssd1306_128x64 -DOVERLAY_CONFIG=wifi-data.conf
```

```
west build -b heltec_wifi_lora32_v2 $HOME/[repo dir]/sensor-gateway/zephyr2 -- -DSHIELD=ssd1306_128x64 -DOVERLAY_CONFIG=wifi-data.conf
```

```
west flash && west espressif monitor
```
