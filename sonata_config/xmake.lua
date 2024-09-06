-- Copyright Configured Things Ltd and CHERIoT Contributors.
-- SPDX-License-Identifier: MIT


set_project("CHERIoT Compartmentalised Config")
sdkdir = path.absolute("../cheriot-rtos/sdk")

includes(sdkdir)
set_toolchains("cheriot-clang")

-- Support libraries
--includes(path.join(sdkdir, "lib/freestanding"),
--         path.join(sdkdir, "lib/string"))

includes(path.join(sdkdir, "lib"))
includes("../network-stack/lib")

option("board")
    set_default("sonata")

-- sonata libraries
includes("sonata_lcd");

-- library for JSON parser   
library("json_parser")
    set_default(false)
    add_files("../configuration_broker/json_parser/json_parser.cc")
    add_files("../configuration_broker/json_parser/core_json.cc")

-- Library for RGB LED config service
library("rgb_led")
    set_default(false)
    add_deps("cxxrt")
    add_files("rgb_led/rgb_led.cc")       

-- Library for User LED config service
library("user_led")
    set_default(false)
    add_deps("cxxrt")
    add_files("user_led/user_led.cc")       

-- LCD Console
compartment("console")
    add_deps("cxxrt")
    add_files("console/console.cc")

-- LCD Console Test
--!compartment("console_test")
--!    add_files("console_test/console_test.cc")

-- Mocked MQTT Client
--!compartment("mqtt_stub")
--!    add_files("mqtt_stub/mqtt_stub.cc")

-- MQTT via uart
--!compartment("mqtt_uart")
--!    add_files("mqtt_uart/mqtt_uart.cc")

-- MQTT via Broker
compartment("mqtt")
    add_includedirs("../network-stack/include")
    add_deps("freestanding", "TCPIP", "NetAPI", "TLS", "Firewall", "SNTP", "MQTT", "time_helpers", "debug", "stdio")
    add_files("mqtt/mqtt.cc")
    on_load(function(target)
        target:add('options', "IPv6")
        local IPv6 = get_config("IPv6")
        target:add("defines", "CHERIOT_RTOS_OPTION_IPv6=" .. tostring(IPv6))
    end)

-- Configuration Broker
debugOption("config_broker")
compartment("config_broker")
    add_rules("cheriot.component-debug")
    add_files("config_broker/config_broker.cc")

-- Configuration Provider
compartment("provider")
    add_files("provider/provider.cc")

-- Configuration JSON parser sandboxes
compartment("parser_rgb_led")
    add_files("parser/parse_rgb_led.cc")

compartment("parser_user_led")
    add_files("parser/parse_user_led.cc")

compartment("parser_console")
    add_files("parser/parse_console.cc")

-- Consumers
compartment("consumer1")
    add_files("consumers/consumer1.cc")
compartment("consumer2")
    add_files("consumers/consumer2.cc")

-- Firmware image for the example.
firmware("config-broker-sonata")
    add_deps("freestanding", "debug", "string")

    -- LCD Console Driver 
    add_deps("lcd")
    add_deps("console")

    --
    -- MQTT Receivers - select 1
    --     mqtt_stub  loops though test messages
    --     mqtt_uart  takes messages from the uart
    --     mqtt       uses a real broker
    add_deps("mqtt")
--!    add_deps("mqtt_stub")
--!    add_deps("mqtt_uart")

    -- Provider (the link between MQTT and the Connfig Broker) 
    add_deps("provider")

    -- Configuration Broker
    add_deps("config_broker")

    -- Parsers for the different Config types
    add_deps("json_parser")
    add_deps("parser_rgb_led")
    add_deps("parser_user_led")
    add_deps("parser_console")

    -- Consumer 1
    --   RGB LED
    --   Console
    add_deps("rgb_led")
    add_deps("consumer1")

    -- Consumer 2
    --   User LEDs
    add_deps("user_led")
    add_deps("consumer2")

    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                -- Thread to set config values.
                -- Starts and loops in the mqtt
                -- compartment.
                compartment = "mqtt",
                priority = 1,
                entry_point = "init",
                stack_size = 8160,
                trusted_stack_frames = 8
            },
            {
                -- Thread to consume config values.
                -- Starts and loops in consumer1
                compartment = "consumer1",
                priority = 2,
                entry_point = "init",
                stack_size = 0x500,
               trusted_stack_frames = 8
            },
            {
                -- Thread to consume config values.
                -- Starts and loops in consumer2
                compartment = "consumer2",
                priority = 2,
                entry_point = "init",
                stack_size = 0x500,
                trusted_stack_frames = 8
            },
            {
                -- TCP/IP stack thread.
                compartment = "TCPIP",
                priority = 1,
                entry_point = "ip_thread_entry",
                stack_size = 0x1000,
                trusted_stack_frames = 5
            },
            {
                -- Firewall thread, handles incoming packets as they arrive.
                compartment = "Firewall",
                -- Higher priority, this will be back-pressured by the message
                -- queue if the network stack can't keep up, but we want
                -- packets to arrive immediately.
                priority = 2,
                entry_point = "ethernet_run_driver",
                stack_size = 0x1000,
                trusted_stack_frames = 5
            }
        }, {expand = false})
    end)




