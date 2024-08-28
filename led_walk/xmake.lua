-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

set_project("Sonata Software")
sdkdir = "../cheriot-rtos/sdk"
includes(sdkdir)
set_toolchains("cheriot-clang")

includes(path.join(sdkdir, "lib"))

option("board")
    set_default("sonata")

compartment("led_walk_raw")
    add_deps("debug")
    add_files("led_walk_raw.cc")


-- A simple demo using only devices on the Sonata board
firmware("sonata_simple_demo")
    add_deps("freestanding", "led_walk_raw")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "led_walk_raw",
                priority = 2,
                entry_point = "start_walking",
                stack_size = 0x200,
                trusted_stack_frames = 1
            },
        }, {expand = false})
    end)

