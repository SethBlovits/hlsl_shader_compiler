# hlsl_shader_compiler
shader compiler for buildstep (directx/windows)

This is a newer version of the shader compiler I wrote that for use with SLUGS.

It's currently only for DirectX12 and windows. When you run the executable with your build step, whether that's a .bat file or something more advanced like CMAKE, it will scrape the directory of wherever the .exe is located and seacrch for .hlsl files.
Once it finds one, it compiles them into their respective pixel and vertex shader .cso files, and then generates a .h file that holds onto the binding layouts that I use later inside of any SLUGS program.
It's not super flexible right now so I may make it accept a command line argument with the filepath of where you want it to run and have it recursively drill down through you file directory from there.

This I just changed this file in advance of a structural change that I am making to slugs, so I currently don't have a sample or anything like that.
