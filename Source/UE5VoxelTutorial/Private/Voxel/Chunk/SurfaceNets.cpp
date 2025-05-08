// Surface Nets - Steven Christy
// Solution derived from https://github.com/mikolalysenko/surface-nets
// Created for the UE5VoxelTutorial project
#include "SurfaceNets.h"
#include "Voxel/Utils/FastNoiseLite.h"
#include <bit>

ASurfaceNets::ASurfaceNets()
{
}

void ASurfaceNets::Setup()
{
	int PaddedSize = Size+2; // Surface Nets requires a bit more oversampling.
	// AxisOffsets defines the length of each axis in elements such that the dot product
	// between it and a FIntVector3 containing the position will give us the Index where
	// that Voxel data is located in the Voxel array.
	AxisOffsets = FIntVector(1, PaddedSize, PaddedSize*PaddedSize);
	Voxels.SetNum(AxisOffsets.Y * AxisOffsets.Z);
}

void ASurfaceNets::Generate2DHeightMap(FVector WorldOffset)
{
	FIntVector3 Position;
	for (Position.Y = 0; Position.Y < AxisOffsets.Y; ++Position.Y)
	{
		for (Position.X = 0; Position.X < AxisOffsets.Y; ++Position.X)
		{
			const float Xpos = Position.X + WorldOffset.X;
			const float ypos = Position.Y + WorldOffset.Y;
			
			const float Height = (Noise->GetNoise(Xpos, ypos) + 1.1f) * (Size / 2);

			for (Position.Z = 0; Position.Z < AxisOffsets.Y; ++Position.Z)
				Voxels[GetVoxelIndex(Position)] = Position.Z - Height;
		}
	}
}

void ASurfaceNets::Generate3DHeightMap(FVector WorldOffset)
{
	FIntVector3 Position;
	for (Position.Z = 0; Position.Z < AxisOffsets.Y; ++Position.Z)
	{
		for (Position.Y = 0; Position.Y < AxisOffsets.Y; ++Position.Y)
		{
			for (Position.X = 0; Position.X < AxisOffsets.Y; ++Position.X)
			{
				Voxels[GetVoxelIndex(Position)] = Noise->GetNoise(WorldOffset.X + Position.X, WorldOffset.Y + Position.Y, WorldOffset.Z + Position.Z);	
			}
		}
	}
}

void ASurfaceNets::GenerateMesh()
{
	FIntVector3 Position;

	// Too bad we need this extra vertex table for computing surface nets.
	// The upside is our mesh will have fewer vertices than marching cubes and will be welded.
	TArray<int32> VertexTable;
	VertexTable.SetNum(Voxels.Num());
	
	VertexCount = 0;
	int MaxSize = AxisOffsets.Y-1; 
	int Index;

	// Precalculate these.
	int IndexOffsets[8];
	for (int Corner = 0; Corner < 8; ++Corner)
		IndexOffsets[Corner] = GetVoxelIndex(VertexOffset[Corner]);

	// March - The marching axis order and memory layout are important here. Change these things only if you know what you are doing.
	for (Position.Z = 0; Position.Z < MaxSize; ++Position.Z)
	{
		for (Position.Y = 0; Position.Y < MaxSize; ++Position.Y)
		{
			for (Position.X = 0, Index = GetVoxelIndex(Position); Position.X < MaxSize; ++Position.X, ++Index)
			{
				VertexTable[Index] = -1;
				float Cube[8];
				// Calculate axis crossings mask
				int Mask = 0;
				for (int Corner = 0; Corner < 8; ++Corner)
				{
					float Value = Cube[Corner] = Voxels[Index+IndexOffsets[Corner]];
					// Check if Sign bits are different indicating a surface is present
					Mask |= (std::bit_cast<uint32>(Value) & 0x80000000) >> (31-Corner);
				}

				if (Mask && Mask < 0xFF)
				{
					int Vertex1 = VertexCount;
					VertexTable[Index] = VertexCount;
					++VertexCount;

					FVector Vertex, Normal;
					ComputeSDFSurface(&Cube[0], Vertex, Normal);
					Vertex += FVector(Position.X, Position.Y, Position.Z);
					Vertex *= BlockSize;
							
					MeshData.Vertices.Add(Vertex);
					MeshData.Normals.Add(Normal);
					
					uint8_t Flags = AxisFlags[Mask & AxisMask];
					
					for (int Axis = 0; Axis < 3; ++Axis)
					{
						if (Flags & (1 << Axis))
						{
							if (Position[Axis]<1)
								continue;
						
							int AxisOrtho1 = (Axis+1) % 3;
							if (Position[AxisOrtho1]<1)
								continue;
						
							int AxisOrtho2 = (Axis+2) % 3;
							if (Position[AxisOrtho2]<1)
								continue;

							int Vertex2 = VertexTable[Index - AxisOffsets[AxisOrtho1]];
							int Vertex3 = VertexTable[Index - AxisOffsets[AxisOrtho1] - AxisOffsets[AxisOrtho2]];
							int Vertex4 = VertexTable[Index - AxisOffsets[AxisOrtho2]];
						
							if (Mask & 1)
								std::swap(Vertex2, Vertex4);

							AddTriangle(Vertex1, Vertex2, Vertex3);
							AddTriangle(Vertex1, Vertex3, Vertex4);
						}
					}
				}				
				
			}
		}
	}
}

int ASurfaceNets::GetVoxelIndex(FIntVector3 Position) const
{
	return Position.Z * AxisOffsets.Z + Position.Y * AxisOffsets.Y + Position.X;
}

void ASurfaceNets::ComputeSDFSurface(const float *Cube, FVector& OutPosition, FVector& OutNormal)
{
    // Compute surface position by averaging edge intersections
    FVector PositionSum(0.f);
    int32 IntersectionCount = 0;

	// To preserve the ordering of the customary edge table I add the special edges to the end.
	// This is why we will iterate in reverse here.
    for (int32 Edge = 15; Edge >= 0; --Edge)
    {
        int32 V0 = Edges[Edge][0];
        int32 V1 = Edges[Edge][1];
        float SDF0 = Cube[V0];
        float SDF1 = Cube[V1];

        if (SDF0 * SDF1 < 0.f)
        {
            float T = SDF0 / (SDF0 - SDF1);
            FVector Intersection = VertexFloats[V0] + T * (VertexFloats[V1] - VertexFloats[V0]);
            PositionSum += Intersection;
            IntersectionCount++;
        }
    	if (Edge == 12)    		
    	{
    		if (IntersectionCount >= 3) // Three of the special points are more than enough, two might even work
    			break;
    	}
    }

	OutPosition = PositionSum / static_cast<float>(IntersectionCount);

	// This method of calculating normals has the advantage of being fast and easy, even if the results don't look great.
	OutNormal = FVector(Cube[1]-Cube[0] + Cube[7]-Cube[6], Cube[2]-Cube[0] + Cube[7]-Cube[5], Cube[4]-Cube[0] + Cube[7]-Cube[3]);
	OutNormal.Normalize();
}

void ASurfaceNets::AddTriangle(int32 IndexA, int32 IndexB, int32 IndexC)
{
	MeshData.Triangles.Add(IndexA);
	MeshData.Triangles.Add(IndexB);
	MeshData.Triangles.Add(IndexC);
}

void ASurfaceNets::ModifyVoxelData(const FIntVector Position, const EVoxelTutorialBlock Block)
{
	// In the spirit of this being an example and not a complete engine I include this to show 
	// how to modify the SDF.
	// It breaks along chunk borders. To fix it the ModifyVoxelData function would need to be
	// applied to all affected chunks.
	// For Surface nets it might also work better not use integer coordinates.
	float Radius = 2;
	float Strength = Block == EVoxelTutorialBlock::Air ? 1.f : -1.f;
	FIntVector Start = Position;
	FIntVector Count;
	
	for (Count.Z = -Radius; Count.Z <= Radius; ++Count.Z)
		for (Count.Y = -Radius; Count.Y < Radius; ++Count.Y)
			for (Count.X = -Radius; Count.X <  Radius; ++Count.X)
			{
				FIntVector Coordinate = Count + Start;
				if (Coordinate.X >= 0 && Coordinate.X <= Size &&
					Coordinate.Y >= 0 && Coordinate.Y <= Size &&
					Coordinate.Z >= 0 && Coordinate.Z <= Size)
				{
					float Distance = FMath::Sqrt(static_cast<float>(Count.Z * Count.Z + Count.Y * Count.Y + Count.X * Count.X));
					Voxels[GetVoxelIndex(Coordinate)] += Strength * FMath::Max(Radius - Distance, 0.0f);   
				}
			}
}
