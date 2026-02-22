idf_component_register(SRCS "main.c"
                            "himcul_framework.c"
                            "himcul_product.c"
                            "himcul_profile.c"
                            "himcul_core.c"
                            "himcul_ota.c"
                            "servo_180.c"
                        PRIV_REQUIRES spi_flash driver
                        INCLUDE_DIRS "")
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-format)
