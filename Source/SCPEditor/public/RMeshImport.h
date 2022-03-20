#pragma once

#include "OriginalAssetImporter.h"

class FRMeshImporter : IMeshImporter
{
	FRMeshImporter()
	{
	}

public:
	static FRMeshImporter* Get()
	{
		static FRMeshImporter* Instance = new FRMeshImporter();
		return Instance;
	}

	static FString ReadString(FArchive& Archive);

	bool Import(FArchive& Reader, TArray<TPair<FRawMesh, FTextureData>>& Out) override;
};
