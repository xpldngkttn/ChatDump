# SimpleCube

Minimal renderer example using GLFW, GLEW and GLM. It displays a rotating cube with user configurable data defined in a `Cube` struct at the top of `main.cpp`.

To build on Linux with GCC:

```
apt-get install libglew-dev libglfw3-dev libglm-dev

g++ main.cpp -std=c++17 -lGLEW -lglfw -lGL -ldl -o cube
```

Run `./cube` and a window with a rotating cube should appear.

