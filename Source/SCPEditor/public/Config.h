#pragma once

#include "Config.generated.h"

UCLASS(Config=Editor, defaultconfig)
class UConfig : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UConfig()
	{
	}

	static UConfig& Get()
	{
		static UConfig* Config = nullptr;
		if(!IsValid(Config))
		{
			Config = NewObject<UConfig>();
			Config->AddToRoot();
		}
		return *Config;
	}

	UPROPERTY(Config, EditAnywhere, Category="AssetLoader")
	FString OriginalGamePath = "";

};
