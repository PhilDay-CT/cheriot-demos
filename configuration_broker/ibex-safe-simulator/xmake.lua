-- Copyright Configured Things Ltd and CHERIoT Contributors.
-- SPDX-License-Identifier: MIT


set_project("CHERIoT Compartmentalised Config")
sdkdir = "../../cheriot-rtos/sdk"
includes(sdkdir)
set_toolchains("cheriot-clang")

-- Support libraries
includes(path.join(sdkdir, "lib/freestanding"),
         path.join(sdkdir, "lib/string"))

option("board")
    set_default("ibex-safe-simulator")


includes("../common/json_parser")
includes("../common/config_broker") 
includes("../common/config_consumer")


-- Library for Mocked logger service
library("logger")
    set_default(false)
    add_files("lib/logger/logger.cc")

-- Library for Mocked RGB LED config service
library("rgb_led")
    set_default(false)
    add_files("lib/rgb_led/rgb_led.cc")       

-- Library for Mocked User LED config service
library("user_led")
    set_default(false)
    add_files("lib/user_led/user_led.cc")       

-- Mocked MQTT Client
compartment("mqtt")
    add_files("mqtt_stub/mqtt.cc")

-- Configuration Provider
includes("../common/provider")

-- Parser init compartment
compartment("parser_init")
    add_files("parser_init/parser_init.cc")

-- Configuration JSON parser sandboxes
includes("../common/parsers/rgb_led")
includes("../common/parsers/user_led")
includes("../common/parsers/logger")

-- Consumers
includes("../common/consumers")

-- Firmware image for the example.
firmware("config-broker-ibex-sim")
    add_deps("freestanding", "debug", "string")
    -- libraries
    add_deps("json_parser")
    add_deps("logger")
    add_deps("rgb_led")
    add_deps("user_led")
    add_deps("config_consumer")
    
    -- compartments
    add_deps("mqtt")
    add_deps("provider")
    add_deps("config_broker")
    add_deps("parser_init")
    add_deps("parser_logger")
    add_deps("parser_rgb_led")
    add_deps("parser_user_led")
    add_deps("consumer1")
    add_deps("consumer2")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                -- Thread to receive config values.
                -- Starts in the parser_init compartment
                -- and then loops in the mqtt compartment.
                compartment = "parser_init",
                priority = 1,
                entry_point = "parser_init",
                stack_size = 0x700,
                trusted_stack_frames = 8
            },
            {
                -- Thread to consume config values.
                -- Starts and loops in consumer1
                compartment = "consumer1",
                priority = 2,
                entry_point = "init",
                stack_size = 0x500,
                trusted_stack_frames = 4
            },
            {
                -- Thread to consume config values.
                -- Starts and loops in consumer2
                compartment = "consumer2",
                priority = 2,
                entry_point = "init",
                stack_size = 0x500,
                trusted_stack_frames = 4
            },
        }, {expand = false})
    end)


