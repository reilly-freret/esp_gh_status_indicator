# a freaking status indicator

built for [lilygo t-display](https://lilygo.cc/products/t-display). uses an [ST7789](https://www.buydisplay.com/download/ic/ST7789.pdf) driver. uses the [esp_lvgl_port](https://components.espressif.com/components/espressif/esp_lvgl_port/versions/2.6.2/readme) library, which is fucked.

## prereqs

- esp idf

## setup

- `idf.py menuconfig` -- see `Kconfig.projbuild`
  - GITHUB_USERNAME
  - GITHUB_REPO
  - GITHUB_AUTH_TOKEN
  - STATUS_CHECK_INTERVAL
  - WIFI_SSID
  - WIFI_PASSWORD
- `idf.py build flash monitor`
