idf_component_register(
    SRCS "eye_animation.c"
    INCLUDE_DIRS "." "eyes_data"
    PRIV_REQUIRES driver esp_timer esp_lcd)