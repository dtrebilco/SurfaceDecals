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

GLint 
gluUnProject(GLdouble winx, GLdouble winy, GLdouble winz,
	     const GLdouble model[16], const GLdouble proj[16],
	     const GLint viewport[4],
	     GLdouble * objx, GLdouble * objy, GLdouble * objz);

// Structure containing all the editor only data
struct EditorData
{
  EditorData();

  bool m_isEditorMode;    //!< If currently in editor mode
  bool m_isLeftMouseDown; //!< State of the left mouse button 
  bool m_isRightMouseDown; //!< State of the right mouse button 

  bool m_debugRenderMode;  //!< Debug render mode
  bool m_wireframeOverlay; //!< If rendering a wireframe overlay

  float m_editSphereSize; //!< Size of the editor sphere
  vec3 m_editSpherePos;   //!< The editor sphere position

  bool m_editWeights;     //!< If editing vertex weights (otherwise editing material type)

  float m_weightInc;      //!< Weight edit increment (decrement if negative)

  int m_materialID;       //!< The current editing material ID
  int m_layerIndex;       //!< The current editing layer
};
EditorData s_editorData;


EditorData::EditorData()
: m_isEditorMode(false)
, m_isLeftMouseDown(false)
, m_isRightMouseDown(false)
, m_debugRenderMode(false)
, m_wireframeOverlay(true)
, m_editSphereSize(50.0f)
, m_editSpherePos(0.0f, 0.0f, 0.0f)
, m_editWeights(true)
, m_weightInc(1.0f)
, m_materialID(0)
, m_layerIndex(0)
{
}


void ConvertMatrix(const mat4 &srcMat, GLdouble outMat[16])
{
  mat4 srcTranspose = transpose(srcMat);

  // Loop and convert to double format
  const float * srcData = (const float*)srcTranspose.rows;
  for(int i=0; i< 16; i++){
    outMat[i] = srcData[i];
  }
}


bool App::GetCollisionTriangle(const int a_x, const int a_y, vec3 & a_colPoint, const BTri *& a_colTriangle)
{
  vec3 newPos;
  vec3 targetPos;

  // Calculate the screen target position
  {
    GLdouble modelview[16];
    GLdouble projection[16];
    GLint viewport[4] ={0, 0, width, height};

    GLdouble posX, posY, posZ;

    ConvertMatrix(m_projectionMatrix, projection);
    ConvertMatrix(m_modelviewMatrix, modelview);

    gluUnProject((GLdouble)a_x, (GLdouble)height - (GLdouble)a_y, 1.0,
         modelview, projection,
	       viewport,
	       &posX, &posY, &posZ);

    targetPos = vec3((float)posX, (float)posY, (float)posZ);
  }
  
  // Check for a collision between the last and new position
  return m_bsp.intersects(camPos, targetPos, &a_colPoint, &a_colTriangle);
}


void App::PaintWeights(const vec3 & a_spherePos, float a_sphereSize, bool a_add)
{
  // Get all the vertices in the sphere area
  Array<uint> indices;
  m_map->GetSphereVertices(a_spherePos, a_sphereSize, indices);
  for(uint i = 0; i < indices.getCount(); i++)
  {
    // Loop and update each vertex
    SurfaceDecalModel::MatVertexData updateData;
    uint vertIndex = indices[i];
    if(m_map->GetVertexMatData(vertIndex, updateData))
    {
      // Add or subtract the weight value
      if(a_add)
      {
        updateData.m_matWeight += (s_editorData.m_weightInc * frameTime);
      }
      else
      {
        updateData.m_matWeight -= (s_editorData.m_weightInc * frameTime);
      }
      updateData.m_matWeight = clamp(updateData.m_matWeight, 0.0f, 1.0f);

      m_map->UpdateVertex(renderer, vertIndex, updateData);
    }
  }
}


void App::PaintSurface(const int x, const int y)
{
  // Calculate the collision point into the BSP
  vec3 colPoint;
  const BTri * colTri;
  if(GetCollisionTriangle(x, y, colPoint, colTri))
  {
    // The triangle index is the collision data
    uint triIndex = (uint)(colTri->data);

    // Get the existing data
    SurfaceDecalModel::MatVertexData updateData;
    if(m_map->GetTriangleMatData(triIndex, updateData))
    {
      ASSERT(s_editorData.m_layerIndex >= 0 && s_editorData.m_layerIndex < 4);
      updateData.m_matSelect[s_editorData.m_layerIndex] = s_editorData.m_materialID;
      
      // Update the triangle data
      m_map->UpdateTriangle(renderer, triIndex, updateData);
    }
  }
}


bool App::GetSpherePosition(const int x, const int y)
{
  vec3 newPos;
  vec3 targetPos;

  // Calculate the screen target position
  {
    GLdouble modelview[16];
    GLdouble projection[16];
    GLint viewport[4] ={0 , 0, width, height};

    GLdouble posX, posY, posZ;

    ConvertMatrix(m_projectionMatrix, projection);
    ConvertMatrix(m_modelviewMatrix, modelview);

    float srcZ = 1.0f;
	  glReadPixels( x, height - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &srcZ );

    gluUnProject((GLdouble)x, (GLdouble)height - (GLdouble)y, srcZ, //1.0,
         modelview, projection,
	       viewport,
	       &posX, &posY, &posZ);

    targetPos = vec3((float)posX, (float)posY, (float)posZ);
  }

  // Get the new position as slightly offset from the surface intersection
  newPos = targetPos - (normalize(targetPos - camPos) * s_editorData.m_editSphereSize * 0.3f);

  /*
  // Check for a collision between the last and new position
  vec3 colPoint;
  const BTri *colTriangle;
  if(bsp.intersects(camPos, targetPos, &colPoint, &colTriangle)){

    // Don't land exactly on the plane, 
    newPos = colPoint + 10.0f * colTriangle->plane.xyz();
  }
  else{
    return false;
  }
*/

  // Assign the editor data new position
  s_editorData.m_editSpherePos = newPos;

  return true;
}


bool App::onKeyEditor(const uint key, const bool pressed)
{
  // Toggle in/out of editor mode
  if(key == KEY_E && pressed)
  {
    s_editorData.m_isEditorMode = !s_editorData.m_isEditorMode;
  }

  // If not in editor mode, return now
  if(!s_editorData.m_isEditorMode)
  {
    s_editorData.m_isLeftMouseDown = false;
    s_editorData.m_isRightMouseDown = false;
    return false;
  }

  if(pressed)
  {
    // Toggle the editor mode
    if(key == KEY_M)
    {
      s_editorData.m_editWeights = !s_editorData.m_editWeights;
    }

    // Save the editor changes
    if(key == KEY_S)
    {
      m_map->SaveVertexData("../Models/Room6/Map0.vd");
    }
    if(key == KEY_L)
    {
      m_map->LoadVertexData("../Models/Room6/Map0.vd", renderer);
    }

    if(key == KEY_1)
    {
      s_editorData.m_debugRenderMode = true;
    }

    if(key == KEY_2)
    {
      s_editorData.m_debugRenderMode = false;
    }

    if(key == KEY_3)
    {
      s_editorData.m_wireframeOverlay = !s_editorData.m_wireframeOverlay;
    }

    if(s_editorData.m_editWeights)
    {
      // Change weight intensity
      if(key == KEY_NUMPAD8)
      {
        s_editorData.m_weightInc += 0.1f;
      }
      if(key == KEY_NUMPAD2)
      {
        s_editorData.m_weightInc -= 0.1f;
      }

      s_editorData.m_weightInc = clamp(s_editorData.m_weightInc, 0.0f, 3.0f);
    }
    else
    {
      // Change material edit params
      if(key == KEY_NUMPAD8)
      {
        s_editorData.m_layerIndex += 1;
      }
      if(key == KEY_NUMPAD2)
      {
        s_editorData.m_layerIndex -= 1;
      }

      if(key == KEY_NUMPAD4)
      {
        s_editorData.m_materialID -= 1;
      }
      if(key == KEY_NUMPAD6)
      {
        s_editorData.m_materialID += 1;
      }

      // Wrap clamp
      if(s_editorData.m_layerIndex < 0)
      {
        s_editorData.m_layerIndex += 4;
      }
      if(s_editorData.m_layerIndex >= 4)
      {
        s_editorData.m_layerIndex -= 4;
      }

      if(s_editorData.m_materialID < 0)
      {
        s_editorData.m_materialID += 256;
      }
      if(s_editorData.m_materialID >= 256)
      {
        s_editorData.m_materialID -= 256;
      }
    }
  }

  return true;
}


bool App::onMouseWheelEditor(const int x, const int y, const int scroll)
{
  // If not in editor mode, return now
  if(!s_editorData.m_isEditorMode){
    return false;
  }

  // Adjust the sphere placement size
  s_editorData.m_editSphereSize = clamp(s_editorData.m_editSphereSize + (float)scroll, 0.0f, 700.0f);

  return true;
}


bool App::onMouseButtonEditor(const int x, const int y, const MouseButton button, const bool pressed)
{
  // If not in editor mode, return now
  if(!s_editorData.m_isEditorMode)
  {
    return false;
  }


  // Set if the mouse is down
  if(button == MOUSE_LEFT)
  {
    s_editorData.m_isLeftMouseDown = pressed;
  }
  if(button == MOUSE_RIGHT)
  {
    s_editorData.m_isRightMouseDown = pressed;
  }

  // Paint the surface if necessary
  if(!s_editorData.m_editWeights && 
     s_editorData.m_isLeftMouseDown)
  {
    PaintSurface(x, y);
  }

  return true;
}


bool App::onMouseMoveEditor(const int x, const int y, const int deltaX, const int deltaY)
{
  // If not in editor mode, return now
  if(!s_editorData.m_isEditorMode){
    return false;
  }

  // Get the new sphere position
  GetSpherePosition(x,y);

  // Paint the surface if necessary
  if(!s_editorData.m_editWeights && 
     s_editorData.m_isLeftMouseDown)
  {
    PaintSurface(x, y);
  }

  return true;
}


ShaderID App::GetRenderShader()
{
  // If it is the standard shader
  if(!s_editorData.m_isEditorMode ||
     !s_editorData.m_debugRenderMode)
  {
    return m_lightingMP_ambient;
  }
  
  // Return a debug renderer
  if(s_editorData.m_editWeights)
  {
    return m_debugVertexWeights;
  }
  else
  {
    return m_debugMatData;
  }
}


void App::SetDebugShaderConstants()
{
  // Set the debug shader constants if necessary
  if(s_editorData.m_isEditorMode && 
     s_editorData.m_debugRenderMode &&
     !s_editorData.m_editWeights)
  {
    renderer->setShaderConstant1f("debugMatIndex", (float)s_editorData.m_layerIndex);
  }
}


void App::drawFrameEditor()
{
  // If not in editor mode, return now
  if(!s_editorData.m_isEditorMode){
    return;
  }

  // Update the weights if necessary
  if(s_editorData.m_editWeights &&
     (s_editorData.m_isLeftMouseDown ||
      s_editorData.m_isRightMouseDown))
  {
    PaintWeights(s_editorData.m_editSpherePos, s_editorData.m_editSphereSize, s_editorData.m_isLeftMouseDown);
  }

  // Draw the wireframe if necessary
  if(s_editorData.m_wireframeOverlay)
  {
    glLineWidth(5.0f);

    renderer->reset();
    renderer->setShader(m_colorOnly);
    renderer->setRasterizerState(m_wireFrameRS);
    renderer->setDepthState(noDepthWrite);
    renderer->apply();

    renderer->setShaderConstant4f("outColor", vec4(1.0f, 1.0f, 1.0f, 1.0f));
    renderer->applyConstants();

    m_map->draw(renderer);
  }

  // Draw the light sphere volumes into the scene
  renderer->reset();
  renderer->setShader(m_lightingColorOnly);
  renderer->setBlendState(m_blendAdd);
  renderer->setRasterizerState(cullNone);
  renderer->setDepthState(noDepthWrite);
  renderer->apply();

  renderer->setShaderConstant3f("lightPos", s_editorData.m_editSpherePos);
  renderer->setShaderConstant1f("lightRadius", s_editorData.m_editSphereSize);
  renderer->setShaderConstant4f("outColor", vec4(0.5f, 0.5f, 0.0f, 0.5f));
  renderer->applyConstants();

  // Draw a sphere the radius of the light
  m_sphereModel->draw(renderer);

  // Draw text data to the screen 
	renderer->setup2DMode(0, (float) width, 0, (float) height);

  // Draw the current light index to the screen
	char str[512];
  int offset = 0;

  // Display the edit mode strings
  if(s_editorData.m_editWeights)
  {
	  renderer->drawText("Blend Weights Edit Mode", (float)width - (14 * 30) - 8, 8.0f + offset, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);

    offset += 38;
    sprintf(str, "Weight Adjustment %.2f", s_editorData.m_weightInc);
	  renderer->drawText(str, (float)width - (14 * 30) - 8, 8.0f + offset, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);
  }
  else
  {
    renderer->drawText("Material Edit Mode", (float)width - (14 * 30) - 8, 8.0f + offset, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);

    offset += 38;
    sprintf(str, "Material %d", s_editorData.m_materialID + 1); // Add 1 to be artist friendly
	  renderer->drawText(str, (float)width - (14 * 30) - 8, 8.0f + offset, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);

    offset += 38;
    sprintf(str, "Layer %d", s_editorData.m_layerIndex + 1); // Add 1 to be artist friendly
	  renderer->drawText(str, (float)width - (14 * 30) - 8, 8.0f + offset, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);
  }
}



