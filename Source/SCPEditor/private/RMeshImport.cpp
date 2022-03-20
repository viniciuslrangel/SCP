#include "RMeshImport.h"

#include "ArchiveHelper.h"

#include "Config.h"
#include "RawMesh.h"
#include "SCPEditor.h"

FString FRMeshImporter::ReadString(FArchive& Archive)
{
	if (Archive.GetArchiveState().IsError())
	{
		return "";
	}
	int32 Size = 0;
	Archive >> Size;
	if (Size <= 0)
	{
		return "";
	}
	TArray<ANSICHAR> Arr;
	Arr.SetNum(Size);
	Archive * Size >> Arr.GetData();
	return FString(Size, Arr.GetData());
}

bool FRMeshImporter::Import(FArchive& Reader, TArray<TPair<FRawMesh, FTextureData>>& Out)
{

	const FString OriginalGamePath = UConfig::Get().OriginalGamePath;

	bool bHasTriggerBox;

	const FString Type = ReadString(Reader);

	if (Type == "RoomMesh")
	{
		bHasTriggerBox = true;
	}
	else if (Type == "RoomMesh.HasTriggerBox")
	{
		bHasTriggerBox = false;
	}
	else
	{
		UE_LOG(LogSCPEditor, Warning, TEXT("File is not a RMesh"));
		check(false);
		return false;
	}

	struct FRMeshVertexData
	{
		float U1, V1;
		float U2, V2;

		uint8 R;
		uint8 G;
		uint8 B;
	};

	int32 MeshCount;
	if (!(Reader >> MeshCount))
	{
		check(false);
		return false;
	}

	for (int32 MeshI = 0; MeshI < MeshCount; MeshI++)
	{
		FTextureData TexData;

		// There is at most textures per Mesh, but the original
		// code does a lot of blending stuff, but it's all
		// light maps and white(empty) textures, so I'm ignoring
		// it here, just finding the first non-useless texture
		// * I try to find the Bump map anyway
		for (int32 j = 0; j <= 1; j++)
		{
			int8 TextureBit;
			if (!(Reader >> TextureBit))
			{
				check(false);
				return false;
			}
			if (!TextureBit) { continue; }

			FString TextureName = ReadString(Reader);
			if (Reader.GetArchiveState().IsError())
			{
				check(false);
				return false;
			}

			if (!TexData.Texture.IsEmpty())
			{
				continue;
			}

			if (TextureName.Contains("_lm"))
			{
				continue;
			}

			TexData.Texture = TextureName;

			FString BumpPath;
			const bool bFound = GConfig->GetString(
				*TextureName,
				TEXT("bump"),
				BumpPath,
				OriginalGamePath / TEXT("Data/materials.ini"));
			if (bFound)
			{
				BumpPath.ReplaceCharInline(TEXT('\\'), TEXT('/'));
				TexData.Bump = OriginalGamePath / BumpPath;
			}
		}

		FRawMesh Mesh;

		TArray<FRMeshVertexData> VertexData;

		int32 VertexCount;
		if (!(Reader >> VertexCount))
		{
			check(false);
			return false;
		}

		for (int32 VertexI = 0; VertexI < VertexCount; VertexI++)
		{
			FVector3f Vertex;
			bool Ok = true;
			Ok &= Ok && Reader >> Vertex.X;
			Ok &= Ok && Reader >> Vertex.Y;
			Ok &= Ok && Reader >> Vertex.Z;
			if (!Ok)
			{
				check(false);
				return false;
			}
			Mesh.VertexPositions.Add(FVector3f(Vertex.Z, Vertex.X, Vertex.Y));

			Ok = true;
			FRMeshVertexData Data;
			Ok &= Ok && Reader >> Data.U1;
			Ok &= Ok && Reader >> Data.V1;
			Ok &= Ok && Reader >> Data.U2;
			Ok &= Ok && Reader >> Data.V2;
			Ok &= Ok && Reader >> Data.R;
			Ok &= Ok && Reader >> Data.G;
			Ok &= Ok && Reader >> Data.B;
			if (!Ok)
			{
				check(false);
				return false;
			}
			VertexData.Add(Data);
		}

		int32 TriangleCount;
		if (!(Reader >> TriangleCount))
		{
			check(false);
			return false;
		}
		for (int32 TriangleI = 0; TriangleI < TriangleCount; TriangleI++)
		{
			int32 Index[3];
			if (!(Reader * 12 >> Index))
			{
				check(false);
				return false;
			}

			for (int32 Comp = 0; Comp < 3; Comp++)
			{
				int32 VertexId = Index[2 - Comp]; // Reverse order (reverse normal direction)
				const auto& [U1, V1, U2, V2, R, G, B] = VertexData[VertexId];

				Mesh.WedgeIndices.Add(VertexId);

				Mesh.WedgeColors.Add(FColor(R, G, B));

				Mesh.WedgeTexCoords[0].Add(FVector2f(U1, V1));
				Mesh.WedgeTexCoords[1].Add(FVector2f(U2, V2));

				Mesh.WedgeTangentX.Add(FVector::ZeroVector);
				Mesh.WedgeTangentY.Add(FVector::ZeroVector);
				Mesh.WedgeTangentZ.Add(FVector::ZeroVector);
			}

			Mesh.FaceSmoothingMasks.Add(0);
			Mesh.FaceMaterialIndices.Add(0);
		}

		Out.Add({Mesh, TexData});
	}

	return true;
}
