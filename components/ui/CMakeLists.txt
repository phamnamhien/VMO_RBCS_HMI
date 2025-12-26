# ESP-IDF Component CMakeLists.txt
# Auto-generated from SquareLine Studio export

idf_component_register(
    SRCS 
        "components/ui_comp_buttoncontainer.c"
        "components/ui_comp.c"
        "screens/ui_scrSplash.c"
        "screens/ui_scrMain.c"
        "screens/ui_scrProcess.c"
        "screens/ui_scrManualControl12.c"
        "screens/ui_scrManualControl22.c"
        "screens/ui_scrSetting.c"
        "screens/ui_scrDetail.c"
        "ui.c"
        "components/ui_comp_hook.c"
        "ui_helpers.c"
        "images/ui_img_bg1_png.c"
        "images/ui_img_vmo_logo_png.c"
        "images/ui_img_bg2_png.c"
        "images/ui_img_vmo_logo2_png.c"
        "images/ui_img_left_side_2_png.c"
        "images/ui_img_battery_png.c"
        "images/ui_img_setting_1_png.c"
        "images/ui_img_help_1_png.c"
        "images/ui_img_manual_png.c"
        "images/ui_img_group_14_png.c"
        "images/ui_img_scrnotconnected_warningicon_png.c"
        "images/ui_img_group_13_png.c"
        "images/ui_img_143516147.c"
        "images/ui_img_1_blue_3_5_png.c"
        "images/ui_img_1_blue_3_8_png.c"
        "images/ui_img_z7358953714029_6a7a8c912df9884a2fa156d0bc4a5d11_1_png.c"
        "fonts/ui_font_Bold.c"
        "fonts/ui_font_H1.c"
        "fonts/ui_font_H11.c"
        "fonts/ui_font_H2.c"
        "fonts/ui_font_Light.c"
        "fonts/ui_font_Number.c"
    INCLUDE_DIRS 
        "."
        "screens"
        "components"
        "images"
    REQUIRES 
        lvgl
)

# Add compile options if needed
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-unused-variable
    -Wno-unused-function
)
