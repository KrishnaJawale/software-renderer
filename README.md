# software-renderer

Software renderer in C++, inspired by [tinyrenderer](https://haqr.eu/tinyrenderer/).

Built to understand how GPUs (and APIs like OpenGL/Vulkan) work by implementing the pipeline with no graphics libraries.

## Images
<img width="525" height="983" alt="image" src="https://github.com/user-attachments/assets/3d82400b-536d-4260-80c5-a27d724a3ff3" />


## Build

```bash
cmake -B build
cmake --build build -j
./build/software-renderer
```

Opens an SDL2 window with the rendered frame (Esc to quit). Also writes `framebuffer.tga`.
