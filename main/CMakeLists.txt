idf_component_register(
    SRCS "main.c"
         "quiz_model.c"
         "quiz_bank.c"
         "app_physics.c"
         "demo_navigation.c"
         "ui_pixel.c"
         "ui_pixel_math.c"
         "demo_display.c"
         "demo_button.c"
         "demo_audio.c"
         "demo_battery.c"
         "demo_radio.c"
         "demo_wifi.c"
         "demo_ble.c"
         "demo_low_power.c"
    INCLUDE_DIRS "."
    REQUIRES bsp bt esp_event esp_hw_support esp_netif esp_timer esp_wifi nvs_flash
)

# 中文界面字体(由 lv_font_conv 从 Noto Sans SC 生成的子集)。
target_sources(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/physics_font_16.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/physics_font_22.c"
)
