#pragma once

#include "RawMesh.h"
#include "SlateBasics.h"
#include "Factories/TextureFactory.h"
#include "Factories/WorldFactory.h"

class FOriginalAssetImporter
{
public:
	FOriginalAssetImporter() :
		TextureFactory(NewObject<UTextureFactory>()),
		WorldFactory(NewObject<UWorldFactory>())
	{
	};

	static FOriginalAssetImporter& Get()
	{
		static TUniquePtr<FOriginalAssetImporter> Instance(new FOriginalAssetImporter);
		return *Instance;
	}

	struct FWrapFileOptions
	{
		FString Title;
		FString FileTypes;
		bool MultipleFiles = false;
	};

	FReply WrapFileDialog(
		FWrapFileOptions Options,
		void (FOriginalAssetImporter::*Target)(const FString&) const
	) const;

	void ImportRMeshInteractive(const FString& Path) const;

	TSubclassOf<AActor> ImportRMesh(const FString& AssetFullPath, const FString& TargetAssetPath, FString& ErrCode) const;

	void ImportLevelInteractive(const FString& Path) const;

	bool ImportLevel(const FString& AssetFullPath, const FString& TargetAssetPath, FString& ErrCode) const;

private:
	UTexture2D* GetOrImportTexture(const FString& ObjectPath,
	                               const FString& ImportPath = "",
	                               bool IsBump = false,
	                               FString* Err = nullptr) const;

	UStaticMesh* SaveRawMeshToAsset(FRawMesh& Mesh,
	                                const TArray<FStaticMaterial>& Materials,
	                                const FString& SavePath,
	                                FString* Err = nullptr) const;

	const TStrongObjectPtr<UTextureFactory> TextureFactory;
	const TStrongObjectPtr<UWorldFactory> WorldFactory;

	FString TmpDefaultError;
};

struct FTextureData
{
	FString Texture;
	FString Bump;
};

inline FString Hash(const FRawMesh& Mesh)
{
	// TODO Center pivot before hashing
	const uint64 HashNum = CityHash64(
		(const char*)Mesh.VertexPositions.GetData(),
		Mesh.VertexPositions.Num() * sizeof(FVector3f)
	);
	return BytesToHex((const uint8*)&HashNum, sizeof(HashNum) / sizeof(uint8));
}

class IMeshImporter
{
public:
	virtual ~IMeshImporter()
	{
	};
	virtual bool Import(FArchive& Reader, TArray<TPair<FRawMesh, FTextureData>>& Out) = 0;
};\
