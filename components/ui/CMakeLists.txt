# ESP-IDF Component CMakeLists.txt
# Auto-generated from SquareLine Studio export

idf_component_register(
    SRCS 
        "screens/ui_scrSplash.c"
        "screens/ui_scrMain.c"
        "screens/ui_scrSetting.c"
        "ui.c"
        "components/ui_comp_hook.c"
        "ui_helpers.c"
        "ui_events.c"
        "images/ui_img_imgsplashbackground_png.c"
        "images/ui_img_robot_4_png.c"
        "images/ui_img_imgmainbackground_png.c"
        "images/ui_img_scrsettingicon_png.c"
        "images/ui_img_imgmainslotserialdetail_png.c"
        "images/ui_img_scrmain_batteryempty_png.c"
        "images/ui_img_scrmain_batterywarning_png.c"
        "images/ui_img_imgmainslotdetailbackground_png.c"
        "images/ui_img_imgmainmanualswapbat_png.c"
        "images/ui_img_scrmain_robotlowbattery_png.c"
        "images/ui_img_scrmain_robotfullbattery_png.c"
        "images/ui_img_scrmain_robotcharging_png.c"
        "images/ui_img_imgmainrbtoslot1_png.c"
        "images/ui_img_imgmainrbtoslot2_png.c"
        "images/ui_img_imgmainrbtoslot3_png.c"
        "images/ui_img_imgmainrbtoslot4_png.c"
        "images/ui_img_imgmainrbtoslot5_png.c"
        "images/ui_img_scrmain_batteryicon_png.c"
        "images/ui_img_scrnotconnecticon_png.c"
        "images/ui_img_imgsettingbackground_png.c"
        "images/ui_img_imgsettingbackicon_png.c"
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
