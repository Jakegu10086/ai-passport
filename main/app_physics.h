// main/app_physics.h —— “物理题”应用入口与按键分发。
#pragma once

#include "bsp_button.h"

// 创建并载入应用首页(需在 LVGL 初始化之后调用,函数内部自行加锁)。
void app_physics_start(void);

// 由按键事件任务调用;内部只做轻量 UI 更新,并自行缩短 LVGL 锁范围。
void app_physics_handle_key(bsp_btn_t btn, bsp_btn_ev_t ev);
