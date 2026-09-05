# software-renderer

Software renderer in C++, inspired by [tinyrenderer](https://haqr.eu/tinyrenderer/).

Built to understand how GPUs (and APIs like OpenGL/Vulkan) work by implementing the pipeline with no graphics libraries.

## Build

```bash
cmake -B build
cmake --build build -j
./build/software-renderer
```

Opens an SDL2 window with the rendered frame (Esc to quit). Also writes `framebuffer.tga`.