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


-- Firmware image for the example.
firmware("mqtt-client")
    add_deps("freestanding", "debug", "string")
    -- libraries
    add_deps("mqtt")
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

