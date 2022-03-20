#pragma once

#include "SlateBasics.h"

class FSCPEditorStyle
{
	class FStyle : public FSlateStyleSet
	{
	public:
		FStyle();

		void Initialize();
	};
public:
	static void Initialize();

	static void Shutdown();

	static const class ISlateStyle& Get();

	static FName GetStyleSetName();

private:
	static TSharedPtr<FStyle> Create();

	static TSharedPtr<FStyle> StyleInstance;
};
