/* ============================================================================
  Surface Decal Demo
  By Damian Trebilco
 
  Origional base lighting demo by "Humus"  
============================================================================ */

/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/

#include "../Framework3/OpenGL/OpenGLApp.h"
#include "../Framework3/Util/Model.h"
#include "../Framework3/Util/BSP.h"
#include "../Framework3/Math/Scissor.h"

#include "SurfaceDecalModel.h"


struct Decal
{
  mat4 m_matrix;
  mat4 m_orient;

	float3 m_position;
	float m_radius;
	float m_intensity;
};


class App : public OpenGLApp
{
public:

  App();
  char *getTitle() const { return "Surface Decals"; }

  void resetCamera();
  void moveCamera(const vec3 &dir);

  bool init();
  void exit();

  void onSize(const int w, const int h);
	bool onKey(const uint key, const bool pressed);
	bool onMouseMove(const int x, const int y, const int deltaX, const int deltaY);
	bool onMouseButton(const int x, const int y, const MouseButton button, const bool pressed);
	bool onMouseWheel(const int x, const int y, const int scroll);

  bool load();
  void unload();

  void drawLightingMPAmbient();
  void drawLightingMP();
  void drawDecalMask();

  void drawFrame();

protected:

  mat4 m_projectionMatrix;   //!< The current frame's projection matrix
  mat4 m_modelviewMatrix;    //!< The current frame's modelview matrix

  SurfaceDecalModel * m_map; //!< The rendering map
  BSP m_bsp;                 //!< The collision bsp 

  Model * m_sphereModel;     //!< Editor sphere model

  ShaderID m_depthOnly;         //!< The depth only shader
  ShaderID m_colorOnly;         //!< The flat color shader
  ShaderID m_lightingColorOnly; //!< The flat color shader for the sphere

  ShaderID m_decal;             //!< The decal render shader
  ShaderID m_lightingMP;      
  ShaderID m_lightingMP_ambient; //!< The main scene render shader
  
  ShaderID m_debugMatData;       //!< Debugging material data shader
  ShaderID m_debugVertexWeights; //!< Debugging vertex weights shader

  TextureID m_perlin;            //!< The perlin noise texture
  TextureID m_decalTex;          //!< Th decal texture

  TextureID m_texArray;          //!< The main diffuse texture array
  TextureID m_bumpTexArray;      //!< The main bump texture array

  TextureID m_depthRT;           //!< The depth only render target 
  TextureID m_depthTex;          //!< The depth only texture
  TextureID m_screenMask;        //!< The screen size mask texture

  SamplerStateID m_trilinearAniso;      //!< Texture filtering
  SamplerStateID m_trilinearAnisoClamp; //!< Texture filtering (with clamp wrapping)
  SamplerStateID m_pointSample;         //!< Texture point sampling

  BlendStateID m_blendAdd;         //!< Blend additive
  BlendStateID m_noColorWrite;     //!< No color writing

  RasterizerStateID m_wireFrameRS; //!< Wireframe rasterize

	Array <Decal> m_decals;

  // To remove
  float parallax[6];
  TextureID base[4], bump[4], gloss[4];

  // Get the triangle collision for the specified x,y screen coordinates
  bool GetCollisionTriangle(const int a_x, const int a_y, vec3 & a_colPoint, const BTri *& a_colTriangle);
  void UpdateDecals();

  // Position light editor methods
  bool GetSpherePosition(const int x, const int y);
  void PaintWeights(const vec3 & a_spherePos, float a_sphereSize, bool a_add);
  void PaintSurface(const int x, const int y);
  void SetDebugShaderConstants();

  // Get the render shader
  ShaderID GetRenderShader();

  bool onKeyEditor(const uint key, const bool pressed);
  bool onMouseWheelEditor(const int x, const int y, const int scroll);
  bool onMouseButtonEditor(const int x, const int y, const MouseButton button, const bool pressed);
  bool onMouseMoveEditor(const int x, const int y, const int deltaX, const int deltaY);
  void drawFrameEditor();
};
