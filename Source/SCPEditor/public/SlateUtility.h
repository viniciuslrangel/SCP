#pragma once

class FSlateUtility
{
public:
	inline static constexpr auto AddBorderLabel = [](FText Label, auto Widget) -> auto
	{
		return SNew(SBorder)
			.Padding(1.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().Padding(5.0f, 5.0f)[SNew(STextBlock).Text(Label)]
				+ SVerticalBox::Slot().Padding(15.0f, 0.0f, 5.0f, 10.0f).AutoHeight()
				[
					SNew(SBorder)[Widget]
				]
			];
	};

	inline static constexpr auto AddLabel = [](FText Label, auto Widget) -> auto
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(true)
			[
				SNew(STextBlock).Text(Label)
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				Widget
			];
	};
};
