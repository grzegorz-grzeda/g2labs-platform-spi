/*
 * MIT License
 *
 * Copyright (c) 2023 G2Labs Grzegorz Grzęda
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "g2labs-platform-spi.h"
#include <stdlib.h>
#include "driver/spi_master.h"
#define G2LABS_LOG_MODULE_NAME "platform-spi"
#define G2LABS_LOG_MODULE_LEVEL (G2LABS_LOG_MODULE_LEVEL_DEBUG)
#include <g2labs-log.h>

typedef struct platform_spi {
    uint8_t host_id;
    spi_bus_config_t buscfg;
} platform_spi_t;

typedef struct platform_spi_device {
    platform_spi_t* spi;
    spi_device_handle_t device;
    spi_device_interface_config_t devcfg;
} platform_spi_device_t;

uint8_t platform_spi_get_host_count(void) {
    return 2;
}

platform_spi_t* platform_spi_initialize(platform_spi_configuration_t configuration) {
    platform_spi_t* spi = calloc(1, sizeof(*spi));
    if (!spi) {
        return NULL;
    }
    spi_bus_config_t buscfg = {
        .miso_io_num = configuration.miso_pin,
        .mosi_io_num = configuration.mosi_pin,
        .sclk_io_num = configuration.sclk_pin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0,
    };
    spi->host_id = configuration.host_id;
    spi->buscfg = buscfg;
    esp_err_t ret = spi_bus_initialize(configuration.host_id, &spi->buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    D("Initialized SPI for host %u", configuration.host_id);
    return spi;
}

platform_spi_device_t* platform_spi_add_device(platform_spi_t* spi, platform_spi_device_configuration_t configuration) {
    if (!spi) {
        return NULL;
    }
    platform_spi_device_t* device = calloc(1, sizeof(*device));
    if (!device) {
        return NULL;
    }
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = configuration.clock_frequency_hz,
        .mode = configuration.mode,
        .spics_io_num = configuration.ncss_pin,
        .queue_size = 1,
    };
    device->spi = spi;
    device->devcfg = devcfg;
    esp_err_t ret = spi_bus_add_device(device->spi->host_id, &device->devcfg, &device->device);
    D("[NCSS:%u]Added device mode %u for host %u", configuration.ncss_pin, configuration.mode, spi->host_id);
    ESP_ERROR_CHECK(ret);
    return device;
}

size_t platform_spi_transmit(platform_spi_device_t* device, const char* tx_buffer, char* rx_buffer, size_t size) {
    if (!device || !tx_buffer || !size) {
        return 0;
    }
    size_t rxsize = rx_buffer ? size : 0;
    spi_transaction_t transaction = {
        .tx_buffer = tx_buffer,
        .rx_buffer = rx_buffer,
        .length = size * 8,
        .rxlength = rxsize * 8,
    };
    esp_err_t ret = spi_device_polling_transmit(device->device, &transaction);
    D("[NCSS:%u] Sent %lu, received %lu bytes", device->devcfg.spics_io_num, size, rxsize);
    ESP_ERROR_CHECK(ret);
    return size;
}
