#include "Mesh.h"

bool Mesh::HasMorphTargets() const
{
	for (const Submesh& submesh : submeshes)
	{
		if (HasFlag(submesh.flags, VertexAttribute::MORPH_TARGET0_POSITION))
		{
			return true;
		}
	}
	return false;
}
