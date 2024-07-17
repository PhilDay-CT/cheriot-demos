-- Copyright Microsoft and CHERIoT Contributors.
-- SPDX-License-Identifier: MIT

-- Contributed by Configured Things Ltd

set_project("CHERIoT Compartmentalised Config")
sdkdir = "../cheriot-rtos/sdk"
includes(sdkdir)
set_toolchains("cheriot-clang")

-- Support libraries
-- Support libraries
includes(path.join(sdkdir, "lib/freestanding"),
         path.join(sdkdir, "lib/string"))

option("board")
    set_default("ibex-safe-simulator")

-- Library for Mocked logger
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
    add_files("config_broker.cc")

-- Configuration Publisher
debugOption("publisher")
compartment("publisher")
    add_rules("cheriot.component-debug")
    add_files("publisher/publisher.cc")

-- Configuration JSON parser sandbox
compartment("parser")
    add_rules("cheriot.component-debug")
    add_files("parser/parser.cc")
    add_files("parser/core_json.cc")

-- Compartments to be configured
--compartment("subscriber1")
--    add_files("subscriber1.cc")
--compartment("subscriber2")
--    add_files("subscriber2.cc")
compartment("subscriber3")
    add_files("subscriber3.cc")

compartment("validator")
    add_files("validator/validator.cc")


-- Firmware image for the example.
firmware("compartment_config")
    -- Both compartments require memcpy
    add_deps("freestanding", "debug", "string")
    --add_deps("config_data")
    add_deps("logger")
    add_deps("mqtt")
    add_deps("publisher")
    add_deps("parser")
    add_deps("config_broker")
    --add_deps("subscriber1")
    --add_deps("subscriber2")
    add_deps("subscriber3")
    add_deps("validator")
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
        --    {
        --        compartment = "subscriber1",
        --        priority = 3,
        --        entry_point = "init",
        --        stack_size = 0x500,
        --        trusted_stack_frames = 4
        --    },
        --    {
        --        compartment = "subscriber2",
        --        priority = 1,
        --        entry_point = "init",
        --        stack_size = 0x500,
        --        trusted_stack_frames = 4
        --    },
            {
                compartment = "subscriber3",
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
