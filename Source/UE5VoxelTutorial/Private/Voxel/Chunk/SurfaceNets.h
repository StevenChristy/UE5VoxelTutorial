// Surface Nets - Steven Christy
// Solution derived from https://github.com/mikolalysenko/surface-nets
// Created for the UE5VoxelTutorial project
// Note: Surface nets can produce non-manifold edges where exactly 4 triangles share an edge.
// While they don't look very good it is possible to collapse them if needed. 
#pragma once
#include "CoreMinimal.h"
#include "ChunkBase.h"
#include "SurfaceNets.generated.h"


UCLASS()
class UE5VOXELTUTORIAL_API ASurfaceNets : public AChunkBase
{
	GENERATED_BODY()

public:
	FIntVector3 AxisOffsets = FIntVector3(0,0,0);

	// 5 bits of the 8 bit corner mask are enough to determine which axis to create quads for.
	static inline constexpr uint8_t AxisMask = 0x1F;
	static inline constexpr uint8_t AxisFlags[32] =
	{
		0,7,1,6,2,5,3,4,0,7,1,6,2,5,3,4,4,3,5,2,6,1,7,0,4,3,5,2,6,1,7,0
	};

	// We use these calculate index offsets within the voxel buffer for the cube corners
	static inline const FIntVector3 VertexOffset[8] =
	{
		{0,0,0},{1,0,0},
		{0,1,0},{1,1,0},
		{0,0,1},{1,0,1},
		{0,1,1},{1,1,1}
	};

	// Same as above, but for calculating suface positions within the cube and avoiding conversion.
	// We will lerp between these points based on the values in the SDF
	static inline const FVector VertexFloats[8] = {
		FVector(0.f, 0.f, 0.f),
		FVector(1.f, 0.f, 0.f),
		FVector(0.f, 1.f, 0.f),
		FVector(1.f, 1.f, 0.f),
		FVector(0.f, 0.f, 1.f),
		FVector(1.f, 0.f, 1.f),
		FVector(0.f, 1.f, 1.f),
		FVector(1.f, 1.f, 1.f) 
	};
	
	// 16 Edges:
	// Here I have added 4 special "edges" or rather lines passing between opposing corners of the cube.
	// In many cases, these will provide a much better surface position
	static inline constexpr uint8_t Edges[16][2] = {
		{0,1},{1,3},{3,2},{2,0},
		{4,5},{5,7},{7,6},{6,4},
		{0,4},{1,5},{3,7},{2,6},
		{0,7},{1,6},{2,5},{3,4}
	};
	
	TArray<float> Voxels;
	
	ASurfaceNets();

	virtual void Setup() override;
	virtual void Generate2DHeightMap(FVector WorldOffset) override;
	virtual void Generate3DHeightMap(FVector WorldOffset) override;

	virtual void GenerateMesh() override;

	FORCEINLINE int GetVoxelIndex(FIntVector3 Position) const;

	FORCEINLINE void ComputeSDFSurface(const float* Cube, FVector& OutPosition, FVector& OutNormal);
	FORCEINLINE void AddTriangle(int32 IndexA, int32 IndexB, int32 IndexC);

protected:
	virtual void ModifyVoxelData(const FIntVector Position, const EVoxelTutorialBlock Block) override;
};
