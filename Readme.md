# OpenGL Transparency

Sample to demonstrate different ways to render a transparent 3D model.

Initial algorithm: Order Independent
Transparency with
Dual Depth Peeling (Bavoil and Myers 2008).

<div style="display: flex; flex-direction: row; justify-content: space-around;">
  <div style="text-align: center;">
    <img src="doc/dual_depth_peeling_medium.png"
         alt="nvidia dragon opengl2 dual depth peeling">
    <p>OpenGL 2</p>
  </div>
  <div style="text-align: center;">
    <img src="doc/dual_depth_peeling_gl3_medium.png"
         alt="nvidia dragon opengl3 and more dual depth peeling">
    <p>OpenGL 3+</p>
  </div>
</div>

## Links

- https://developer.download.nvidia.com/SDK/10/opengl/screenshots/samples/dual_depth_peeling.html
- https://developer.nvidia.com/content/transparency-or-translucency-rendering
- [DualDepthPeeling.pdf](doc/DualDepthPeeling.pdf)
- [OrderIndependentTransparency.pdf](doc/OrderIndependentTransparency.pdf)

## Dependencies

- [GLEW](https://github.com/nigels-com/glew): To load OpenGL symbols
- [FreeGLUT](https://github.com/freeglut/freeglut): To create OpenGL context
- [GLM](https://github.com/g-truc/glm): To manage OpenGL geometries
- [Open Asset Import Library](https://github.com/assimp/assimp): To load the 3D NVidia dragon mesh
- [FreeType](https://github.com/freetype/freetype): To create text glyph to render in OpenGL

All dependencies can be install by [vcpkg](https://github.com/microsoft/vcpkg) with `vcpkg install` or during the CMake configuration by setting `VCPKG_ROOT=[path/to/vcpkg]` and `PATH="$PATH:$VCPKG_ROOT"` environment variable. See https://learn.microsoft.com/fr-fr/vcpkg/get_started/get-started.
