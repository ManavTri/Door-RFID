idf_component_register(SRCS "main.cpp"
                       PRIV_REQUIRES spi_flash
                       REQUIRES nvs_flash protocol_examples_common esp_netif
                       INCLUDE_DIRS ".")
