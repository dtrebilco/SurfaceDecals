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

#include "App.h"

BaseApp *app = new App();


App::App()
: m_depthRT(TEXTURE_NONE)
{
}


void App::resetCamera()
{
  camPos = vec3(-557.0f, 135.0f, 5.8f);
  wx = 0.0634f;
  wy = -1.58f;
}


void App::moveCamera(const vec3 &dir)
{
  vec3 newPos = camPos + dir * (speed * frameTime);

  vec3 point;
  const BTri *tri;
  if (m_bsp.intersects(camPos, newPos, &point, &tri)){
    newPos = point + tri->plane.xyz();
  }
  m_bsp.pushSphere(newPos, 30);

  camPos = newPos;
}


bool App::init()
{
  m_map = new SurfaceDecalModel();
  if (!m_map->loadObj("../Models/Room6/Map.obj")){
    delete m_map;
    return false;
  }

  m_map->computeTangentSpace(true);
  m_map->cleanUp();

  {
    Stream stream = m_map->getStream(m_map->findStream(TYPE_VERTEX));
    vec3 *vertices = (vec3 *) stream.vertices;
    uint *indices = stream.indices;
    for (uint i = 0; i < m_map->getIndexCount(); i += 3){
      // Add the triangle index as the collsion data
      m_bsp.addTriangle(vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]], (void*)(i / 3));
    }

    m_bsp.build();
  }

  m_map->changeAllGeneric(true);

  // Create the render sphere model
  m_sphereModel = new Model();
  m_sphereModel->createSphere(3);
  m_sphereModel->cleanUp();

  int tab = configDialog->addTab("Rendering");
  
  // Select the rendering tab as the active tab
  configDialog->setCurrentTab(tab);

  return true;
}


void App::exit()
{
  delete m_map;
  delete m_sphereModel;
}


void App::onSize(const int w, const int h)
{
  OpenGLApp::onSize(w, h);

  if (renderer && 
      m_depthRT != TEXTURE_NONE)
  {
    // Make sure render targets are the size of the window
    renderer->resizeRenderTarget(m_depthRT, w, h, 1, 1, 0);
  }
}


bool App::onKey(const uint key, const bool pressed)
{
  // If in editor mode
  if(onKeyEditor(key, pressed)){
    return true;
  }

  return OpenGLApp::onKey(key, pressed);
}


bool App::onMouseMove(const int x, const int y, const int deltaX, const int deltaY)
{
  // If in editor mode
  if(onMouseMoveEditor(x, y, deltaX,deltaY)){
    return true;
  }

  return OpenGLApp::onMouseMove(x, y, deltaX,deltaY);
}


bool App::onMouseButton(const int x, const int y, const MouseButton button, const bool pressed)
{
  // If in editor mode
  if(onMouseButtonEditor(x, y, button, pressed)){
    return true;
  }

  if (pressed)
  {
    // Add a new decal at the indicated position
    if (button == MOUSE_LEFT)
    {
      float cosX = cosf(wx), sinX = sinf(wx), cosY = cosf(wy), sinY = sinf(wy);
      vec3 dz(-cosX * sinY, -sinX, cosX * cosY);

      float3 pos;
      const BTri *triData;
      if (m_bsp.intersects(camPos, camPos + 4000.0f * dz, &pos, &triData))
      {
        vec3 colNormal(triData->plane.x, triData->plane.y, triData->plane.z);
        vec3 camNormal = dz;

        // Calculate the direction vectors for the matrix
        vec3 fwd = camNormal - colNormal;
        if (dot(fwd, fwd) < 0.0001f)
        {
          fwd = vec3(0.0f, 0.0f, 1.0f);
        }
        fwd = normalize(fwd);

        vec3 right = cross(fwd, vec3(0.0f, 0.0f, 1.0f)); 
        if (dot(right, right) < 0.0001f)
        {
          right = cross(fwd, vec3(1.0f, 0.0f, 0.0f));
          if(dot(right, right) < 0.0001f)
          {
            right = cross(fwd, vec3(0.0f, 1.0f, 0.0f));
          }
        }
        right = normalize(right);
        vec3 up = cross(right, fwd);
      
        float x = float(rand()) * (2 * PI / RAND_MAX);
        float y = float(rand()) * (2 * PI / RAND_MAX);
        float z = float(rand()) * (2 * PI / RAND_MAX);

        const float radius = 120.0f + float(rand()) * (80.0f / RAND_MAX);

        Decal decal;
        decal.m_position = pos;
        decal.m_radius = radius;
        decal.m_intensity = 10.0f;
        decal.m_matrix = translate(0.5f, 0.5f, 0.5f) * scale(0.5f / radius, 0.5f / radius, 0.5f / radius) * rotateZXY(x, y, z) * translate(-pos);

        decal.m_orient = mat4(up.x, right.x, fwd.x, 0.0f,
                              up.y, right.y, fwd.y, 0.0f,
                              up.z, right.z, fwd.z, 0.0f,
                              0.0f, 0.0f, 0.0f, 1.0f);

        m_decals.add(decal);
      }
    }
  }

  return OpenGLApp::onMouseButton(x, y, button, pressed);
}


bool App::onMouseWheel(const int x, const int y, const int scroll)
{
  // If in editor mode
  if(onMouseWheelEditor(x, y, scroll)){
    return true;
  }

  return OpenGLApp::onMouseWheel(x, y, scroll);
}


bool App::load()
{
  // Just require OpenGL 3.0
  if(!GLVER(3,0))
  {
    ErrorMsg("No GL 3.0 support");
    return false;
  }

  // Set the shader version used
  ((OpenGLRenderer*)renderer)->SetShaderVersionStr("#version 130");

  if (!m_map->makeDrawable(renderer)) return false;
  if (!m_sphereModel->makeDrawable(renderer)) return false;

  // Load in the vertex data for the map (should be done once, not every time the render is reset)
  m_map->LoadVertexData("../Models/Room6/Map0.vd", renderer);

  // Samplerstates
  if ((m_trilinearAniso = renderer->addSamplerState(TRILINEAR_ANISO, WRAP, WRAP, WRAP)) == SS_NONE) return false;
  if ((m_trilinearAnisoClamp = renderer->addSamplerState(TRILINEAR_ANISO, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;
  if ((m_pointSample = renderer->addSamplerState(NEAREST, CLAMP, CLAMP, CLAMP)) == SS_NONE) return false;

  // FBO
  int fboDepthBits = 24;

  // Create the fullscreen buffers
  if ((m_depthRT = renderer->addRenderDepth(width, height, fboDepthBits)) == TEXTURE_NONE) return false;
  if ((m_depthTex = renderer->addRenderTarget(width, height, FORMAT_RGB32F, m_pointSample)) == TEXTURE_NONE) return false;
  if ((m_screenMask = renderer->addRenderTarget(width, height, FORMAT_RGBA8, m_pointSample)) == TEXTURE_NONE) return false;
  
  // Shaders
  const char *attribs[] = { NULL, "textureCoord", "tangent", "binormal", "normal", "matWeight", "matIndices" };
  if ((m_depthOnly = renderer->addShader("depthOnly.shd")) == SHADER_NONE) return false;

  if ((m_lightingColorOnly = renderer->addShader("lightingColorOnly.shd")) == SHADER_NONE) return false;
  
  if ((m_decal = renderer->addShader("decal.shd")) == SHADER_NONE) return false;
  if ((m_lightingMP = renderer->addShader("lightingMP.shd", attribs, elementsOf(attribs))) == SHADER_NONE) return false;
  if ((m_lightingMP_ambient = renderer->addShader("lightingMP_ambient.shd", attribs, elementsOf(attribs))) == SHADER_NONE) return false;

  // Debug shaders
  if ((m_debugMatData = renderer->addShader("lightingMP_ambient.shd", attribs, elementsOf(attribs), "#define DEBUG_MAT_DATA\n")) == SHADER_NONE) return false;
  if ((m_debugVertexWeights = renderer->addShader("lightingMP_ambient.shd", attribs, elementsOf(attribs), "#define DEBUG_VERTEX_WEIGHTS\n")) == SHADER_NONE) return false;
  
  if ((m_colorOnly = renderer->addShader("PlainColor.shd", attribs, elementsOf(attribs))) == SHADER_NONE) return false;

  const char * diffuseTexArrayNames[] =
  {
   "../Textures/floor_wood_3.dds",
   "../Textures/brick01.dds",
   "../Textures/stone08.dds",
   "../Textures/StoneWall_1-4.dds",
   "../Textures/Leaves.dds",
   "../Textures/Brick02.dds",
  };

  if ((m_texArray = renderer->addTexture  (diffuseTexArrayNames, true, m_trilinearAniso, elementsOf(diffuseTexArrayNames))) == SHADER_NONE) return false;


  const char * bumpTexArrayNames[] =
  {
   "../Textures/floor_wood_3Bump.dds",
   "../Textures/brick01Bump.dds",
   "../Textures/stone08Bump.dds",
   "../Textures/StoneWall_1-4Bump.dds",
   "../Textures/LeavesBump.dds",
   "../Textures/Brick02Bump.dds",
  };

  if ((m_bumpTexArray = renderer->addTexture  (bumpTexArrayNames, true, m_trilinearAniso, elementsOf(bumpTexArrayNames))) == SHADER_NONE) return false;

  if ((m_decalTex = renderer->addTexture ("../Textures/decaltest.dds", true, m_trilinearAnisoClamp)) == SHADER_NONE) return false;

  if ((m_perlin = renderer->addTexture ("../Textures/Perlin.dds", true, m_trilinearAniso)) == SHADER_NONE) return false;

  // Textures
  if ((base[0] = renderer->addTexture  ("../Textures/floor_wood_3.dds",                   true, m_trilinearAniso)) == SHADER_NONE) return false;
  if ((bump[0] = renderer->addNormalMap("../Textures/floor_wood_3Bump.dds", FORMAT_RGBA8, true, m_trilinearAniso)) == SHADER_NONE) return false;
  parallax[0] = 0.04f;

  if ((base[1] = renderer->addTexture  ("../Textures/brick01.dds",                   true, m_trilinearAniso)) == SHADER_NONE) return false;
  if ((bump[1] = renderer->addNormalMap("../Textures/brick01Bump.dds", FORMAT_RGBA8, true, m_trilinearAniso)) == SHADER_NONE) return false;
  parallax[1] = 0.04f;

  if ((base[2] = renderer->addTexture  ("../Textures/stone08.dds",                   true, m_trilinearAniso)) == SHADER_NONE) return false;
  if ((bump[2] = renderer->addNormalMap("../Textures/stone08Bump.dds", FORMAT_RGBA8, true, m_trilinearAniso)) == SHADER_NONE) return false;
  parallax[2] = 0.04f;

  if ((base[3] = renderer->addTexture  ("../Textures/StoneWall_1-4.dds",                   true, m_trilinearAniso)) == SHADER_NONE) return false;
  if ((bump[3] = renderer->addNormalMap("../Textures/StoneWall_1-4Bump.dds", FORMAT_RGBA8, true, m_trilinearAniso)) == SHADER_NONE) return false;
  parallax[3] = 0.03f;
  parallax[4] = 0.02f;
  parallax[5] = 0.01f;

  // Blendstates
  if ((m_blendAdd = renderer->addBlendState(ONE, ONE)) == BS_NONE) return false;
  if ((m_noColorWrite = renderer->addBlendState(ONE, ZERO, BM_ADD, NONE)) == BS_NONE) return false;
  
  // Raster states
  if((m_wireFrameRS = renderer->addRasterizerState(CULL_NONE, WIREFRAME)) == RS_NONE) return false;

  // Huh? This is not already all 1's? (according to the spec? - Nvidia bug if main surface does not have stencil?)
  glStencilMask(0xFFFFFFFF); 

  return true;
}


void App::unload()
{
}


void App::drawDecalMask()
{
  float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  // Being lazy here - should re-use the depth buffer as a texture 
  // (unsure what weird drivers may do however)
  renderer->changeRenderTarget(m_depthTex, m_depthRT);
  renderer->clear(true, true, true, clearColor);

  renderer->reset();
  renderer->setShader(m_depthOnly);
  renderer->setRasterizerState(cullBack);
  renderer->apply();

  m_map->draw(renderer);

  // Render to the screen mask texture
  renderer->changeRenderTarget(m_screenMask, m_depthRT);
  renderer->clear(true, false, false, clearColor);

  // Loop and render all the decal meshes
  renderer->reset();
  renderer->setShader(m_decal);
  //renderer->setShaderConstant4x4f("ViewProj", viewProj);
  renderer->setShaderConstant2f("pixelSize", float2(1.0f / width, 1.0f / height));
  renderer->setTexture("depthTex", m_depthTex);
  renderer->setTexture("decalTex", m_decalTex);
  renderer->setBlendState(m_blendAdd); // Add only writing to alpha channel?
  renderer->setDepthState(noDepthWrite);
  renderer->setRasterizerState(cullBack);
  renderer->apply();

  for (uint i = 0; i < m_decals.getCount(); i++)
  {
    // Test if the sphere is in the frusturm (lazy scissor test - should do a simple sphere -> frustum test for speed)
    int dummy[4];
    if(getScissorRectangle(m_modelviewMatrix, m_decals[i].m_position, m_decals[i].m_radius, 1.5f, width, height, &dummy[0], &dummy[1], &dummy[2], &dummy[3]))
    {
      renderer->setShaderConstant3f("pos", m_decals[i].m_position);
      renderer->setShaderConstant1f("radius", m_decals[i].m_radius);
      renderer->setShaderConstant4x4f("orient", m_decals[i].m_orient);
      renderer->setShaderConstant1f("matIntensity", m_decals[i].m_intensity);
      //renderer->setShaderConstant4x4f("ScreenToLocal", m_decals[i].m_matrix * viewProjInv);
      renderer->applyConstants();

      m_sphereModel->draw(renderer);
    }
  }

  // Could use the RGB channels as a differed light indexed texture

  renderer->changeToMainFramebuffer();
}


void App::drawLightingMPAmbient()
{
  // Make sure depth writes are on
  renderer->changeDepthState(DS_NONE);
  glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  // Get the shader to render with (or abort)
  ShaderID renderShader = GetRenderShader();
  if(renderShader == SHADER_NONE)
  {
    return;
  }

  renderer->reset();
  renderer->setShader(renderShader);
  renderer->setRasterizerState(cullBack);
  renderer->apply();

  vec2 parallaxArray[6];
  for(uint i = 0; i < elementsOf(parallaxArray); i++)
  {
    parallaxArray[i] = vec2(2, -1) * parallax[i] * 2.0f;
  }

  renderer->setTexture("diffuseArray", m_texArray);
  renderer->setTexture("bumpArray", m_bumpTexArray);
  renderer->setTexture("perlinNoise", m_perlin);
  renderer->setTexture("screenMask", m_screenMask);
  renderer->applyTextures();

  renderer->setShaderConstant2f("pixelSize", float2(1.0f / width, 1.0f / height));
  renderer->setShaderConstant3f("camPos", camPos);
  renderer->setShaderConstantArray2f("plxCoeffsArray", parallaxArray, elementsOf(parallaxArray));
  SetDebugShaderConstants();
  renderer->applyConstants();

  m_map->draw(renderer);
}


void App::drawLightingMP()
{
  renderer->reset();
  renderer->setShader(m_lightingMP);
  renderer->setShaderConstant3f("camPos", camPos);
  renderer->setRasterizerState(cullBack);
  renderer->setBlendState(m_blendAdd);
  renderer->setDepthState(noDepthWrite);
  renderer->apply();

  for (uint k = 0; k < 4; k++){
    renderer->setTexture("Base", base[k]);
    renderer->setTexture("Bump", bump[k]);
    renderer->applyTextures();

    renderer->setShaderConstant1i("hasParallax", int(parallax[k] > 0.0f));
    renderer->setShaderConstant2f("plxCoeffs", vec2(2, -1) * parallax[k]);

    glEnable(GL_SCISSOR_TEST);

    //for(uint i=0; i<MAX_LIGHT_TOTAL; i++)
    {

      {
        //glScissor(lightDataArray[i].screenX, lightDataArray[i].screenY, lightDataArray[i].screenWidth, lightDataArray[i].screenHeight);

        //renderer->setShaderConstant3f("lightColor", lightDataArray[i].color);
        //renderer->setShaderConstant3f("lightPos", lightDataArray[i].position);
        //renderer->setShaderConstant1f("invRadius", 1.0f / lightDataArray[i].size);
        //renderer->applyConstants();
    
        m_map->drawBatch(renderer, k);
      }
    }
    glDisable(GL_SCISSOR_TEST);
  }

}

void App::UpdateDecals()
{
  //Loop over all decals and remove expired ones
  for(int i = m_decals.getCount() - 1; i >= 0; i--)
  {
    m_decals[i].m_intensity -= frameTime;
    if(m_decals[i].m_intensity <= 0.0f)
    {
      m_decals.fastRemove(i);
    }
  }
}


void App::drawFrame()
{
  // Update and load the modelview and projection matrices
  m_projectionMatrix = perspectiveMatrixX(1.5f, width, height, 5, 4000);
  m_modelviewMatrix = rotateXY(-wx, -wy);
  m_modelviewMatrix.translate(-camPos);

  glMatrixMode(GL_PROJECTION);
  glLoadTransposeMatrixfARB(m_projectionMatrix);

  glMatrixMode(GL_MODELVIEW);
  glLoadTransposeMatrixfARB(m_modelviewMatrix);

  // Update the decals for time
  UpdateDecals();

  // Draw the decal mask texture
  drawDecalMask();

  // Render the lights using a forward render pass
  drawLightingMPAmbient();
  //drawLightingMP();

  // Draw the editor data (if in editor mode)
  drawFrameEditor();
}

