// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "ChunkBase.h"
#include "Voxel/Utils/Enums.h"

#include "NaiveChunk.generated.h"

class FastNoiseLite;
class UProceduralMeshComponent;

UCLASS()
class ANaiveChunk final : public AChunkBase
{
	GENERATED_BODY()

protected:
	virtual void Setup() override;
	virtual void Generate2DHeightMap(FVector Position) override;
	virtual void Generate3DHeightMap(FVector Position) override;
	virtual void GenerateMesh() override;
	virtual void ModifyVoxelData(const FIntVector Position, const EVoxelTutorialBlock Block) override;

private:
	TArray<EVoxelTutorialBlock> Blocks;

	const FVector BlockVertexData[8] = {
		FVector(100,100,100),
		FVector(100,0,100),
		FVector(100,0,0),
		FVector(100,100,0),
		FVector(0,0,100),
		FVector(0,100,100),
		FVector(0,100,0),
		FVector(0,0,0)
	};

	const int BlockTriangleData[24] = {
		0,1,2,3, // Forward
		5,0,3,6, // Right
		4,5,6,7, // Back
		1,4,7,2, // Left
		5,4,1,0, // Up
		3,2,7,6  // Down
	};
	
	bool Check(FVector Position) const;
	void CreateFace(EVoxelTutorialDirection Direction, FVector Position);
	TArray<FVector> GetFaceVertices(EVoxelTutorialDirection Direction, FVector Position) const;
	FVector GetPositionInDirection(EVoxelTutorialDirection Direction, FVector Position) const;
	FVector GetNormal(EVoxelTutorialDirection Direction) const;
	int GetBlockIndex(int X, int Y, int Z) const;
};
