/* ============================================================================
  Surface decal model
  By Damian Trebilco
============================================================================ */

#include "../Framework3/Util/Model.h"

class SurfaceDecalModel : public Model
{
public:

  // Helper struct describing the raw material vertex data
  struct MatVertexData
  {
    float m_matWeight;    //!< The current vertex material weighting
    uint8 m_matSelect[4]; //!< The vertex material selection
  };

  SurfaceDecalModel();
  ~SurfaceDecalModel();


  /// Setup the vertex stream to contain the material IDs and blend weights
  /// (only call this after the model is in the final format)

  /// Get the triangle data at the specified index
  bool GetTriangleMatData(uint a_triIndex, MatVertexData & a_retData) const;

  /// Update the specified triangle with the new material vertex data
  void UpdateTriangle(Renderer *a_renderer, uint a_triIndex, const MatVertexData & a_newData);

  /// Get the material data at the specified vertex
  bool GetVertexMatData(uint a_vertexIndex, MatVertexData & a_retData) const;

  /// Update the vertex buffer with new data
  void UpdateVertex(Renderer *a_renderer, uint a_vertexIndex, const MatVertexData & a_newData);

  /// Get the vertex indices for the passed sphere
  void GetSphereVertices(const vec3 & a_pos, float a_radius, Array<uint> & a_retVertexIndices) const;

  /// Save out the extra vertex stream
  bool SaveVertexData(const char * a_fileName);

  /// Load in an addition vertex stream
  bool LoadVertexData(const char * a_fileName, Renderer *a_renderer);

  /// Re-order the index buffer so that each vertex has a known provoking vertex (add new vertices where necessary)
	virtual uint assemble(const StreamID *aStreams, const uint nStreams, float **destVertices, uint **destIndices, bool separateArrays);
 
  /// Create an extra vertex buffer and render format
  uint makeDrawable(Renderer *renderer, const bool useCache = true, const ShaderID shader = SHADER_NONE);
	void unmakeDrawable(Renderer *renderer);

  void setBuffers(Renderer *renderer);
	void draw(Renderer *renderer);
	void drawBatch(Renderer *renderer, const uint batch);
	void drawSubBatch(Renderer *renderer, const uint batch, const uint first, const uint count);

protected:

  MatVertexData * m_matVertexData;   //!< The raw vertex array

	VertexFormatID m_secondVertexFormat; //!< The vertex format including the second stream
	VertexBufferID m_secondVertexBuffer; //!< The second vertex buffer

};
