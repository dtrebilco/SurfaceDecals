/* ============================================================================
  Surface decal model
  By Damian Trebilco
============================================================================ */

#include "SurfaceDecalModel.h"
#include "..\\Framework3\\Util\\Array.h"


SurfaceDecalModel::SurfaceDecalModel()
: m_matVertexData(NULL)
, m_secondVertexFormat(VF_NONE)
, m_secondVertexBuffer(VB_NONE) 
{
}



SurfaceDecalModel::~SurfaceDecalModel()
{
  delete [] m_matVertexData;

  // Delete vertex buffer and vertex format?

}


bool SurfaceDecalModel::GetTriangleMatData(uint a_triIndex, MatVertexData & a_retData) const
{
	if (lastVertexCount > 65535 ||
      !lastIndices ||
      ((a_triIndex * 3) + 2) >= nIndices )
  {
    return false;
  }

  unsigned short * indices = (unsigned short *) lastIndices;

  // Get the provoking vertex index (last vertex in OpenGL for triangles)
  uint vertexIndex = indices[(a_triIndex*3) + 2];

  // If the vertex is not in range, or the vertex buffer is not allocated, ignore
  if(vertexIndex >= lastVertexCount ||
     !m_matVertexData)
  {
    return false;
  }

  // Copy the material data
  a_retData = m_matVertexData[vertexIndex];

  return true;
}


void SurfaceDecalModel::UpdateTriangle(Renderer *a_renderer, uint a_triIndex, const MatVertexData & a_newData)
{
  // Get the vertex index
  uint vertexIndex = 0;

	if (lastVertexCount <= 65535 &&
      lastIndices &&
      ((a_triIndex * 3) + 2) < nIndices ){

    unsigned short * indices = (unsigned short *) lastIndices;

    // Get the provoking vertex index (last vertex in OpenGL for triangles)
    vertexIndex = indices[(a_triIndex*3) + 2];

    UpdateVertex(a_renderer, vertexIndex, a_newData);
	}
}


bool SurfaceDecalModel::GetVertexMatData(uint a_vertexIndex, MatVertexData & a_retData) const
{
  // If the vertex is not in range, or the vertex buffer is not allocated, ignore
  if(a_vertexIndex >= lastVertexCount ||
     !m_matVertexData)
  {
    return false;
  }

  a_retData = m_matVertexData[a_vertexIndex];

  return true;
}


void SurfaceDecalModel::UpdateVertex(Renderer *a_renderer, uint a_vertexIndex, const MatVertexData & a_newData)
{
  // If the vertex is not in range, or the vertex buffer is not allocated, ignore
  if(a_vertexIndex >= lastVertexCount ||
     !m_matVertexData)
  {
    return;
  }

  // Update the in memory copy
  m_matVertexData[a_vertexIndex] = a_newData;

  // Upload to the graphics card
  a_renderer->updateVertexBuffer(m_secondVertexBuffer, a_vertexIndex * sizeof(MatVertexData), sizeof(MatVertexData), &a_newData);
}


void SurfaceDecalModel::GetSphereVertices(const vec3 & a_pos, float a_radius, Array<uint> & a_retVertexIndices) const
{
  a_retVertexIndices.clear();

  // Get the number of vertices and stride
  uint componentCount = getComponentCount();

  if(lastVertexCount <= 0 ||
     !lastVertices ||
     componentCount < 3)
  {
    return;
  }

  float radiusSquared = a_radius * a_radius;

  // Loop for all vertices
  const float *currVertex = lastVertices;
  for(uint i = 0; i < lastVertexCount; i++)
  {
    // Assume the first 3 are the position
    const vec3 * testVertex = (const vec3*)currVertex;

    // If within the sphere
    vec3 diff = a_pos - *testVertex;
    if(dot(diff, diff) < radiusSquared)
    {
      a_retVertexIndices.add(i);
    }

    // Go to next vertex
    currVertex += componentCount;
  }
}


bool SurfaceDecalModel::SaveVertexData(const char * a_fileName)
{
  if(lastVertexCount == 0 ||
     m_matVertexData == NULL)
  {
    return false;
  }

  // Open the file for writing
	FILE *file = fopen(a_fileName, "wb");
	if (file == NULL)
  {
    return false;
  }

  // Write out the number of vertices
	fwrite(&lastVertexCount, sizeof(lastVertexCount), 1, file);

  // Write out each vertex
  fwrite(m_matVertexData, sizeof(MatVertexData), lastVertexCount, file);

  fclose(file);

  return true;
}


bool SurfaceDecalModel::LoadVertexData(const char * a_fileName, Renderer *a_renderer)
{
  if(lastVertexCount == 0 ||
     m_matVertexData == NULL)
  {
    return false;
  }

  // Open the file for reading
	FILE *file = fopen(a_fileName, "rb");
	if (file == NULL)
  {
    return false;
  }

  // Read in the vertex count
  uint32 fileVerexCount = 0;
	fread(&fileVerexCount, sizeof(fileVerexCount), 1, file);

  // Check if matching counts
  if(fileVerexCount != lastVertexCount)
  {
    fclose(file);
    return false;
  }
  
  // Read in the vertex data
	fread(m_matVertexData, sizeof(MatVertexData), fileVerexCount, file);

  fclose(file);

  // Upload to the graphics card
  a_renderer->updateVertexBuffer(m_secondVertexBuffer, 0, lastVertexCount * sizeof(MatVertexData), m_matVertexData);

  return true;
}


uint SurfaceDecalModel::assemble(const StreamID *aStreams, const uint nStreams, float **destVertices, uint **destIndices, bool separateArrays)
{
  uint retVert = Model::assemble(aStreams, nStreams, destVertices, destIndices, separateArrays);
  if(!retVert)
  {
    return 0;
  }

  // Re-order in indices such that each triangle has a unique provoking vertex (the vertex that will determine the flat color over the triangle)
  // In OpenGL, by default it is the last vertex (D3D uses first vertex)

  // Create an array to mark how ever vertex has been used 
  Array<int> isUsed;
  isUsed.setCount(retVert);
  memset(isUsed.getArray(), 0, sizeof(int) * retVert);

  uint *indices = *destIndices;

  // Loop over all the indices
  for(uint i = 0; i < nIndices; i+=3)
  {
    // If the existing provoking vertex is not used, leave as is
    if(!isUsed[indices[i+2]])
    {
      isUsed[indices[i+2]] = 1;
    }
    else
    {
      // Test second vertex
      if(!isUsed[indices[i+1]])
      {
        isUsed[indices[i+1]] = 1;

        // Rotate indices 
        uint tempLast = indices[i+2];

        indices[i+2] = indices[i+1];
        indices[i+1] = indices[i+0];
        indices[i+0] = tempLast;
      }
      else
      {
        // Test third vertex
        if(!isUsed[indices[i]])
        {
          isUsed[indices[i]] = 1;

          // Rotate indices 
          uint tempFirst = indices[i];

          indices[i+0] = indices[i+1];
          indices[i+1] = indices[i+2];
          indices[i+2] = tempFirst;
        }
        else
        {
          // Could potentailly see if connecting triangles could use a different provoking vertex to free up a vertex (store triangle ID in the isUsed array)

          // Add new vertex to the vertex array and set the new index (if not separateArrays)
          // (for now, just ignore)
          //retVert++; 
        }
      }
    }
  }

  // Resize the vertex array if added vertices

  return retVert;
}



uint SurfaceDecalModel::makeDrawable(Renderer *renderer, const bool useCache, const ShaderID shader)
{
  // Call base class first
  uint retVert = Model::makeDrawable(renderer, useCache, shader);
  if(!retVert)
  {
    return 0;
  }

  // Create the new vertex format with the extra vertex buffer
	FormatDesc *format = new FormatDesc[streams.getCount() + 2];
  {
	  uint i = 0;
    for (; i < streams.getCount(); i++){
		  format[i].stream = 0;
		  format[i].type   = streams[i].type;
		  format[i].format = FORMAT_FLOAT;
		  format[i].size   = streams[i].nComponents;
	  }

	  format[i].stream = 1;
	  format[i].type   = TYPE_GENERIC;
	  format[i].format = FORMAT_FLOAT;
	  format[i].size   = 1;
    i++;

	  format[i].stream = 1;
	  format[i].type   = TYPE_GENERIC;
	  format[i].format = FORMAT_UBYTE;
	  format[i].size   = 4;
  }

  // Create the second vertex format
	if ((m_secondVertexFormat = renderer->addVertexFormat(format, streams.getCount() + 2, shader)) == VF_NONE)
  {
    return 0;
  }
  delete [] format;

  // Allocate the vertex data if not allready allocated
  if(!m_matVertexData)
  {
    m_matVertexData = new MatVertexData[lastVertexCount];
    memset(m_matVertexData, 0, lastVertexCount * sizeof(MatVertexData));

    // Init with layers
    for(uint i = 0; i < lastVertexCount; i++)
    {
      m_matVertexData[i].m_matWeight = 0.0f;
      m_matVertexData[i].m_matSelect[0] = 0;
      m_matVertexData[i].m_matSelect[1] = 1;
      m_matVertexData[i].m_matSelect[2] = 2;
      m_matVertexData[i].m_matSelect[3] = 3;
    }
  }
  
  // Create the second vertex buffer
	if ((m_secondVertexBuffer = renderer->addVertexBuffer(lastVertexCount * sizeof(MatVertexData), STATIC, m_matVertexData)) == VB_NONE)
  {
    return 0;
  }

  return retVert;
}


void SurfaceDecalModel::unmakeDrawable(Renderer *renderer)
{
  Model::unmakeDrawable(renderer);

  // Delete vertex format

  // Delete vertex buffer

}


void SurfaceDecalModel::setBuffers(Renderer *renderer)
{
  Model::setBuffers(renderer);

  // Set the second vertex format
	renderer->setVertexFormat(m_secondVertexFormat);

  // Set the second vertex buffer
	renderer->setVertexBuffer(1, m_secondVertexBuffer);
}


void SurfaceDecalModel::draw(Renderer *renderer)
{
	ASSERT(vertexBuffer != VB_NONE);
	ASSERT(indexBuffer  != IB_NONE);

	renderer->changeVertexFormat(m_secondVertexFormat);
	renderer->changeVertexBuffer(0, vertexBuffer);
	renderer->changeVertexBuffer(1, m_secondVertexBuffer);
	renderer->changeIndexBuffer(indexBuffer);

	renderer->drawElements(PRIM_TRIANGLES, 0, nIndices, 0, lastVertexCount);
}


void SurfaceDecalModel::drawBatch(Renderer *renderer, const uint batch)
{
	ASSERT(vertexBuffer != VB_NONE);
	ASSERT(indexBuffer  != IB_NONE);

	renderer->changeVertexFormat(m_secondVertexFormat);
	renderer->changeVertexBuffer(0, vertexBuffer);
	renderer->changeVertexBuffer(1, m_secondVertexBuffer);
	renderer->changeIndexBuffer(indexBuffer);

	renderer->drawElements(PRIM_TRIANGLES, batches[batch].startIndex, batches[batch].nIndices, batches[batch].startVertex, batches[batch].nVertices);
}


void SurfaceDecalModel::drawSubBatch(Renderer *renderer, const uint batch, const uint first, const uint count)
{
	ASSERT(vertexBuffer != VB_NONE);
	ASSERT(indexBuffer  != IB_NONE);

	renderer->changeVertexFormat(m_secondVertexFormat);
	renderer->changeVertexBuffer(0, vertexBuffer);
	renderer->changeVertexBuffer(1, m_secondVertexBuffer);
	renderer->changeIndexBuffer(indexBuffer);

	int startIndex = batches[batch].startIndex + first;
	int indexCount = min(count, batches[batch].nIndices - first);

	renderer->drawElements(PRIM_TRIANGLES, startIndex, indexCount, batches[batch].startVertex, batches[batch].nVertices);
}

