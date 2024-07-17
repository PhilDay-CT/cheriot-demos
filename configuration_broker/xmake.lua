-- Copyright Configred Things and CHERIoT Contributors.
-- SPDX-License-Identifier: MIT


set_project("CHERIoT Compartmentalised Config")
sdkdir = "../cheriot-rtos/sdk"
includes(sdkdir)
set_toolchains("cheriot-clang")

-- Support libraries
includes(path.join(sdkdir, "lib/freestanding"),
         path.join(sdkdir, "lib/string"))

option("board")
    set_default("ibex-safe-simulator")

-- Library for Mocked logger service
library("logger")
    set_default(false)
    add_files("logger/logger.cc")     

-- Mocked MQTT Client
compartment("mqtt")
    add_files("mqtt_stub/mqtt.cc")

-- Configuration Broker
debugOption("config_broker")
compartment("config_broker")
    add_rules("cheriot.component-debug")
    add_files("config_broker/config_broker.cc")

-- Validator
compartment("validator")
    add_files("validator/validator.cc")

-- Configuration Publisher
compartment("publisher")
    add_files("publisher/publisher.cc")

-- Configuration JSON parser sandbox
compartment("parser")
    add_files("parser/parser.cc")
    add_files("parser/core_json.cc")

-- Consumers
compartment("consumer1")
    add_files("consumers/consumer1.cc")

-- Firmware image for the example.
firmware("compartment_config")
    add_deps("freestanding", "debug", "string")
    add_deps("logger")
    add_deps("mqtt")
    add_deps("publisher")
    add_deps("parser")
    add_deps("config_broker")
    add_deps("validator")
    add_deps("consumer1")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "mqtt",
                priority = 2,
                entry_point = "init",
                stack_size = 0x600,
                trusted_stack_frames = 8
            },
            {
                compartment = "consumer1",
                priority = 1,
                entry_point = "init",
                stack_size = 0x500,
                trusted_stack_frames = 4
            },
            {
                compartment = "publisher",
                priority = 2,
                entry_point = "bad_dog",
                stack_size = 0x500,
                trusted_stack_frames = 4
            },
        }, {expand = false})
    end)
