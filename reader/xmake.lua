-- Copyright lowRISC Contributors.
-- SPDX-License-Identifier: Apache-2.0

set_project("Sonata Software")
sdkdir = "../cheriot-rtos/sdk"
includes(sdkdir)
set_toolchains("cheriot-clang")

includes(path.join(sdkdir, "lib"))

option("board")
    set_default("sonata")

-- sonata libraries
includes("sonata_lcd");

compartment("reader")
    add_deps("debug")
    add_files("reader.cc")


-- A simple demo using only devices on the Sonata board
firmware("sonata_reader")
    add_deps("freestanding", "reader")
    add_deps("lcd")
    on_load(function(target)
        target:values_set("board", "$(board)")
        target:values_set("threads", {
            {
                compartment = "reader",
                priority = 2,
                entry_point = "init",
                stack_size = 0x400,
                trusted_stack_frames = 3
            },
        }, {expand = false})
    end)

