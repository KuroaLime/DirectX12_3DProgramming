#include "stdafx.h"
#include "Collision.h"
#include "Object.h"


inline XMVECTOR Support_OOBB(const BoundingOrientedBox& box, const XMVECTOR& vDir)	{
	XMFLOAT3 f3Corners[8];
	box.GetCorners(f3Corners);

	float fMaxDot = -FLT_MAX;
	XMVECTOR vBestSupport = XMVectorZero();

	for (int i = 0; i < 8; ++i)
	{
		XMVECTOR vCurrent = XMLoadFloat3(&f3Corners[i]);
		float fDot = XMVectorGetX(XMVector3Dot(vCurrent, vDir));
		if (fDot > fMaxDot)
		{
			fMaxDot = fDot;
			vBestSupport = vCurrent;
		}
	}
	return vBestSupport;
}
std::vector<EPA_Triangle>::iterator CCollisionAlogrithm::EPA_GetClosestTriangle(std::vector<EPA_Triangle>& triangles)
{
	auto  closest = triangles.begin();
	float minDstSq = FLT_MAX;

	for (auto it = triangles.begin(); it != triangles.end(); ++it)
	{
		if (it->distSq < minDstSq)
		{
			minDstSq = it->distSq;
			closest = it;
		}
	}
	return closest;
}
CCollisionAlogrithm::CCollisionAlogrithm(float cellSize) : m_fCellSize(cellSize) {}

void CCollisionAlogrithm::ClearGrid() {
	for (auto& KeyValue : m_Grid)
		KeyValue.second.objects.clear();
}


void CCollisionAlogrithm::UpdateDynamicObjectInGrid(CGameObject* obj) {

	BoundingOrientedBox worldBigOOBB;
	XMMATRIX xmmtxWorld = XMLoadFloat4x4(&obj->m_xmf4x4World);
	obj->m_xmBigOOGG.Transform(worldBigOOBB, xmmtxWorld);

	BoundingBox newAABB = GetWorldAABBFromOOBB(worldBigOOBB);

	std::unordered_set<Cellkey, CellKeyHasher> newKeySet;
	int minX = static_cast<int>(floorf((newAABB.Center.x - newAABB.Extents.x) / m_fCellSize));
	int maxX = static_cast<int>(floorf((newAABB.Center.x + newAABB.Extents.x) / m_fCellSize));
	int minY = static_cast<int>(floorf((newAABB.Center.y - newAABB.Extents.y) / m_fCellSize));
	int maxY = static_cast<int>(floorf((newAABB.Center.y + newAABB.Extents.y) / m_fCellSize));
	int minZ = static_cast<int>(floorf((newAABB.Center.z - newAABB.Extents.z) / m_fCellSize));
	int maxZ = static_cast<int>(floorf((newAABB.Center.z + newAABB.Extents.z) / m_fCellSize));

	for (int x = minX; x <= maxX; ++x)
	{
		for (int y = minY; y <= maxY; ++y)
		{
			for (int z = minZ; z <= maxZ; ++z)
			{
				newKeySet.insert(Cellkey{ x, y, z });
			}
		}
	}

	std::vector<Cellkey>& oldKeys = obj->m_CurrentGridCells;
	std::unordered_set<Cellkey, CellKeyHasher> oldKeySet(oldKeys.begin(), oldKeys.end());

	for (const auto& oldKey : oldKeys)
	{
		if (newKeySet.find(oldKey) == newKeySet.end())
		{
			auto it = m_Grid.find(oldKey);
			if (it != m_Grid.end())
			{
				SpatialCell& cell = it->second;
				auto& vec = cell.objects;
				vec.erase(std::remove(vec.begin(), vec.end(), obj), vec.end());
			}
		}
	}
	for (const auto& newKey : newKeySet)
	{
		if (oldKeySet.find(newKey) == oldKeySet.end())
		{
			m_Grid[newKey].objects.push_back(obj);
		}
	}

	obj->m_CurrentGridCells.assign(newKeySet.begin(), newKeySet.end());
}

void CCollisionAlogrithm::CheckCollision() {


	std::unordered_set<GameObjectPair, PairHasher> checkedPairs;

	for (auto& kv : m_Grid)
	{

		SpatialCell& cell = kv.second;
		auto& objs = cell.objects;

		size_t n = objs.size();

		//char sz[256];
		//OutputDebugStringA(sz);
		if (n < 2) continue;



		for (size_t i = 0; i < n; ++i)
		{
			CGameObject* a = objs[i];
			if (!a || !a->IsCollidable()) continue;

			for (size_t j = i + 1; j < n; ++j)
			{
				CGameObject* b = objs[j];

				if (!b || !b->IsCollidable()) continue;
				GameObjectPair p(a, b);

				if (checkedPairs.insert(p).second)
				{
					if (!CanLayersCollide(a->GetLayer(), b->GetLayer())) continue;

					if (a->m_xmWorldSphere.Intersects(b->m_xmWorldSphere)) {

						
						XMVECTOR mtv;
						if (GetPreciseIntersectionInfo(a, b, mtv))
							if (a->bRender && b->bRender)
							{
								OnCollision(a, b, mtv);
							}
					}
				}
			}
		}
	}
}


Cellkey CCollisionAlogrithm::GetCellKey(const XMFLOAT3& pos) const {
	Cellkey key;
	key.x = static_cast<int>(floorf(pos.x / m_fCellSize));
	key.y = static_cast<int>(floorf(pos.y / m_fCellSize));
	key.z = static_cast<int>(floorf(pos.z / m_fCellSize));
	return key;
}

BoundingBox CCollisionAlogrithm::GetWorldAABBFromOOBB(const BoundingOrientedBox& oobb) const {
	BoundingBox aabb;
	XMFLOAT3 corners[8];
	oobb.GetCorners(corners);
	BoundingBox::CreateFromPoints(aabb,8,corners,sizeof(XMFLOAT3));
	return aabb;
}
void CCollisionAlogrithm::InsertObjectToGridByAABB(CGameObject* obj) {
	for (const auto& key : obj->m_CurrentGridCells)
	{
		auto it = m_Grid.find(key);
		if (it != m_Grid.end())
		{
			SpatialCell& cell = it->second;
			auto& vec = cell.objects;
			vec.erase(std::remove(vec.begin(), vec.end(), obj), vec.end());

			// if (vec.empty()) m_Grid.erase(it);
		}
	}

	obj->m_CurrentGridCells.clear(); //

	BoundingBox aabb = GetWorldAABBFromOOBB(obj->m_xmWorldOOBB);
	InsertObjectToGridByAABB_Internal(obj, aabb);
}
void CCollisionAlogrithm::InsertObjectToGridByAABB_Internal(CGameObject* obj,const BoundingBox& aabb) {
	int minX = static_cast<int>(floorf((aabb.Center.x - aabb.Extents.x) / m_fCellSize));
	int maxX = static_cast<int>(floorf((aabb.Center.x + aabb.Extents.x) / m_fCellSize));
	int minY = static_cast<int>(floorf((aabb.Center.y - aabb.Extents.y) / m_fCellSize));
	int maxY = static_cast<int>(floorf((aabb.Center.y + aabb.Extents.y) / m_fCellSize));
	int minZ = static_cast<int>(floorf((aabb.Center.z - aabb.Extents.z) / m_fCellSize));
	int maxZ = static_cast<int>(floorf((aabb.Center.z + aabb.Extents.z) / m_fCellSize));

	for (int x = minX; x <= maxX; ++x)
	{
		for (int y = minY; y <= maxY; ++y)
		{
			for (int z = minZ; z <= maxZ; ++z)
			{
				Cellkey key{ x, y, z };
				SpatialCell& cell = m_Grid[key];
				cell.objects.push_back(obj);
				obj->m_CurrentGridCells.push_back(key);
			}
		}
	}
}
void CCollisionAlogrithm::DebugPrintCellCount(CGameObject* obj, const char* name) {

}

void CCollisionAlogrithm::GetCollisionLeafNodes(CGameObject* obj, std::vector<CGameObject*>& leafNodes) {
	if (obj->IsCollisionLeaf())
	{
		leafNodes.push_back(obj);
	}

	if (obj->m_pChild)
	{
		GetCollisionLeafNodes(obj->m_pChild, leafNodes);
	}

	if (obj->m_pSibling)
	{
		GetCollisionLeafNodes(obj->m_pSibling, leafNodes);
	}
}
bool CCollisionAlogrithm::GetPreciseIntersectionInfo(CGameObject* objA, CGameObject* objB, XMVECTOR& mtv) {

	std::vector<CGameObject*> leavesA;
	GetCollisionLeafNodes(objA, leavesA);

	std::vector<CGameObject*> leavesB;
	GetCollisionLeafNodes(objB, leavesB);

	if (leavesA.empty() || leavesB.empty())
	{
		return false;
	}
	
	float fMinOverlapSq = FLT_MAX; 
	XMVECTOR vBestMtv = XMVectorZero();
	bool bFoundCollision = false;


	for (CGameObject* leafA : leavesA)
	{
		for (CGameObject* leafB : leavesB)
		{
			XMVECTOR vCurrentMtv = XMVectorZero();

			if (GJK_OOBB_Test(leafA->m_xmWorldOOBB, leafB->m_xmWorldOOBB, vCurrentMtv))
			{

				bFoundCollision = true;


				float fOverlapSq = XMVectorGetX(XMVector3LengthSq(vCurrentMtv));


				if (fOverlapSq < fMinOverlapSq)
				{
					fMinOverlapSq = fOverlapSq;
					vBestMtv = vCurrentMtv;
				}
			}
		}
	}
	if (bFoundCollision)
	{
		mtv = vBestMtv;

		XMFLOAT3 mtvF;
		XMStoreFloat3(&mtvF, mtv);
		//char sz[256];
		//sprintf_s(sz, "[GJK/EPA MTV] (%.3f, %.3f, %.3f)  len=%.5f\n",
		//	mtvF.x, mtvF.y, mtvF.z,
		//	sqrtf(mtvF.x * mtvF.x + mtvF.y * mtvF.y + mtvF.z * mtvF.z));
		//OutputDebugStringA(sz);
		return true;
	}
	return false;
}


/// /////////////////
XMVECTOR CCollisionAlogrithm::GJK_MinkowskiSupport(const DirectX::BoundingOrientedBox& boxA,const DirectX::BoundingOrientedBox& boxB,const XMVECTOR& vDir) {
	// (A - B) in Dir = A.Support(Dir) - B.Support(-Dir)
	XMVECTOR vSupportA = Support_OOBB(boxA, vDir);
	XMVECTOR vSupportB = Support_OOBB(boxB, XMVectorNegate(vDir));
	return XMVectorSubtract(vSupportA, vSupportB);
}

bool CCollisionAlogrithm::GJK_ProcessSimplex(GJK_Simplex& simplex, XMVECTOR& vDir) {
	XMVECTOR A = simplex[simplex.nSize - 1];
	XMVECTOR AO = XMVectorNegate(A); // A -> Origin

	if (simplex.nSize == 2)
	{
		XMVECTOR B = simplex[0];
		XMVECTOR AB = XMVectorSubtract(B, A);

		//    (Dot(AB, AO) <= 0)
		if (XMVectorGetX(XMVector3Dot(AB, AO)) <= 0.0f)
		{
			simplex.vPoints[0] = A;
			simplex.nSize = 1;
			vDir = AO;
			return false;
		}

		vDir = XMVector3Cross(XMVector3Cross(AB, AO), AB);

		return false;
	}
	else if (simplex.nSize == 3)
	{
		// B: simplex[1], C: simplex[0]
		XMVECTOR B = simplex[1];
		XMVECTOR C = simplex[0];
		XMVECTOR AB = XMVectorSubtract(B, A);
		XMVECTOR AC = XMVectorSubtract(C, A);

		XMVECTOR ABC_normal = XMVector3Cross(AB, AC);

		XMVECTOR vPerpAB = XMVector3Cross(AB, ABC_normal);
		XMVECTOR vPerpAC = XMVector3Cross(ABC_normal, AC);

		if (XMVectorGetX(XMVector3Dot(ABC_normal, AO)) > 0.0f)
		{
			vDir = ABC_normal;
			if (XMVectorGetX(XMVector3Dot(vDir, AO)) < 0.0f)
				vDir = XMVectorNegate(vDir);

			return false;
		}

		if (XMVectorGetX(XMVector3Dot(vPerpAB, AO)) > 0.0f)
		{
			simplex.vPoints[0] = B;
			simplex.vPoints[1] = A;
			simplex.nSize = 2;
			return GJK_ProcessSimplex(simplex, vDir);
		}

		if (XMVectorGetX(XMVector3Dot(vPerpAC, AO)) > 0.0f)
		{
			simplex.vPoints[0] = C;
			simplex.vPoints[1] = A;
			simplex.nSize = 2;
			return GJK_ProcessSimplex(simplex, vDir);
		}

		vDir = ABC_normal;
		if (XMVectorGetX(XMVector3Dot(vDir, AO)) < 0.0f)
			vDir = XMVectorNegate(vDir);


		return false;
	}
	else if (simplex.nSize == 4)
	{
		// A: simplex[3], B: simplex[2], C: simplex[1], D: simplex[0]
		XMVECTOR B = simplex[2];
		XMVECTOR C = simplex[1];
		XMVECTOR D = simplex[0];

		XMVECTOR ABC_normal = XMVector3Cross(XMVectorSubtract(B, A), XMVectorSubtract(C, A));
		if (XMVectorGetX(XMVector3Dot(ABC_normal, D)) > 0.0f) ABC_normal = XMVectorNegate(ABC_normal);

		XMVECTOR ACD_normal = XMVector3Cross(XMVectorSubtract(C, A), XMVectorSubtract(D, A));
		if (XMVectorGetX(XMVector3Dot(ACD_normal, B)) > 0.0f) ACD_normal = XMVectorNegate(ACD_normal);

		XMVECTOR ADB_normal = XMVector3Cross(XMVectorSubtract(D, A), XMVectorSubtract(B, A));
		if (XMVectorGetX(XMVector3Dot(ADB_normal, C)) > 0.0f) ADB_normal = XMVectorNegate(ADB_normal);

		XMVECTOR BCD_normal = XMVector3Cross(XMVectorSubtract(C, B), XMVectorSubtract(D, B));
		if (XMVectorGetX(XMVector3Dot(BCD_normal, A)) > 0.0f) BCD_normal = XMVectorNegate(BCD_normal);


		if (XMVectorGetX(XMVector3Dot(ABC_normal, AO)) > 0.0f)
		{
			simplex.vPoints[0] = C;
			simplex.vPoints[1] = B;
			simplex.vPoints[2] = A;
			simplex.nSize = 3;
			vDir = ABC_normal;
			return false;
		}
		else if (XMVectorGetX(XMVector3Dot(ACD_normal, AO)) > 0.0f)
		{
			simplex.vPoints[1] = C;
			simplex.vPoints[2] = A;
			simplex.nSize = 3;
			vDir = ACD_normal;
			return false;
		}
		else if (XMVectorGetX(XMVector3Dot(ADB_normal, AO)) > 0.0f)
		{
			simplex.vPoints[0] = D;
			simplex.vPoints[1] = B;
			simplex.vPoints[2] = A;
			simplex.nSize = 3;
			vDir = ADB_normal;
			return false;
		}

		return true;
	}

	if (simplex.nSize == 1)
	{
		vDir = AO;
		return false;
	}

	return false;
}

XMVECTOR CCollisionAlogrithm::EPA_GetMTV(GJK_Simplex& simplex,const DirectX::BoundingOrientedBox& boxA,const DirectX::BoundingOrientedBox& boxB) {
	std::vector<EPA_Triangle> triangles;

	EPA_Triangle t1 = { simplex[0], simplex[1], simplex[2] }; // D-C-B
	EPA_Triangle t2 = { simplex[0], simplex[2], simplex[3] }; // D-B-A
	EPA_Triangle t3 = { simplex[0], simplex[3], simplex[1] }; // D-A-C
	EPA_Triangle t4 = { simplex[1], simplex[3], simplex[2] }; // C-A-B

	t1.normal = XMVector3Cross(XMVectorSubtract(t1.b, t1.a), XMVectorSubtract(t1.c, t1.a));
	t2.normal = XMVector3Cross(XMVectorSubtract(t2.b, t2.a), XMVectorSubtract(t2.c, t2.a));
	t3.normal = XMVector3Cross(XMVectorSubtract(t3.b, t3.a), XMVectorSubtract(t3.c, t3.a));
	t4.normal = XMVector3Cross(XMVectorSubtract(t4.b, t4.a), XMVectorSubtract(t4.c, t4.a));
	t1.normal = (XMVectorGetX(XMVector3Dot(t1.normal, t1.a)) > 0.f) ? XMVectorNegate(t1.normal) : t1.normal;
	t2.normal = (XMVectorGetX(XMVector3Dot(t2.normal, t2.a)) > 0.f) ? XMVectorNegate(t2.normal) : t2.normal;
	t3.normal = (XMVectorGetX(XMVector3Dot(t3.normal, t3.a)) > 0.f) ? XMVectorNegate(t3.normal) : t3.normal;
	t4.normal = (XMVectorGetX(XMVector3Dot(t4.normal, t4.a)) > 0.f) ? XMVectorNegate(t4.normal) : t4.normal;

	t1.distSq = XMVectorGetX(XMVector3Dot(t1.normal, t1.a)); t1.distSq *= t1.distSq;
	t2.distSq = XMVectorGetX(XMVector3Dot(t2.normal, t2.a)); t2.distSq *= t2.distSq;
	t3.distSq = XMVectorGetX(XMVector3Dot(t3.normal, t3.a)); t3.distSq *= t3.distSq;
	t4.distSq = XMVectorGetX(XMVector3Dot(t4.normal, t4.a)); t4.distSq *= t4.distSq;

	triangles.push_back(t1); triangles.push_back(t2); triangles.push_back(t3); triangles.push_back(t4);

	for (int iter = 0; iter < 32; ++iter)
	{
		if (triangles.empty())
		{
			return XMVectorZero();
		}

		auto closestTriIt = EPA_GetClosestTriangle(triangles);
		EPA_Triangle closestTri = *closestTriIt;

		XMVECTOR vDir = closestTri.normal;
		XMVECTOR P = GJK_MinkowskiSupport(boxA, boxB, vDir);

		float distP = XMVectorGetX(XMVector3Dot(P, vDir));
		float distTri = XMVectorGetX(XMVector3Dot(closestTri.a, vDir));

		if (distP - distTri < 0.001f)
		{
			return vDir * distTri;
		}

		std::vector<EPA_Edge> edges;
		for (auto it = triangles.begin(); it != triangles.end();)
		{
			if (XMVectorGetX(XMVector3Dot(it->normal, XMVectorSubtract(P, it->a))) > 0.f)
			{
				edges.push_back({ it->a, it->b });
				edges.push_back({ it->b, it->c });
				edges.push_back({ it->c, it->a });
				it = triangles.erase(it);
			}
			else
			{
				++it;
			}
		}

		std::vector<EPA_Edge> uniqueEdges;
		for (auto e1 : edges)
		{
			bool bIsUnique = true;
			for (auto e2 : edges)
			{
				if (&e1 == &e2) continue;
				if (XMVector3Equal(e1.a, e2.b) && XMVector3Equal(e1.b, e2.a))
				{
					bIsUnique = false;
					break;
				}
			}
			if (bIsUnique) uniqueEdges.push_back(e1);
		}

		for (auto edge : uniqueEdges)
		{
			EPA_Triangle newTri = { P, edge.a, edge.b };
			newTri.normal = XMVector3Cross(XMVectorSubtract(newTri.b, newTri.a), XMVectorSubtract(newTri.c, newTri.a));
			if (XMVectorGetX(XMVector3Dot(newTri.normal, newTri.a)) > 0.f)
				newTri.normal = XMVectorNegate(newTri.normal);

			newTri.distSq = XMVectorGetX(XMVector3Dot(newTri.normal, newTri.a));
			newTri.distSq *= newTri.distSq;

			triangles.push_back(newTri);
		}
	}

	return XMVectorZero();
}

bool CCollisionAlogrithm::GJK_OOBB_Test(const DirectX::BoundingOrientedBox& boxA,const DirectX::BoundingOrientedBox& boxB,XMVECTOR& mtv) {
	GJK_Simplex simplex;
	XMVECTOR vDir = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);

	XMVECTOR P = GJK_MinkowskiSupport(boxA, boxB, vDir);
	simplex.Push(P);

	vDir = XMVectorNegate(P);

	for (int iter = 0; iter < 32; ++iter)
	{
		P = GJK_MinkowskiSupport(boxA, boxB, vDir);

		float fDot = XMVectorGetX(XMVector3Dot(P, vDir));
		if (fDot < 0.000001f)
		{
			mtv = XMVectorZero();
			return false;
		}

		simplex.Push(P);

		if (GJK_ProcessSimplex(simplex, vDir))
		{

			mtv = EPA_GetMTV(simplex, boxA, boxB);

			if (XMVector3Equal(mtv, XMVectorZero()))
			{
				XMVECTOR centerDiff = XMVectorSubtract(XMLoadFloat3(&boxA.Center), XMLoadFloat3(&boxB.Center));
				if (XMVectorGetX(XMVector3LengthSq(centerDiff)) < 0.00001f)
				{
					mtv = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f) * 0.1f;
				}
				else
				{
					mtv = XMVector3Normalize(centerDiff) * 0.1f;
				}
			}
			return true;
		}
	}

	mtv = XMVectorZero();
	return false;
}
