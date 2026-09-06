#pragma once
#include"stdafx.h"
#include"d3dx12.h"

#include <vector>
#include<unordered_map>
#include<unordered_set>
#include<functional>

class CGameObject;
enum class EObjectLayer;

struct Cellkey {
	int x, y, z;
	bool operator==(const Cellkey& other)const {
		return x == other.x && y == other.y && z == other.z;
	}
};

struct CellKeyHasher {
	size_t operator()(const Cellkey& key)const {
		size_t h1 = std::hash<int>()(key.x);
		size_t h2 = std::hash<int>()(key.y);
		size_t h3 = std::hash<int>()(key.z);
		return h1 ^ (h2 << 1) ^ (h3 << 2);
	}
};

struct SpatialCell {
	std::vector<CGameObject*> objects;
};

struct GameObjectPair {
	CGameObject* a;
	CGameObject* b;

	bool operator ==(const GameObjectPair& other) const {
		return a == other.a && b == other.b;
	}
	GameObjectPair(CGameObject* objA, CGameObject* objB) {
		a = std::min(objA, objB);
		b = std::max(objA, objB);
	}

};

struct PairHasher {
	size_t operator()(const GameObjectPair& p)const {
		auto hash1 = std::hash<CGameObject*>{}(p.a);
		auto hash2 = std::hash<CGameObject*>{}(p.b);
		return hash1 ^ (hash2 << 1);
	}
};

struct EPA_Edge { XMVECTOR a, b; };
struct EPA_Triangle { XMVECTOR a, b, c; XMVECTOR normal; float distSq; };
struct GJK_Simplex
{
	std::vector<XMVECTOR> vPoints;
	UINT nSize = 0;

	void Clear() { vPoints.clear(); nSize = 0; }
	void Push(const XMVECTOR& p) { vPoints.push_back(p); nSize = vPoints.size(); }
	XMVECTOR& operator[](UINT i) { return vPoints[i]; }
	const XMVECTOR& operator[](UINT i) const { return vPoints[i]; }
};

class CCollisionAlogrithm {
public:
	explicit CCollisionAlogrithm(float cellSize = 200.0f);

	void ClearGrid();
	void InsertObjectToGridByAABB(CGameObject* obj);
	void UpdateDynamicObjectInGrid(CGameObject* obj);

	void CheckCollision();

	void SetCellSize(float size) { m_fCellSize = size; }
	float GetCellsize() const { return m_fCellSize; }

	std::function<bool(EObjectLayer, EObjectLayer)> CanLayersCollide;
	std::function<void(CGameObject*, CGameObject*, const XMVECTOR& mtv)> OnCollision;

public:
	void GetCollisionLeafNodes(CGameObject* obj, std::vector<CGameObject*>& leafNodes);
private:
	float m_fCellSize;
	std::unordered_map<Cellkey, SpatialCell, CellKeyHasher> m_Grid;

	Cellkey GetCellKey(const XMFLOAT3& pos) const;
	DirectX::BoundingBox GetWorldAABBFromOOBB(const DirectX::BoundingOrientedBox& oobb) const;

	void InsertObjectToGridByAABB_Internal(CGameObject* obj,
		const DirectX::BoundingBox& aabb);
	void DebugPrintCellCount(CGameObject* obj, const char* name);

	bool GetPreciseIntersectionInfo(CGameObject* objA, CGameObject* objB, XMVECTOR& mtv);

	XMVECTOR GJK_MinkowskiSupport(const DirectX::BoundingOrientedBox& boxA,
		const DirectX::BoundingOrientedBox& boxB,
		const XMVECTOR& vDir);

	bool GJK_ProcessSimplex(GJK_Simplex& simplex, XMVECTOR& vDir);

	XMVECTOR EPA_GetMTV(GJK_Simplex& simplex,
		const DirectX::BoundingOrientedBox& boxA,
		const DirectX::BoundingOrientedBox& boxB);

	bool GJK_OOBB_Test(const DirectX::BoundingOrientedBox& boxA,
		const DirectX::BoundingOrientedBox& boxB,
		XMVECTOR& mtv);
	static std::vector<EPA_Triangle>::iterator EPA_GetClosestTriangle(std::vector<EPA_Triangle>& triangles);
};

class CSpatialGrid {
public:
	CSpatialGrid(float cellSize = 200.0f) : m_fCellSize(cellSize) {}
	void Clear();
	void InsertObjectByAABB(CGameObject* obj, const BoundingBox& aabb);
	void UpdateDynamicObject(CGameObject* obj, const BoundingBox& newAABB);

	template <typename Fn>
	void ForEachCell(Fn fn) {
		for (auto& kv : m_Grid) {
			fn(kv.first, kv.second);
		}
	}

	const std::unordered_map<Cellkey, SpatialCell, CellKeyHasher>& GetGrid() const {
		return m_Grid;
	}
	float GetCellSize() const { return m_fCellSize; }
private:
	float m_fCellSize;
	std::unordered_map<Cellkey, SpatialCell, CellKeyHasher> m_Grid;

	Cellkey GetCellkey(const XMFLOAT3& pos) const;
};
