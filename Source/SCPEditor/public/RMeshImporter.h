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

	bool Import(FArchive& Reader, TArray<FData>& Out) override;
};
