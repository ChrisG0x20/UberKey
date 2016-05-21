# UberKey
Lua scriptable user mode keyboard hook and interception application.

## About
This program is a prototype implementation of keyboard customization utility. It has the ability to listen for key presses, and releases, and execute Lua scripts in response. It can listen to for key events passively or actively. Keys generate “make” and “break” events, when a key is press and released, respectively.

When in passive mode, the program merely listens for key events as they are input into whatever application has the current keyboard focus. When a key event of interest occurs it may trigger Lua functions to execute in response. Passive mode is probably most useful for adding side-effects to certain key sequences or, mapping some unused key to a useful function within the application, or system in general.

When in active mode, the program installs a low-level keyboard hook procedure into the Windows keyboard event queue. When a key event of interest occurs, it is first, filtered out of the Windows keyboard event queue then, the low-level hook procedure triggers the appropriate Lua function to execute. The Lua script may inject a replacement key sequence into the Keyboard event queue, take some other custom action, or do nothing at all.

While this application is functional. I would not consider it complete. There’s a lot of missing functionality and usability. While this makes an excellent proof-of-concept, do not consider it even Alpha quality software.

## Lua API

#### Standard Lua Libraries
All of the standard Lua libraries are loaded by default. No attempt to sandbox the Lua environment has been made.

#### General Functions
`print(...)` The default Lua _print()_ function has been overridden by this program for to technical reasons. Although, it is likely not 100% identical to the builtin version, it seems to work OK.

`dumpstack()` Dumps the current Lua stack to the console window in a somewhat formatted display.

#### Scancode and Virtual Key Tables
Two readonly tables have been added to the global namespace: `scancodes` and `virtual_keys`. Each table is a view into the current (as seen by Windows) state of the current keyboard. Here’s an example of how to view each of the tables:

	print("Size of virtual_keys table: ", #virtual_keys, "\n", virtual_keys)
	print(“Size of scancodes table: ", #scancodes, "\n", scancodes)
	x = virtual_keys[64]
	print("Virtual key (64)'s current state is: ", x)

#### Virtual Key Symbolic Names
Since dealing with raw scancode and virtual key values makes wimps cry, Microsoft created a bunch of symbolic names for most virtual key values. Those symbolic names have been mostly reproduced within the Lua environment. So, to get the state of the **F9** key, instead of scripting `x = virtual_keys[120]` you may write `x = virtual_keys[vk.f9]`.

You may output the values of the global `vk` table to see what else is available. In general letters appear as you’d expect (i.e. `vk.a, vk.b, vk.c, ...`) and numbers start with an underscore character (i.e. `vk._1, vk._2, vk._3, ...`). A few keys even have more than one name, for example: **kana**, **hangul**, and **hangeul** may all be used to refer to the same virtual key value: **21**.

#### Passive Listening
	function do_action_a()
		print(“I see an A”)
	end
	keyboard.listen_for_virtual_key_make(vk.a, do_action_a)
	 
	-- OR --
	 
	keyboard.listen_for_virtual_key_make(vk.h, function()
		keyboard.send_text("ello, world")
	end)

The passive **listen** functions all take two parameters: 
1. The virtual key value or scancode to listen for.
2. The callback function to execute when the key event occurs.
---- 
`callback(virtual_key, scancode, e0, e1, extra_information)`

Each callback function is passed the following parameters, which you are free to use or ignore:

1. The virtual key value associated with the triggered key event.
2. The scancode associated with the triggered key event.
3. The state of enhanced key flag zero.
4. The state of enhanced key flag one.
5. An optional vendor specific informational value. _NOTE:_ This would be information added by a 3rd-party keyboard driver.
---- 
`keyboard.listen_for_virtual_key_make(virtual_key, callback)`
> Begin listening for a **virtual key make** event.
`keyboard.listen_for_virtual_key_break(virtual_key, callback)`
> Begin listening for a **virtual key break** event.
`keyboard.listen_for_scancode_make(scancode, callback)`
> Begin listening for a **scancode make** event.
`keyboard.listen_for_scancode_break(scancode, callback)`
> Begin listening for a **scancode break** event.

The **stop listening** functions all take a single parameter: the virtual key value or the scancode to stop listening for.

`keyboard.stop_listening_for_virtual_key_make(virtual_key)`
> Stop listening for a **virtual key make** event.
`keyboard.stop_listening_for_virtual_key_break(virtual_key)`
> Stop listening for a **virtual key break** event.
`keyboard.stop_listening_for_scancode_make(scancode)`
> Stop listening for a **scancode make** event.
`keyboard.stop_listening_for_scancode_break(scancode)`
> Stop listening for a **scancode break** event.

#### Active Listening
Intercepting key events works almost identically to the passive listening functions. The primary difference is that you must remember to call the `keyboard.hook` function to install the low-level keyboard hook procedure before any of the interception functions will work.

`keyboard.hook()`
> Install the global low-level keyboard hook procedure into the current Windows environment.
`keyboard.unhook()`
> Remove the global low-level keyboard hook procedure.

`keyboard.intercept_virtual_key_make(virtual_key, callback)`
`keyboard.intercept_virtual_key_break(virtual_key, callback)`
`keyboard.intercept_scancode_make(scancode, callback)`
`keyboard.intercept_scancode_break(scancode, callback)`
`keyboard.stop_intercepting_virtual_key_make(virtual_key)`
`keyboard.stop_intercepting_virtual_key_break(virtual_key)`
`keyboard.stop_intercepting_scancode_make(scancode)`
`keyboard.stop_intercepting_scancode_break(scancode)`

#### Generating Artificial Key Events
There are several functions for generating different low-level keyboard events and sending them to the application with keyboard focus. Each of these low-level functions may be called with one, or _optionally_ two, parameters:
> 1. The **optional** enhanced key flag: **0xe0**
> 2.  The virtual key value or scancode.

`keyboard.send_virtual_key_make([0xe0], virtual_key)`
> Sends a **virtual key make** event to the application with keyboard focus. 
`keyboard.send_virtual_key_break([0xe0], virtual_key)`
> Sends a **virtual key break** event to the application with keyboard focus.
`keyboard.send_scancode_make([0xe0], scancode)`
> Sends a **scancode make** event to the application with keyboard focus.
`keyboard.send_scancode_break([0xe0], scancode)`
> Sends a **scancode break** event to the application with keyboard focus.
---- 
There are a couple higher-level keyboard event generating functions.

`keyboard.send_keys(virtual_key,...)`
> This function sends a variable number of virtual key make/break pairs to the application with keyboard focus. The enhanced key flag value (**0xe0**) gets no special treatment. Example:
> 	keyboard.send_keys(vk.a) -- sends 2 events: make 'a', break 'a'
> 	keyboard.send_keys(vk.tab, vk.h, vk.i) -- sends 6 key events, which emulate pressing and releasing 3 keys: TAB 'h' 'i'
`keyboard.send_text([text],[virtual_key],...)`
> This function is used to simulate complex text input from the keyboard. The function accepts a variable number of strings and/or individual virtual keys as parameters.
> When the parameter type is a virtual key value, the function generates and sends a make/break pair exactly as `send_keys()` does.
> When the parameter type is a string of unicode (UTF-8) text, the function attempts to intelligently generate the appropriate virtual key make/break sequence to recreate the text string as keyboard input. This means, if your string includes characters (_technically:_ code-points) that are generated using modifier keys, those modifier key events will be emulated. For example, to send the character **‘a’**, the function need only send the make/break pair for **vk.a**. However, to send the character for a capital letter **’A’**, the function will send the following key event sequence:
> 1. **MAKE:** vk.shift
> 2. **MAKE:** vk.a
> 3. **BREAK:** vk.a
> 4. **BREAK:** vk.shift 

> Example usage:
> 	keyboard.send_text("aAbBCCdD", vk.e, "E", "f")

#### Virtual Key Metadata
Windows has some notion of metadata associated with many virtual keys. For ease of reference, some of the most useful metadata has been added into to the Lua environment of this program. The metadata in a inside the `keyboard` namespace. It may be accessed like this:

	-- Get some metadata about virtual key codes
	for i = 0, #keyboard.virtual_key_descriptions do
		print("VK: ", i, " - ", keyboard.virtual_key_descriptions[i]);
	end
	print(#keyboard.virtual_key_descriptions + 1, " key descriptions listed")

### Technical Notes

Intercepted key events trigger Lua callbacks _synchronously_ within the Windows keyboard key event processing queue. This is not necessarily a desirable behavior. It implies that a slow Lua callback would block additional keyboard events from being processed until the callback has completed whatever slow business it’s involved in. I believe that modern versions of Windows have some sort of time-out value to prevent total loss of keyboard input. Even so, slow keyboard hooks would be really annoying.

In the future, I would like the program’s default behavior to: enqueue all intercepted keyboard events for background processing by this program. Then, quickly return the keyboard processing thread to Windows. Thereby, making the interception mode operate _asynchronously_.

It may still be useful to have an optional synchronous mode in order to guarantee key event sequencing for certain applications. However, I would do more testing before spending time developing a better synchronous mode.

### Boost Software License - Version 1.0
- See accompanying file LICENSE\_1\_0.txt or copy at [http://www.boost.org/LICENSE\_1\_0.txt][1]

#### The Boost License Design Requirements:
- Must be simple to read and understand.
- Must grant permission without fee to copy, use and modify the software for any use (commercial and non-commercial).
- Must require that the license appear with all copies [including redistributions] of the software source code.
- Must not require that the license appear with executables or other binary uses of the library.
- Must not require that the source code be available for execution or other binary uses of the library.

[1]:	http://www.boost.org/LICENSE_1_0.txt