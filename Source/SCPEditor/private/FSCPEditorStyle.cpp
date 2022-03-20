#include "FSCPEditorStyle.h"

#include "Slate/SlateGameResources.h"

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define IMAGE_BRUSH_SVG( RelativePath, ... ) FSlateVectorImageBrush(RootToContentDir(RelativePath, TEXT(".svg")), __VA_ARGS__)

TSharedPtr<FSCPEditorStyle::FStyle> FSCPEditorStyle::StyleInstance = nullptr;

void FSCPEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FSCPEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

const ISlateStyle& FSCPEditorStyle::Get()
{
	return *StyleInstance;
}

FName FSCPEditorStyle::GetStyleSetName()
{
	static FName StyleSetName("SCPEditorStyles");
	return StyleSetName;
}

TSharedPtr<FSCPEditorStyle::FStyle> FSCPEditorStyle::Create()
{
	TSharedRef<FStyle> Style = MakeShareable(new FStyle);
	Style->Initialize();
	return Style;
}

FSCPEditorStyle::FStyle::FStyle() : FSlateStyleSet("SCPEditorStyles")
{
}

void FSCPEditorStyle::FStyle::Initialize()
{
	SetContentRoot(FPaths::ProjectContentDir() / TEXT("Editor/Slate"));
	SetCoreContentRoot(FPaths::ProjectContentDir() / TEXT("Slate"));

	
	Set("Toolbar.OpenIcon", new IMAGE_BRUSH("Icons/SCP", FVector2D(40.0f, 40.0f)));
	Set("Toolbar.OpenIcon.small", new IMAGE_BRUSH("Icons/SCP.small", FVector2D(20.0f, 20.0f)));

	Set("Toolbar.OpenIcon.White", new IMAGE_BRUSH("Icons/SCP.white", FVector2D(40.0f, 40.0f)));
	Set("Toolbar.OpenIcon.White.small", new IMAGE_BRUSH("Icons/SCP.white.small", FVector2D(20.0f, 20.0f)));
}
