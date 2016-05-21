print("goodbye, cruel world")

a = virtual_keys[64]
print("virtual key (64)'s current state is: ", a)

b = scancodes[64]
print("scancode (64)'s current state is: ", b)

-- virtual_keys[64] = true -- ERROR
-- scancodes[64] = true -- ERROR

print("virtual_keys size: ", #virtual_keys, "\n", virtual_keys)
print("scancodes size: ", #scancodes, "\n", scancodes)

print(vk.kana)

-- Metadata about Virtual Key Codes
for i = 0, #keyboard.virtual_key_descriptions do
    print("VK: ", i, " - ", keyboard.virtual_key_descriptions[i]);
end
print(#keyboard.virtual_key_descriptions + 1, " key descriptions listed")

keyboard.listen_for_virtual_key_make(vk.a, function(vk_code, scancode, e0, e1, extra_info)
    print("post-process 'a' key make")
end)

keyboard.listen_for_scancode_make(0x30, function(vk_code, scancode, e0, e1, extra_info)
    print("post-process scancode: ", scancode, " make")
end)

keyboard.listen_for_scancode_break(0x2e, function(vk_code, scancode, e0, e1, extra_info)
    print("post-process scancode: ", scancode, " break")
end)

keyboard.listen_for_virtual_key_break(vk.d, function(vk_code, scancode, e0, e1, extra_info)
    print("post-process 'd' key break")
end)

keyboard.listen_for_virtual_key_make(vk.escape, function()
    print("clearing listeners")
    keyboard.stop_listening_for_virtual_key_make(vk.a)
    keyboard.stop_listening_for_scancode_make(0x30)
    keyboard.stop_listening_for_scancode_break(0x2e)
    keyboard.stop_listening_for_virtual_key_break(vk.d)
end)

keyboard.listen_for_virtual_key_make(vk.f4, function()
    keyboard.send_virtual_key_make(vk.shift)
    keyboard.send_scancode_make(0x21)
    keyboard.send_scancode_break(0x21)
    keyboard.send_virtual_key_break(vk.shift)
end)

keyboard.listen_for_virtual_key_make(vk.f6, function()
    keyboard.send_scancode_make(0xe0, 0x1d)
    --keyboard.send_virtual_key_make(0xe0, vk.control)
    --keyboard.send_virtual_key_make(vk.shift)
    keyboard.send_virtual_key_make(0xe0, vk.v)
    keyboard.send_virtual_key_break(0xe0, vk.v)
    --keyboard.send_virtual_key_break(vk.shift)
    --keyboard.send_virtual_key_break(0xe0, vk.control)
    keyboard.send_scancode_break(0xe0, 0x1d)
end)

keyboard.listen_for_virtual_key_make(vk.f7, function()
    keyboard.send_text("BcDeFg", 12, "hIJklMNoP", "q") -- sends unicode text to the application with focus
end)

keyboard.listen_for_virtual_key_make(vk.f9, function()
    keyboard.hook()
end)

keyboard.listen_for_virtual_key_make(vk.f10, function()
    keyboard.unhook()
end)

keyboard.listen_for_virtual_key_make(vk.f11, function()
    keyboard.intercept_virtual_key_make(vk.z, function()
        keyboard.send_text("Zed")
    end)
    keyboard.intercept_virtual_key_break(vk.z, function() end)
end)

keyboard.listen_for_virtual_key_make(vk.f12, function()
    keyboard.stop_intercepting_virtual_key_make(vk.z);
    keyboard.stop_intercepting_virtual_key_break(vk.z);
end)

--dumpstack()

--vk.code["a"];

--[[
-- APPCOMMAND_MEDIA_PLAY_PAUSE, keyboard/mouse/unidentifed hardware
--application_command_descriptions
--keyboard.listen_for_application_command(ac.media_play_pause, function(app_command, source_device)
--    print("application command")
--end)

--keyboard.intercept_scancode_make
keyboard.intercept_virtual_key_make(vk.a, function(vk_code, scancode, e0, e1, extra_info)
    print("pre-process 'a' key make")
end)

--keyboard.intercept_scancode_break
keyboard.intercept_virtual_key_break(vk.a, function(vk_code, scancode, e0, e1, extra_info)
    print("pre-process 'a' key break")
end)

keyboard.send_virtual_key_make(vk.lshift) -- sends only the virutal key make signal; takes 1 or 2 arguments (see below)
keyboard.send_keys(vk.tab, vk.a, ...) -- sends make/break pair for each vk; 0xe0 (gets no special treatment)

keyboard.send_virutal_key_break(vk.lshift) -- sends only the virtual key break signal; takes 1 or 2 arguments (see below)

keyboard.send_virtual_key_make(0xe0, vk.control) -- ex. using two args, first argument specifies the e0 flag
keyboard.send_virtual_key_break(0xe0, vk.control)

keyboard.send_scancode_make(0xe0, 0x1d) -- just like the virtual key make/break functions, these functions send only the make or break signal
keyboard.send_scancode_break(0xe0, 0x1d) -- ex. using two args, first argument specifies the e0 or e1 flag
--]]
