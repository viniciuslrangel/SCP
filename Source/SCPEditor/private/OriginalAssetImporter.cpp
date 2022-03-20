#include "OriginalAssetImporter.h"

#include "DesktopPlatformModule.h"
#include "SCPEditorModule.h"

#include "ArchiveHelper.h"
#include "BlueprintCompilationManager.h"
#include "ComponentAssetBroker.h"
#include "Config.h"
#include "IContentBrowserSingleton.h"
#include "ImageUtils.h"
#include "KismetCompilerModule.h"
#include "MeshDescription.h"
#include "PackageTools.h"
#include "RawMesh.h"
#include "RMeshImport.h"
#include "SCPEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "Dialogs/DlgPickPath.h"
#include "Dialogs/SOutputLogDialog.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/TextureFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialExpressionBumpOffset.h"
#include "Misc/RemoteConfigIni.h"
#include "UObject/GCObjectScopeGuard.h"
#include "UObject/SavePackage.h"

FReply FOriginalAssetImporter::WrapFileDialog(
	FWrapFileOptions Options,
	void (FOriginalAssetImporter::*Target)(const FString&) const
) const
{
	static FSCPEditorModule& Module = FModuleManager::LoadModuleChecked<FSCPEditorModule>(TEXT("SCPEditor"));

	TArray<FString> OpenFilenames;

	const void* ParentWindow = nullptr;
	if (const TSharedPtr<SWindow> Root = FGlobalTabmanager::Get()->GetRootWindow())
	{
		ParentWindow = Root->GetNativeWindow()->GetOSWindowHandle();
	}

	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	const bool bOpened = DesktopPlatform->OpenFileDialog(
		ParentWindow,
		Options.Title,
		UConfig::Get().OriginalGamePath,
		TEXT(""),
		Options.FileTypes,
		Options.MultipleFiles ? EFileDialogFlags::Multiple : EFileDialogFlags::None,
		OpenFilenames
	);

	if (!bOpened)
	{
		return FReply::Handled();
	}

	const FString Filename = OpenFilenames[0];

	(this->*Target)(Filename);

	return FReply::Handled();
}

void FOriginalAssetImporter::ImportRMeshInteractive(const FString& Path) const
{
	const TSharedPtr<SDlgPickPath> PickAssetPathWidget = SNew(SDlgPickPath)
		.Title(FText::FromString(TEXT("Choose save assets location Location")));
	if (PickAssetPathWidget->ShowModal() != EAppReturnType::Ok)
	{
		return;
	}

	const FString TargetAssetPath = PickAssetPathWidget->GetPath().ToString();

	if (FString Err; !ImportRMesh(Path, TargetAssetPath, Err))
	{
		const FText Msg = FText::FromString(Err);
		UE_LOG(LogSCPEditor, Error, TEXT("Import RMesh error %s"), *Err);
		SOutputLogDialog::Open(
			FText::FromString(TEXT("Import error")),
			FText::FromString(TEXT("Could not import rmesh file")),
			Msg
		);
	}
}

struct FTextureDataEx
{
	UTexture2D* Tex = nullptr;
	UTexture2D* Bump = nullptr;

	operator bool() const { return Tex != nullptr; }
};

struct FRoomTemplate
{
	int8 XPos;
	int8 YPos;
	FString Name;
	FString EventName;
	float EventProb;

	uint8 Angle;
};


TSubclassOf<AActor> FOriginalAssetImporter::ImportRMesh(const FString& AssetFullPath,
                                                        const FString& TargetAssetPath,
                                                        FString& ErrCode) const
{
	IMeshImporter* Importer;
	FString Ext = FPaths::GetExtension(AssetFullPath);

	if (Ext == "rmesh")
	{
		Importer = (IMeshImporter*)FRMeshImporter::Get();
	}
	else
	{
		ErrCode = "invalid file extension";
		return nullptr;
	}

	const FString AssetPathFolder = FPaths::GetPath(AssetFullPath);
	const FString AssetName = FPaths::GetBaseFilename(AssetFullPath);

	FString MeshActorName = TEXT("A_") + AssetName;
	FString MeshActorPath = TargetAssetPath / MeshActorName;

	{
		if (const TSubclassOf<AActor> ExistingBlueprint = LoadClass<AActor>(
			nullptr,
			*(MeshActorPath + TEXT(".") + MeshActorName + TEXT("_C")),
			nullptr,
			LOAD_Quiet | LOAD_NoWarn
		))
		{
			return ExistingBlueprint.Get();
		}
	}

	UE_LOG(LogSCPEditor, Display, TEXT("importing rmesh from %s"), *AssetFullPath);

	const TUniquePtr<FArchive> Reader{IFileManager::Get().CreateFileReader(*AssetFullPath)};

	if (!Reader)
	{
		ErrCode = FString::Format(TEXT("Failed to import asset: couldn't open \"{0}\""), {AssetFullPath});
		check(false);
		return nullptr;
	}

	const FString OriginalGamePath = UConfig::Get().OriginalGamePath;


	TArray<UStaticMesh*> StaticMeshList;

	TArray<TPair<FRawMesh, FTextureData>> MeshData;
	Importer->Import(*Reader, MeshData);

	StaticMeshList.SetNum(MeshData.Num());

	for (int32 MeshDataI = 0; MeshDataI < MeshData.Num(); MeshDataI++)
	{
		auto& [RawMesh, TextureData] = MeshData[MeshDataI];
		UMaterial* Material = nullptr;

		if (!TextureData.Texture.IsEmpty())
		{
			FString TextureName = TextureData.Texture;

			FString AbsoluteTexturePath = FPaths::IsRelative(TextureName) ? AssetPathFolder / TextureName : TextureName;

			if (!FPaths::FileExists(AbsoluteTexturePath))
			{
				check(false);
				return nullptr;
			}

			FString TextureBaseName = TEXT("T_") + FPaths::GetBaseFilename(TextureName);
			FString TextureAssetPath = UPackageTools::SanitizePackageName(TargetAssetPath / TextureBaseName);

			UTexture2D* Tex = GetOrImportTexture(TextureAssetPath, AbsoluteTexturePath, false, &ErrCode);
			if (Tex == nullptr)
			{
				check(false);
				return nullptr;
			}

			FTextureDataEx TextureEx;
			TextureEx.Tex = Tex;

			if (!Tex)
			{
				check(false);
				return nullptr;
			}

			FString BumpPath;
			const bool bFound = GConfig->GetString(
				*FPaths::GetBaseFilename(TextureName),
				TEXT("bump"),
				BumpPath,
				OriginalGamePath / TEXT("Data/materials.ini"));
			if (bFound)
			{
				BumpPath.ReplaceCharInline(TEXT('\\'), TEXT('/'));
				FString BumpName = TEXT("T_") + FPaths::GetBaseFilename(BumpPath) + TEXT("_B");
				TextureEx.Bump = GetOrImportTexture(
					TargetAssetPath / BumpName,
					OriginalGamePath / BumpPath,
					true
				);
			}

			// Generate Material
			if (Tex)
			{
				const FString MaterialBaseName = TEXT("M_") + FPaths::GetBaseFilename(TextureName);
				const FString MaterialPath = TargetAssetPath / MaterialBaseName;
				UPackage* Package = CreatePackage(*MaterialPath);

				Material = LoadObject<UMaterial>(nullptr, *MaterialPath,
				                                 nullptr, LOAD_Quiet | LOAD_NoWarn);
				if (Material == nullptr)
				{
					UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
					Material = (UMaterial*)MaterialFactory->FactoryCreateNew(
						UMaterial::StaticClass(), Package, *MaterialBaseName,
						RF_Public | RF_Standalone, nullptr, GWarn);
					FAssetRegistryModule::AssetCreated(Material);
					Package->SetDirtyFlag(true);

					{
						auto* BaseColorSamplerNode = NewObject<UMaterialExpressionTextureSample>(Material);
						Material->Expressions.Add(BaseColorSamplerNode);
						Material->BaseColor.Expression = BaseColorSamplerNode;

						BaseColorSamplerNode->MaterialExpressionEditorX = -336;
						BaseColorSamplerNode->Texture = TextureEx.Tex;
						BaseColorSamplerNode->SamplerType = SAMPLERTYPE_Color;

						if (TextureEx.Bump)
						{
							auto* BumpOffset = NewObject<UMaterialExpressionBumpOffset>(Material);
							BumpOffset->MaterialExpressionEditorX = -624;
							Material->Expressions.Add(BumpOffset);
							BaseColorSamplerNode->Coordinates.Expression = BumpOffset;

							{
								auto* BumpMapSamplerNode = NewObject<UMaterialExpressionTextureSample>(Material);
								BumpMapSamplerNode->MaterialExpressionEditorX = -896;
								Material->Expressions.Add(BumpMapSamplerNode);
								BumpOffset->Height.Expression = BumpMapSamplerNode;

								BumpMapSamplerNode->Texture = TextureEx.Bump;
								BumpMapSamplerNode->SamplerType = SAMPLERTYPE_Normal;
							}
						}
					}

					Material->PreEditChange(nullptr);
					Material->PostEditChange();
					FGlobalComponentReregisterContext RecreateComponents;
				}
			}
		}

		FString MeshHash = Hash(RawMesh);

		FString MeshAssetPath = TargetAssetPath / TEXT("SM_Mesh_") + MeshHash;

		{
			FString MeshObjectAssetPath = MeshAssetPath + TEXT(".") + FPaths::GetBaseFilename(MeshAssetPath);

			UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *MeshObjectAssetPath, nullptr,
			                                                  LOAD_Quiet | LOAD_NoWarn);
			if (StaticMesh != nullptr)
			{
				StaticMeshList[MeshDataI] = StaticMesh;
				continue;
			}
		}

		TArray<FStaticMaterial> MaterialArray;
		if (Material)
		{
			MaterialArray.Add(FStaticMaterial(Material));
		}

		FString Err;
		UStaticMesh* StaticMesh = SaveRawMeshToAsset(
			RawMesh, MaterialArray,
			MeshAssetPath,
			&Err);

		if (!StaticMesh)
		{
			ErrCode = FString::Format(TEXT("could not save StaticMesh #{0}\n{1}"), {MeshDataI, Err});
			return nullptr;
		}

		StaticMeshList[MeshDataI] = StaticMesh;
	}

	UPackage* ActorMeshPackage = CreatePackage(*MeshActorPath);

	UClass* BlueprintClass = nullptr;
	UClass* BlueprintGeneratedClass = nullptr;
	IKismetCompilerInterface& KismetCompilerModule = FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(
		"KismetCompiler");
	KismetCompilerModule.GetBlueprintTypesForClass(AActor::StaticClass(), BlueprintClass, BlueprintGeneratedClass);

	UBlueprint* MeshBlueprint = FKismetEditorUtilities::CreateBlueprint(
		AActor::StaticClass(),
		ActorMeshPackage,
		*MeshActorName,
		BPTYPE_Normal,
		BlueprintClass,
		BlueprintGeneratedClass
	);

	USCS_Node* RootNode = MeshBlueprint->SimpleConstructionScript->CreateNode(USceneComponent::StaticClass());
	MeshBlueprint->SimpleConstructionScript->AddNode(RootNode);

	for (int32 MeshI = 0; MeshI < StaticMeshList.Num(); MeshI++)
	{
		UStaticMesh* StaticMesh = StaticMeshList[MeshI];

		USCS_Node* Node = MeshBlueprint->SimpleConstructionScript->CreateNode(UStaticMeshComponent::StaticClass());

		Node->SetVariableName(FName(
			*(FString(TEXT("Mesh_")) + FString::FromInt(MeshI))
		));

		Cast<UStaticMeshComponent>(Node->ComponentTemplate.Get())->bCastDynamicShadow = false;

		FComponentAssetBrokerage::AssignAssetToComponent(Node->ComponentTemplate, StaticMesh);
		RootNode->AddChildNode(Node);
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(MeshBlueprint);
	FBPCompileRequest CompileRequest(MeshBlueprint, EBlueprintCompileOptions::None, nullptr);
	FBlueprintCompilationManager::CompileSynchronously(CompileRequest);

	ActorMeshPackage->SetDirtyFlag(true);
	FAssetRegistryModule::AssetCreated(MeshBlueprint);

	return MeshBlueprint->GeneratedClass.Get();
}

void FOriginalAssetImporter::ImportLevelInteractive(const FString& Path) const
{
	const TSharedPtr<SDlgPickPath> PickAssetPathWidget = SNew(SDlgPickPath)
		.Title(FText::FromString(TEXT("Choose save assets location Location")));
	if (PickAssetPathWidget->ShowModal() != EAppReturnType::Ok)
	{
		return;
	}

	const FString TargetAssetPath = PickAssetPathWidget->GetPath().ToString();

	if (FString Err; !ImportLevel(Path, TargetAssetPath, Err))
	{
		const FText Msg = FText::FromString(Err);
		UE_LOG(LogSCPEditor, Error, TEXT("Import level error %s"), *Err);
		SOutputLogDialog::Open(
			FText::FromString(TEXT("Import error")),
			FText::FromString(TEXT("Could not import")),
			Msg
		);
	}
}

bool FOriginalAssetImporter::ImportLevel(const FString& AssetFullPath, const FString& TargetAssetPath,
                                         FString& ErrCode) const
{
	const FString OriginalGamePath = UConfig::Get().OriginalGamePath;
	const FString AssetName = FPaths::GetBaseFilename(AssetFullPath);

	UE_LOG(LogSCPEditor, Display, TEXT("importing level from %s"), *AssetFullPath);

	const TUniquePtr<FArchive> Reader{IFileManager::Get().CreateFileReader(*AssetFullPath)};

	int WaitingBreakLines = 2;
	while (WaitingBreakLines > 0)
	{
		ANSICHAR Char;
		if (!(Reader >> Char))
		{
			ErrCode = "invalid file format";
			return false;
		}
		if (Char == '\n') WaitingBreakLines--;
	}

	int8 TransitionZones[2];
	Reader >> TransitionZones;

	int32 RoomAmount, ForestPieceAmount, MTAmount;
	Reader >> RoomAmount;
	Reader >> ForestPieceAmount;
	Reader >> MTAmount;

	TArray<FRoomTemplate> FacilityRooms;
	FacilityRooms.SetNum(RoomAmount);

	for (int RoomI = 0; RoomI < RoomAmount; RoomI++)
	{
		FRoomTemplate& Room = FacilityRooms[RoomI];
		Reader >> Room.XPos;
		Reader >> Room.YPos;
		Room.Name = FRMeshImporter::ReadString(*Reader);
		Room.Name.ToLowerInline();
		Reader >> Room.Angle;

		Room.EventName = FRMeshImporter::ReadString(*Reader);
		Reader >> Room.EventProb;
	}

	const FString SavePath = TargetAssetPath / AssetName;

	UPackage* SavePackage = CreatePackage(*SavePath);

	//World'/Game/FirstPersonCPP/Maps/FirstPersonExampleMap.FirstPersonExampleMap'
	UWorld* World = (UWorld*)WorldFactory->FactoryCreateNew(UWorld::StaticClass(), SavePackage, *AssetName,
	                                                        RF_Public | RF_Standalone, nullptr, GWarn);
	FGCObjectScopeGuard DontGCWorld(World);

	const FString RoomIniFilename = OriginalGamePath / TEXT("Data/rooms.ini");
	FConfigFile* Config = GConfig->Find(RoomIniFilename);

	FScopedSlowTask ImportTask(FacilityRooms.Num(), FText::FromString(TEXT("Generating & Importing meshes")));
	ImportTask.MakeDialog();

	for (const FRoomTemplate& Room : FacilityRooms)
	{
		TSubclassOf<AActor> Actor;

		ImportTask.EnterProgressFrame();
		FConfigSection* Section = Config->Find(Room.Name);
		if (!Section)
		{
			UE_LOG(LogSCPEditor, Warning, TEXT("skipping room %s: config section not found"), *Room.Name);
			continue;
		}
		const FConfigValue* Value = Section->Find(TEXT("mesh path"));
		if (!Value)
		{
			UE_LOG(LogSCPEditor, Warning, TEXT("skipping room %s: config mesh path not found"), *Room.Name);
			continue;
		}
		FString RoomAssetPath = Value->GetValue();
		RoomAssetPath.ReplaceCharInline(TEXT('\\'), TEXT('/'));

		FString Err;
		Actor = ImportRMesh(OriginalGamePath / RoomAssetPath, TargetAssetPath, Err);
		if (!Actor)
		{
			UE_LOG(LogSCPEditor, Warning, TEXT("skipping room %s: failed to load actor"), *Room.Name);
			continue;
		}

		int Angle = Room.Angle; // In Degress
		if (Angle != 0 || Angle != 2)
		{
			Angle += 2;
		}
		Angle = (Angle + 1) % 4;
		Angle *= 90;

		FTransform Transform;
		Transform.SetLocation(FVector3d(Room.XPos * 2048.0, Room.YPos * 2048.0, 0.0));
		Transform.SetRotation(FRotator::MakeFromEuler(FVector3d::UpVector * Angle).Quaternion());
		World->SpawnActor(Actor.Get(), &Transform);
	}

	FAssetRegistryModule::AssetCreated(World);
	SavePackage->SetDirtyFlag(true);

	return true;
}

UTexture2D* FOriginalAssetImporter::GetOrImportTexture(const FString& ObjectPath,
                                                       const FString& ImportPath,
                                                       bool IsBump,
                                                       FString* Err) const
{
	const FString TextureBaseName = FPaths::GetBaseFilename(ObjectPath);
	const FString AssetPath = ObjectPath + TEXT(".") + TextureBaseName;

	UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *AssetPath,
	                                         nullptr, LOAD_Quiet | LOAD_NoWarn);
	if (Tex)
	{
		return Tex;
	}

	if (ImportPath.IsEmpty())
	{
		if (Err)
		{
			*Err = FString::Format(TEXT("texture not found in asset registry: {0}"), {*ObjectPath});
		}
		return nullptr;
	}

	UPackage* AssetPackage = CreatePackage(*ObjectPath);

	bool bCanceled = false;
	Tex = (UTexture2D*)TextureFactory->FactoryCreateFile(
		UTexture2D::StaticClass(),
		AssetPackage,
		*TextureBaseName,
		RF_Standalone | RF_Public,
		ImportPath,
		nullptr,
		GWarn,
		bCanceled
	);
	if (bCanceled)
	{
		if (Err)
		{
			*Err = FString::Format(TEXT("texture load canceled\nImport of ({0}) from {1}"), {ObjectPath, ImportPath});
		}
		return nullptr;
	}
	if (!Tex)
	{
		if (Err)
		{
			*Err = FString::Format(TEXT("could not load texture ({0}) from {1}"), {ObjectPath, ImportPath});
		}
		return nullptr;
	}

	if (IsBump)
	{
		Tex->LODGroup = TEXTUREGROUP_WorldNormalMap;
	}

	Tex->UpdateResource();
	Tex->PostEditChange();

	FAssetRegistryModule::AssetCreated(Tex);
	AssetPackage->SetDirtyFlag(true);

	return Tex;
}

UStaticMesh* FOriginalAssetImporter::SaveRawMeshToAsset(FRawMesh& Mesh,
                                                        const TArray<FStaticMaterial>& Materials,
                                                        const FString& SavePath, FString* Err) const
{
	if (!Mesh.IsValidOrFixable())
	{
		if (Err)
		{
			*Err = "invalid raw mesh";
		}
		return nullptr;
	}

	UPackage* Package = CreatePackage(*SavePath);

	// const FName MeshName = MakeUniqueObjectName(Package, UStaticMesh::StaticClass(), FName(*FPaths::GetCleanFilename(SavePath)));
	const FName MeshName = FName(*FPaths::GetCleanFilename(SavePath));

	UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, MeshName, RF_Public | RF_Standalone);
	StaticMesh->InitResources();
	StaticMesh->SetLightingGuid();

	FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();
	SrcModel.BuildSettings.bRecomputeNormals = true;
	SrcModel.BuildSettings.bRecomputeTangents = true;
	SrcModel.BuildSettings.bRemoveDegenerates = true;
	SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
	SrcModel.BuildSettings.bUseFullPrecisionUVs = false;
	SrcModel.BuildSettings.bGenerateLightmapUVs = true;
	SrcModel.BuildSettings.SrcLightmapIndex = 0;
	SrcModel.BuildSettings.DstLightmapIndex = MAX_MESH_TEXTURE_COORDS - 1;
	SrcModel.SaveRawMesh(Mesh);

	StaticMesh->SetLightMapCoordinateIndex(MAX_MESH_TEXTURE_COORDS - 1);

	for (int32 MaterialID = 0; MaterialID < Materials.Num(); MaterialID++)
	{
		StaticMesh->GetStaticMaterials().Add(Materials[MaterialID]);
	}

	TArray<int32> UniqueMaterialIndices;
	for (int32 MaterialIndex : Mesh.FaceMaterialIndices)
	{
		UniqueMaterialIndices.AddUnique(MaterialIndex);
	}

	int32 SectionIndex = 0;
	for (const int32 UniqueMaterialIndex : UniqueMaterialIndices)
	{
		StaticMesh->GetSectionInfoMap().Set(0, SectionIndex, FMeshSectionInfo(UniqueMaterialIndex));
		SectionIndex++;
	}

	StaticMesh->GetOriginalSectionInfoMap().CopyFrom(StaticMesh->GetSectionInfoMap());

	StaticMesh->ImportVersion = LastVersion;
	StaticMesh->CreateBodySetup();
	UBodySetup* Body = StaticMesh->GetBodySetup();
	Body->CollisionTraceFlag = CTF_UseComplexAsSimple;
	Body->bMeshCollideAll = 1;

	StaticMesh->Build(false);
	StaticMesh->PostEditChange();

	// ReSharper disable once CppExpressionWithoutSideEffects
	Package->SetDirtyFlag(true);

	FAssetRegistryModule::AssetCreated(StaticMesh);

	return StaticMesh;
}

// Old Import Texture2D code
/*
UTexture2D* FOriginalAssetImporter::SaveTextureToAsset(UTexture2D* TransientTex, const FString& SavePath,
													   FString& Err) const
{
	UPackage* Package = CreatePackage(*SavePath);

	// const FName MeshName = MakeUniqueObjectName(Package, UTexture2D::StaticClass(), FName(*FPaths::GetBaseFilename(SavePath)));
	const FName MeshName = FName(*FPaths::GetBaseFilename(SavePath));

	UTexture2D* Tex = NewObject<UTexture2D>(Package, MeshName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	Tex->AddToRoot();

	// Steals the TransientTex data
	FTexturePlatformData* PlatformData = TransientTex->GetPlatformData();
	Tex->SetPlatformData(PlatformData);
	TransientTex->SetPlatformData(nullptr);

	FByteBulkData Bulk = PlatformData->Mips[0].BulkData;

	void* PixelsData = Bulk.Lock(LOCK_READ_ONLY);
	Tex->Source.Init(
		PlatformData->SizeX, PlatformData->SizeY,
		1, 1,
		ETextureSourceFormat::TSF_BGRA8,
		static_cast<uint8*>(PixelsData)
	);
	Bulk.Unlock();

	Tex->UpdateResource();
	// ReSharper disable once CppExpressionWithoutSideEffects
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(Tex);

	const FString TexAssetPath = FPackageName::LongPackageNameToFilename(
		SavePath, FPackageName::GetAssetPackageExtension()
	);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	SaveArgs.bForceByteSwapping = true;

	if (const bool Saved = UPackage::SavePackage(Package, Tex, *TexAssetPath, SaveArgs); !Saved)
	{
		Tex->RemoveFromRoot();
		Err = FString::Format(TEXT("Could not save the asset \"{0}\"package"), {SavePath});
		return nullptr;
	}

	Tex->RemoveFromRoot();
	return Tex;
}
 */
