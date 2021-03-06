#include "llightscene.h"
#include "scene/3d/mesh_instance.h"
#include "llighttests_simd.h"
#include "llightmapper_base.h"
#include "llighttypes.h"


//#define LLIGHTSCENE_VERBOSE

using namespace LM;

bool LightScene::TestVoxelHits(const Ray &ray, const PackedRay &pray, const Voxel &voxel, float max_dist, bool bCullBackFaces)
{
	int quads = voxel.m_PackedTriangles.size();

	if (!bCullBackFaces)
	{
		for (int q=0; q<quads; q++)
		{
			// get pointers to 4 triangles
			const PackedTriangles & ptris = voxel.m_PackedTriangles[q];
			if (pray.IntersectTest(ptris, max_dist))
				return true;
		}
	}
	else
	{
		/*
		// test backface culling
		Tri t;
		t.pos[0] = Vector3(0, 0, 0);
		t.pos[2] = Vector3(1, 1, 0);
		t.pos[1] = Vector3(1, 0, 0);
		t.ConvertToEdgeForm();
		PackedTriangles pttest;
		pttest.Create();
		pttest.Set(0, t);
		pttest.Set(1, t);
		pttest.Set(2, t);
		pttest.Set(3, t);
		pttest.Finalize(4);

		Ray rtest;
		rtest.o = Vector3(0.5, 0.4, -1);
		rtest.d = Vector3(0, 0, 1);
		PackedRay prtest;
		prtest.Create(rtest);

		prtest.IntersectTest_CullBackFaces(pttest, 10);
		*/

		for (int q=0; q<quads; q++)
		{
			// get pointers to 4 triangles
			const PackedTriangles & ptris = voxel.m_PackedTriangles[q];
			if (pray.IntersectTest_CullBackFaces(ptris, max_dist))
				return true;
		}
	}

	return false;
}


void LightScene::ProcessVoxelHits(const Ray &ray, const PackedRay &pray, const Voxel &voxel, float &r_nearest_t, int &r_nearest_tri)//, int ignore_triangle_id_p1)
{
	//#define LLIGHTMAPPED_DEBUG_COMPARE_SIMD

//	float record_nearest_t = r_nearest_t;
//	int record_nearest_tri = r_nearest_tri;

//#ifdef LLIGHTMAPPER_USE_SIMD
	if (m_bUseSIMD)
	{
		//LightTests_SIMD simd;

		// groups of 4
		int quads = voxel.m_PackedTriangles.size();

		// special case of ignore triangles being set
//		int ignore_quad = -1;
//		int ignore_quad_index;
//		if (ignore_triangle_id_p1)
//		{
//			int test_tri_id = ignore_triangle_id_p1-1;

//			for (int n=0; n<voxel.m_iNumTriangles; n++)
//			{
//				if (voxel.m_TriIDs[n] == test_tri_id)
//				{
//					// found
//					ignore_quad = n / 4;
//					ignore_quad_index = n % 4;
//				}
//			}
//		}

		for (int q=0; q<quads; q++)
		{
			// get pointers to 4 triangles
			const PackedTriangles & ptris = voxel.m_PackedTriangles[q];

			//  we need to deal with the special case of the ignore_triangle being set. This will normally only occur on the first voxel.
			int winner;
			// if there is no ignore triangle, or the quad is not the one with the ignore triangle, do the fast path
//			if ((!ignore_triangle_id_p1) || (q != ignore_quad))
//			{
				winner = pray.Intersect(ptris, r_nearest_t);
//			}
//			else
//			{
//				// slow path
//				PackedTriangles pcopy = ptris;
//				pcopy.MakeInactive(ignore_quad_index);
//				winner = pray.Intersect(pcopy, r_nearest_t);
//			}

			if (winner != 0)
			//if (pray.Intersect(ptris, r_nearest_t, winner))
			{
				winner--;
				int winner_tri_index = (q * 4) + winner;
				//int winner_tri = m_Tracer.m_TriHits[winner_tri_index];
				int winner_tri = voxel.m_TriIDs[winner_tri_index];

				/*
			// test assert condition
			if (winner_tri != ref_winner_tri_id)
			{
				// do again to debug
				float test_nearest_t = FLT_MAX;
				simd.TestIntersect4(pTris, ray, test_nearest_t, winner);

				// repeat reference test
				int ref_start = nStart-4;
				for (int n=0; n<4; n++)
				{
					unsigned int tri_id = m_Tracer.m_TriHits[ref_start++];

					float t = 0.0f;
					ray.TestIntersect_EdgeForm(m_Tris_EdgeForm[tri_id], t);

				}
			}
			*/


				r_nearest_tri = winner_tri;
			}

			//		assert (r_nearest_t <= (nearest_ref_dist+0.001f));
		}

		// print result
//		if (r_nearest_tri != -1)
//			print_line("SIMD\tr_nearest_tri " + itos (r_nearest_tri) + " dist " + String(Variant(r_nearest_t)));

		return;
	} // if use SIMD
//#endif

	// trace after every voxel
	int nHits = m_Tracer.m_TriHits.size();
	int nStart = 0;


	// just for debugging do whole test again
//	int simd_nearest_tri = r_nearest_tri;
//	r_nearest_t = record_nearest_t;
//	r_nearest_tri = record_nearest_tri;

	// leftovers
	for (int n=nStart; n<nHits; n++)
	{
		unsigned int tri_id = m_Tracer.m_TriHits[n];

		float t = 0.0f;
		//			if (ray.TestIntersect(m_Tris[tri_id], t))
		if (ray.TestIntersect_EdgeForm(m_Tris_EdgeForm[tri_id], t))
		{
			if (t < r_nearest_t)
			{
				r_nearest_t = t;
				r_nearest_tri = tri_id;
			}
		}
	}

	// print result
//	if (r_nearest_tri != -1)
//		print_line("REF\tr_nearest_tri " + itos (r_nearest_tri) + " dist " + String(Variant(r_nearest_t)));

//	assert (r_nearest_tri == simd_nearest_tri);
}



void LightScene::ProcessVoxelHits_Old(const Ray &ray, const Voxel &voxel, float &r_nearest_t, int &r_nearest_tri)
{
	// trace after every voxel
	int nHits = m_Tracer.m_TriHits.size();
	int nStart = 0;

	//#define LLIGHTMAPPED_DEBUG_COMPARE_SIMD

#ifdef LLIGHTMAPPER_USE_SIMD
	if (m_bUseSIMD)
	{
		LightTests_SIMD simd;

		// groups of 4
		int quads = nHits / 4;


		for (int q=0; q<quads; q++)
		{
			// get pointers to 4 triangles
			const Tri * pTris[4];
#if LLIGHTMAPPED_DEBUG_COMPARE_SIMD
			float nearest_ref_dist = FLT_MAX;
			int ref_winner_tri_id = -1;
			int ref_winner_n = -1;
#endif

			for (int n=0; n<4; n++)
			{
				unsigned int tri_id = m_Tracer.m_TriHits[nStart++];

#if LLIGHTMAPPED_DEBUG_COMPARE_SIMD
				float t = 0.0f;
				print_line ("ref input triangle" + itos(n));
				const Tri &ref_input_tri = m_Tris_EdgeForm[tri_id];
				String sz = "\t";
				for (int abc=0; abc<3; abc++)
				{
					sz += "(" + String(Variant(ref_input_tri.pos[abc])) + ") ";
				}
				print_line(sz);

				if (ray.TestIntersect_EdgeForm(ref_input_tri, t))
				{
					if (t < nearest_ref_dist)
					{
						nearest_ref_dist = t;
						ref_winner_tri_id = tri_id;
						ref_winner_n = n;
					}
				}
#endif

				pTris[n] = &m_Tris_EdgeForm[tri_id];
			}

			// compare with old

			// test 4
			//		int test[4];
			int winner;
			if (simd.TestIntersect4(pTris, ray, r_nearest_t, winner))
			{
				int winner_tri_index = nStart-4 + winner;
				int winner_tri = m_Tracer.m_TriHits[winner_tri_index];

				/*
			// test assert condition
			if (winner_tri != ref_winner_tri_id)
			{
				// do again to debug
				float test_nearest_t = FLT_MAX;
				simd.TestIntersect4(pTris, ray, test_nearest_t, winner);

				// repeat reference test
				int ref_start = nStart-4;
				for (int n=0; n<4; n++)
				{
					unsigned int tri_id = m_Tracer.m_TriHits[ref_start++];

					float t = 0.0f;
					ray.TestIntersect_EdgeForm(m_Tris_EdgeForm[tri_id], t);

				}
			}
			*/


				r_nearest_tri = winner_tri;
			}

			//		assert (r_nearest_t <= (nearest_ref_dist+0.001f));
		}

	} // if use SIMD
#endif

	// leftovers
	for (int n=nStart; n<nHits; n++)
	{
		unsigned int tri_id = m_Tracer.m_TriHits[n];

		float t = 0.0f;
		//			if (ray.TestIntersect(m_Tris[tri_id], t))
		if (ray.TestIntersect_EdgeForm(m_Tris_EdgeForm[tri_id], t))
		{
			if (t < r_nearest_t)
			{
				r_nearest_t = t;
				r_nearest_tri = tri_id;
			}
		}
	}

}

bool LightScene::TestIntersect_Ray(const Ray &ray, float max_dist, const Vec3i &voxel_range, bool bCullBackFaces)
{
	Ray voxel_ray;
	Vec3i ptVoxel;

	// prepare voxel trace
	if (!m_Tracer.RayTrace_Start(ray, voxel_ray, ptVoxel))
		return false;

	//bool bFirstHit = false;
//	Vec3i ptVoxelFirstHit;

	// if we have specified a (optional) maximum range for the trace in voxels
	Vec3i ptVoxelStart = ptVoxel;

	// create the packed ray as a once off and reuse it for each voxel
	PackedRay pray;
	pray.Create(ray);

	while (true)
	{
		//Vec3i ptVoxelBefore = ptVoxel;

		const Voxel * pVoxel = m_Tracer.RayTrace(voxel_ray, voxel_ray, ptVoxel);
		if (!pVoxel)
			break;

		if (TestVoxelHits(ray, pray, *pVoxel, max_dist, bCullBackFaces))
			return true;

		// check for voxel range
		if (abs(ptVoxel.x - ptVoxelStart.x) > voxel_range.x)
			break;
		if (abs(ptVoxel.y - ptVoxelStart.y) > voxel_range.y)
			break;
		if (abs(ptVoxel.z - ptVoxelStart.z) > voxel_range.z)
			break;

	} // while

	return false;
}


int LightScene::FindIntersect_Ray(const Ray &ray, float &u, float &v, float &w, float &nearest_t, const Vec3i * pVoxelRange, int &num_tests)//, int ignore_tri_p1)
{
	nearest_t = FLT_MAX;
	int nearest_tri = -1;

	Ray voxel_ray;
	Vec3i ptVoxel;

	// prepare voxel trace
	if (!m_Tracer.RayTrace_Start(ray, voxel_ray, ptVoxel))
		return nearest_tri;

	bool bFirstHit = false;
	Vec3i ptVoxelFirstHit;

	// if we have specified a (optional) maximum range for the trace in voxels
	Vec3i ptVoxelStart;
	if (pVoxelRange)
	{
		ptVoxelStart = ptVoxel;
	}

	// create the packed ray as a once off and reuse it for each voxel
	PackedRay pray;
	pray.Create(ray);

	while (true)
	{
		Vec3i ptVoxelBefore = ptVoxel;

		const Voxel * pVoxel = m_Tracer.RayTrace(voxel_ray, voxel_ray, ptVoxel);
//		if (!m_Tracer.RayTrace(voxel_ray, voxel_ray, ptVoxel))
		if (!pVoxel)
			break;

		ProcessVoxelHits(ray, pray, *pVoxel, nearest_t, nearest_tri);

		// count number of tests for stats
				int nHits = m_Tracer.m_TriHits.size();
				num_tests += nHits;

		// first hit?
		if (!bFirstHit)
		{
			if (nearest_tri != -1)
			{
				bFirstHit = true;
				ptVoxelFirstHit = ptVoxelBefore;
			}
			else
			{
				// check for voxel range
				if (pVoxelRange)
				{
					if (abs(ptVoxel.x - ptVoxelStart.x) > pVoxelRange->x)
						break;
					if (abs(ptVoxel.y - ptVoxelStart.y) > pVoxelRange->y)
						break;
					if (abs(ptVoxel.z - ptVoxelStart.z) > pVoxelRange->z)
						break;
				} // if voxel range
			}
		}
		else
		{
			// out of range of first voxel?
			if (abs(ptVoxel.x - ptVoxelFirstHit.x) > 1)
				break;
			if (abs(ptVoxel.y - ptVoxelFirstHit.y) > 1)
				break;
			if (abs(ptVoxel.z - ptVoxelFirstHit.z) > 1)
				break;
		}

#ifdef LIGHTTRACER_IGNORE_VOXELS
		break;
#endif

	} // while

	if (nearest_tri != -1)
	{
		ray.FindIntersect(m_Tris[nearest_tri], nearest_t, u, v, w);
	}

	return nearest_tri;
}

int LightScene::IntersectRay_old(const Ray &r, float &u, float &v, float &w, float &nearest_t) const
{
	nearest_t = FLT_MAX;
	int nearest_tri = -1;

	for (int n=0; n<m_Tris.size(); n++)
	{
		float t = 0.0f;
		if (r.TestIntersect(m_Tris[n], t))
		{
			if (t < nearest_t)
			{
				nearest_t = t;
				nearest_tri = n;
			}
		}
	}


	if (nearest_tri != -1)
	{
		r.FindIntersect(m_Tris[nearest_tri], nearest_t, u, v, w);
	}

	return nearest_tri;
}

void LightScene::Reset()
{
//	m_ptPositions.resize(0);
//	m_ptNormals.resize(0);
	//m_UVs.resize(0);
	//m_Inds.resize(0);

	m_UVTris.clear(true);
	m_TriUVaabbs.clear(true);
	m_TriPos_aabbs.clear(true);
	m_Tracer.Reset();
	m_Tri_TexelSizeWorldSpace.clear(true);

	m_Tris.clear(true);
	m_TriNormals.clear(true);
	m_Tris_EdgeForm.clear(true);
	m_TriPlanes.clear(true);

	m_Meshes.clear(true);
	m_Tri_MeshIDs.clear(true);
}

void LightScene::FindMeshes(Spatial * pNode)
{
	// mesh instance?
	MeshInstance * pMI = Object::cast_to<MeshInstance>(pNode);
	if (pMI)
	{
		if (IsMeshInstanceSuitable(*pMI))
		{
			m_Meshes.push_back(pMI);
		}
	}

	for (int n=0; n<pNode->get_child_count(); n++)
	{
		Spatial * pChild = Object::cast_to<Spatial>(pNode->get_child(n));
		if (pChild)
		{
			FindMeshes(pChild);
		}
	}
}

bool LightScene::Create_FromMesh(int mesh_id, int width, int height)
{
	const MeshInstance &mi = *m_Meshes[mesh_id];

	Ref<Mesh> rmesh = mi.get_mesh();
	Array arrays = rmesh->surface_get_arrays(0);
	if (!arrays.size())
		return false;

	PoolVector<Vector3> verts = arrays[VS::ARRAY_VERTEX];
	if (!verts.size())
		return false;
	PoolVector<Vector3> norms = arrays[VS::ARRAY_NORMAL];
	if (!norms.size())
		return false;


	PoolVector<Vector2> uvs = arrays[VS::ARRAY_TEX_UV2];
	if (!uvs.size())
	{
		uvs = arrays[VS::ARRAY_TEX_UV];
	}

	if (!uvs.size())
		return false;

	PoolVector<int> inds = arrays[VS::ARRAY_INDEX];
	if (!inds.size())
		return false;

	// we need to get the vert positions and normals from local space to world space to match up with the
	// world space coords in the merged mesh
	Transform trans = mi.get_global_transform();

	PoolVector<Vector3> positions_world;
	PoolVector<Vector3> normals_world;
	Transform_Verts(verts, positions_world, trans);
	Transform_Norms(norms, normals_world, trans);

	// convert to longhand non indexed versions
	int nTris = inds.size() / 3;
	int nOldTris = m_Tris.size();
	int nNewTris = nOldTris + nTris;

	m_Tris.resize(nNewTris);
	m_TriNormals.resize(nNewTris);
	m_Tris_EdgeForm.resize(nNewTris);
	m_TriPlanes.resize(nNewTris);

	m_UVTris.resize(nNewTris);
	m_TriUVaabbs.resize(nNewTris);
	m_TriPos_aabbs.resize(nNewTris);
	m_Tri_TexelSizeWorldSpace.resize(nNewTris);

	m_Tri_MeshIDs.resize(nNewTris);

	int i = 0;
	for (int n=0; n<nTris; n++)
	{
		// adjusted n
		int an = n + nOldTris;

		Tri &t = m_Tris[an];
		Tri &tri_norm = m_TriNormals[an];
		Tri &tri_edge = m_Tris_EdgeForm[an];
		Plane &tri_plane = m_TriPlanes[an];
		UVTri &uvt = m_UVTris[an];
		Rect2 &rect = m_TriUVaabbs[an];
		AABB &aabb = m_TriPos_aabbs[an];
		m_Tri_MeshIDs[an] = mesh_id;

		int ind = inds[i];
		rect = Rect2(uvs[ind], Vector2(0, 0));
		aabb.position = positions_world[ind];
		aabb.size = Vector3(0, 0, 0);

		for (int c=0; c<3; c++)
		{
			ind = inds[i++];

			t.pos[c] = positions_world[ind];
			tri_norm.pos[c] = normals_world[ind];
			uvt.uv[c] = uvs[ind];
			//rect = Rect2(uvt.uv[0], Vector2(0, 0));
			rect.expand_to(uvt.uv[c]);
			//aabb.position = t.pos[0];
			aabb.expand_to(t.pos[c]);
		}

		// plane - calculate normal BEFORE changing winding into UV space
		// because the normal is determined by the winding in world space
		tri_plane = Plane(t.pos[0], t.pos[1], t.pos[2], CLOCKWISE);


		// calculate edge form
		{
			// b - a
			tri_edge.pos[0] = t.pos[1] - t.pos[0];
			// c - a
			tri_edge.pos[1] = t.pos[2] - t.pos[0];
			// a
			tri_edge.pos[2] = t.pos[0];
		}


		// ALWAYS DO THE UV WINDING LAST!!
		// make sure winding is standard in UV space
		if (uvt.IsWindingCW())
		{
			uvt.FlipWinding();
			t.FlipWinding();
			tri_norm.FlipWinding();
		}


#ifdef LLIGHTSCENE_VERBOSE
		String sz;
		sz = "found triangle : ";
		for (int s=0; s<3; s++)
		{
			sz += "(" + String(t.pos[s]) + ") ... ";
		}
		print_line(sz);
		sz = "\tnormal : ";
		for (int s=0; s<3; s++)
		{
			sz += "(" + String(tri_norm.pos[s]) + ") ... ";
		}
		print_line(sz);

		sz = "\t\tUV : ";
		for (int s=0; s<3; s++)
		{
			sz += "(" + String(uvt.uv[s]) + ") ... ";
		}
		print_line(sz);

#endif


		// convert aabb from 0-1 to texels
		//		aabb.position.x *= width;
		//		aabb.position.y *= height;
		//		aabb.size.x *= width;
		//		aabb.size.y *= height;

		// expand aabb just a tad
		rect.expand(Vector2(0.01, 0.01));

		CalculateTriTexelSize(an, width, height);
	}


	return true;
}

bool LightScene::Create(Spatial * pMeshesRoot, int width, int height, int voxel_density)
{
	m_bUseSIMD = true;

	FindMeshes(pMeshesRoot);
	if (!m_Meshes.size())
		return false;

	for (int n=0; n<m_Meshes.size(); n++)
	{
		if (!Create_FromMesh(n, width, height))
			return false;
	}

	m_Tracer.Create(*this, voxel_density);

	return true;
}

// note this is assuming 1:1 aspect ratio lightmaps. This could do x and y size separately,
// but more complex.
void LightScene::CalculateTriTexelSize(int tri_id, int width, int height)
{
	const Tri &tri = m_Tris[tri_id];
	const UVTri &uvtri = m_UVTris[tri_id];

	// length of edges in world space
	float l0 = (tri.pos[1] - tri.pos[0]).length();
	float l1 = (tri.pos[2] - tri.pos[0]).length();

	// texel edge lengths
	Vector2 te0 = uvtri.uv[1] - uvtri.uv[0];
	Vector2 te1 = uvtri.uv[2] - uvtri.uv[0];

	// convert texel edges from uvs to texels
	te0.x *= width;
	te0.y *= height;
	te1.x *= width;
	te1.y *= height;

	// texel edge lengths
	float tl0 = te0.length();
	float tl1 = te1.length();

	// default
	float texel_size = 1.0f;

	if (tl0 >= tl1)
	{
		// check for divide by zero
		if (tl0 > 0.00001f)
			texel_size = l0 / tl0;
	}
	else
	{
		if (tl1 > 0.00001f)
			texel_size = l1 / tl1;
	}

	m_Tri_TexelSizeWorldSpace[tri_id] = texel_size;
}

void LightScene::RasterizeTriangleIDs(LightMapper_Base &base, LightImage<uint32_t> &im_p1, LightImage<uint32_t> &im2_p1, LightImage<Vector3> &im_bary)
{
	int width = im_p1.GetWidth();
	int height = im_p1.GetHeight();

	// create a temporary image of vectors to store the triangles per texel
	LightImage<Vector<uint32_t> > temp_image_tris;
	temp_image_tris.Create(width, height, false);

//	LightImage<Vector<uint32_t> > temp_image_cutting_tris;
//	temp_image_cutting_tris.Create(width, height, false);

	for (int n=0; n<m_UVTris.size(); n++)
	{
		const Rect2 &aabb = m_TriUVaabbs[n];
		const UVTri &tri = m_UVTris[n];

		int min_x = aabb.position.x * width;
		int min_y = aabb.position.y * height;
		int max_x = (aabb.position.x + aabb.size.x) * width;
		int max_y = (aabb.position.y + aabb.size.y) * height;

		// add a bit for luck
		min_x--; min_y--; max_x++; max_y++;

		// clamp
		min_x = CLAMP(min_x, 0, width);
		min_y = CLAMP(min_y, 0, height);
		max_x = CLAMP(max_x, 0, width);
		max_y = CLAMP(max_y, 0, height);

		int debug_overlap_count = 0;

		for (int y=min_y; y<max_y; y++)
		{
			for (int x=min_x; x<max_x; x++)
			{
				float s = (x + 0.5f) / (float) width;
				float t = (y + 0.5f) / (float) height;

//				if ((x == 26) && (y == 25))
//				{
//					print_line("testing");
//				}

				if (tri.ContainsPoint(Vector2(s, t)))
				//if (tri.ContainsTexel(x, y, width , height))
				{
					temp_image_tris.GetItem(x, y).push_back(n);

					uint32_t &id_p1 = im_p1.GetItem(x, y);

					// hopefully this was 0 before
					if (id_p1)
					{
						debug_overlap_count++;
//						if (debug_overlap_count == 64)
//						{
//							print_line("overlap detected");
//						}

						// store the overlapped ID in a second map
						im2_p1.GetItem(x, y) = id_p1;
					}

					// save new id
					id_p1 = n+1;

					// find barycentric coords
					float u,v,w;
					tri.FindBarycentricCoords(Vector2(s, t), u, v, w);
					Vector3 &bary = im_bary.GetItem(x, y);
					bary =Vector3(u,v,w);
				}

			} // for x
		} // for y
	} // for tri


	// translate temporary image vectors into mini lists
	for (int y=0; y<height; y++)
	{
		for (int x=0; x<width; x++)
		{
				MiniList &ml = base.m_Image_TriIDs.GetItem(x, y);
				ml.first = base.m_TriIDs.size();

				const Vector<uint32_t> &vec = temp_image_tris.GetItem(x, y);

				for (int n=0; n<vec.size(); n++)
				{
					base.m_TriIDs.push_back(vec[n]);
					ml.num += 1;
				}

//				if (!ml.num)
//				{
//					ml.first = base.m_TriIDs.size();
//				}
//				BUG IS THESE ARE NOT CONTIGUOUS
//				ml.num += 1;
//				base.m_TriIDs.push_back(n);
		} // for x
	} // for y


	//FindCuts(base);
}


void LightScene::FindCuts_Texel(LightMapper_Base &base, int tx, int ty, int tri_id, const Vector3 &bary)
{
//	if ((tx == 31) && (ty == 44))
//	{
//		print_line("test");
//	}


	// pos and normal
	Vector3 pos;
	const Tri &tri_pos = m_Tris[tri_id];
	tri_pos.InterpolateBarycentric(pos, bary);

	// use facenormal, NOT interpolated normal.
	Vector3 norm = m_TriPlanes[tri_id].normal;

//	Vector3 norm2;
//	m_TriNormals[tri_id].InterpolateBarycentric(norm2, bary);
//	norm2.normalize();

	// push the pos out a little to prevent self intersection
	Vector3 push = norm * 0.005f;
	pos += push;

	// tangent
	Vector3 edge = tri_pos.pos[1] - tri_pos.pos[0];
	float edge_length = edge.length();
	if (edge_length < 0.0001f)
		return;

	// normalize
	edge *= 1.0f / edge_length;

	Vector3 tangent = edge.cross(norm);

	// use the texel_size to determine the max distance to trace
	float texel_size = m_Tri_TexelSizeWorldSpace[tri_id];
	texel_size *= base.m_Settings_AO_CutRange;

	// now we want to do tangent traces out from the pos, in order to find cutting triangles
	Ray r;
	r.o = pos;
	r.d = edge;
	if (FindCuts_TangentTrace(base, tx, ty, r, texel_size)) return;
	r.d = -edge;
	if (FindCuts_TangentTrace(base, tx, ty, r, texel_size)) return;
	r.d = tangent;
	if (FindCuts_TangentTrace(base, tx, ty, r, texel_size)) return;
	r.d = -tangent;
	if (FindCuts_TangentTrace(base, tx, ty, r, texel_size)) return;

	// try the edge and tangent at 45 degrees
	Vector3 edge45 = (edge + tangent).normalized();
	Vector3 tangent45 = (-edge + tangent).normalized();
	edge = edge45;
	tangent = tangent45;
	r.d = edge;
	if (FindCuts_TangentTrace(base, tx, ty, r, texel_size)) return;
	r.d = -edge;
	if (FindCuts_TangentTrace(base, tx, ty, r, texel_size)) return;
	r.d = tangent;
	if (FindCuts_TangentTrace(base, tx, ty, r, texel_size)) return;
	r.d = -tangent;
	if (FindCuts_TangentTrace(base, tx, ty, r, texel_size)) return;


	// debug
#ifdef DEBUG_ENABLED
//	MiniList_Cuts &ml = base.m_Image_Cuts.GetItem(tx, ty);
//	if (ml.num)
//	{
//		String sz;
//		sz = itos (tx) + ", " + itos(ty) + " numcuts " +itos(ml.num);
//		for (int c=0; c<ml.num; c++)
//		{
//			int cuttri_id = base.m_CuttingTris[ml.first + c];
//			sz += "\n\t" + String(Variant(m_TriPlanes[cuttri_id].normal));
//		}
//		print_line( sz);
//	}
#endif
}

// returns true if found cut
bool LightScene::FindCuts_TangentTrace(LightMapper_Base &base, int tx, int ty, Ray r, float max_dist)
{
	// backup the ray just a smidgen to allow for floating point error at the centre
	r.o -= (r.d * 0.0001f);

	// relies on the tracing all being setup, so findcuts should be run AFTER setting everything else up.
	float u, v, w, t;
	int num_tests = 0;
	int tri_id = FindIntersect_Ray(r, u, v, w, t, 0, num_tests);

	if (tri_id == -1)
		return false;

	// hit a tri! add to the cuts if within range.
	// just use a fixed range to start
	if (t > max_dist) // 0.05f
		return false;

	// compare dot products - this may not be necessary
	const Plane &cutting_plane = m_TriPlanes[tri_id];
	float dot = r.d.dot(cutting_plane.normal);

	// ignore if triangle is parallel
	if (fabsf(dot) < 0.001f)
		return false;

	base.m_Image_Cuts.GetItem(tx, ty) = 1;
	return true;
}
/*

	MiniList_Cuts &ml = base.m_Image_Cuts.GetItem(tx, ty);
	if (!ml.num)
	{
		ml.first = base.m_CuttingTris.size();
	}
	else
	{
		// is this triangle already in the list?
		for (uint32_t n=0; n<ml.num; n++)
		{
			int existing_tri_id = base.m_CuttingTris[ml.first + n];

			if (existing_tri_id == tri_id)
				return;

			// is this triangle parallel to existing triangle in list?
			float dot = m_TriPlanes[existing_tri_id].normal.dot(cutting_plane.normal);
			if (dot >= 0.99f)
			{
				//print_line("ignoring parallel cutting plane " + itos (tx) + ", " + itos(ty));
				// parallel, don't add 2nd
				return;
			}

			// opposite, cancel out, delete the existing triangle
			if (dot <= -0.99f)
			{
				//print_line("removing cancelled opposite cutting planes " + itos (tx) + ", " + itos(ty));

				// last in list?
				if (n == ml.num-1)
				{
					ml.num -= 1;
					return;
				}

				// not the last .. more complicated
				base.m_CuttingTris[ml.first + n] = base.m_CuttingTris[ml.first + ml.num-1];
				base.m_CuttingTris.remove_last();
				ml.num -= 1;
				return;
			}
		}
	}

	// not in the list .. add
	ml.num += 1;
	base.m_CuttingTris.push_back(tri_id);


	// when there is more than 1 cut, we need to know whether the triangle cuts
	// form a concave, or a convex profile.
	if (ml.num == 1)
	{
		ml.convex = 0;
	}
	else
	{
		// only take into account 2 cuts, above that, all bets are off
		uint32_t tri0_id = base.m_CuttingTris[ml.first];
		uint32_t tri1_id = base.m_CuttingTris[ml.first+1];

		Tri tri1 = m_Tris[tri1_id];

		// find centroid
		Vector3 ptCentre = (tri1.pos[0] + tri1.pos[1] + tri1.pos[2]) / 3;

		// is the centre behind, or in front of the first plane?
		const Plane &plane0 = m_TriPlanes[tri0_id];
		float dist = plane0.distance_to(ptCentre);

		if (dist >= 0.0f)
			ml.convex = 0;
		else
			ml.convex = 1;
	}


	// make texel dilatable
	//base.m_Image_ID_p1.GetItem(tx, ty) = 0;
}
*/

void LightScene::FindCuts(LightMapper_Base &base)
{
	// go through for each texel, and find cutting tris
	int width = base.m_iWidth;
	int height = base.m_iHeight;

	if (base.bake_begin_function) {
		base.bake_begin_function(height);
	}


	for (int y=0; y<height; y++)
	{

		if ((y % 10) == 0)
		{
			if (base.bake_step_function) {
				base.m_bCancel = base.bake_step_function(y, String("FindCuts: ") + " (" + itos(y) + ")");
				if (base.m_bCancel)
				{
					base.bake_end_function();
					return;
				}
			}
		}

		for (int x=0; x<width; x++)
		{
//			if ((x == 442) && (y == 529))
//			{
//				print_line("test");
//			}

			// just using the default uvtri, and barycentrics.
			int tri_id = base.m_Image_ID_p1.GetItem(x, y);
			if (tri_id)
			{
				// plus one based
				tri_id--;
				const Vector3 &bary = base.m_Image_Barycentric.GetItem(x, y);

				FindCuts_Texel(base, x, y, tri_id, bary);
			}
		}
	}

	if (base.bake_end_function) {
		base.bake_end_function();
	}

}


/*
int LightScene::FindTriAtUV(float x, float y, float &u, float &v, float &w) const
{
	for (int n=0; n<m_UVTris.size(); n++)
	{
		// check aabb
		const Rect2 &aabb = m_TriUVaabbs[n];
		if (!aabb.has_point(Point2(x, y)))
			continue;

		const UVTri &tri = m_UVTris[n];

		if (tri.ContainsPoint(Vector2(x, y)))
		{
			// find barycentric coords
			tri.FindBarycentricCoords(Vector2(x, y), u, v, w);

			return n+1; // plus 1 based
		}
	}

	return 0;
}
*/

void LightScene::Transform_Verts(const PoolVector<Vector3> &ptsLocal, PoolVector<Vector3> &ptsWorld, const Transform &tr) const
{
	for (int n=0; n<ptsLocal.size(); n++)
	{
		Vector3 ptWorld = tr.xform(ptsLocal[n]);
		ptsWorld.push_back(ptWorld);
	}
}

void LightScene::Transform_Norms(const PoolVector<Vector3> &normsLocal, PoolVector<Vector3> &normsWorld, const Transform &tr) const
{
	for (int n=0; n<normsLocal.size(); n++)
	{
		// hacky way for normals, we should use transpose of inverse matrix, dunno if godot supports this
		Vector3 ptNormA = Vector3(0, 0, 0);
		Vector3 ptNormWorldA = tr.xform(ptNormA);
		Vector3 ptNormWorldB = tr.xform(normsLocal[n]);

		Vector3 ptNorm = ptNormWorldB - ptNormWorldA;

		ptNorm = ptNorm.normalized();

		normsWorld.push_back(ptNorm);
	}
}
