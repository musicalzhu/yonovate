# NVS Manager Component
# 统一的 NVS 封装层，简化键值存储操作

idf_component_register(
    SRCS "src/nvs_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash
    PRIV_REQUIRES log
)
