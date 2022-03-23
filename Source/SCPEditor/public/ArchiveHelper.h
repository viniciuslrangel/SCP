#pragma once

#include "Serialization/Archive.h"

template <typename T, SIZE_T Size = sizeof(T)>
bool operator>>(FArchive& Archive, T& Target)
{
	static_assert(!std::is_same_v<T, FString>, "cannot read FString direct from archive");
	
	if (Archive.GetArchiveState().IsError())
	{
		return false;
	}
	Archive.Serialize(&Target, Size);
	return true;
}

template <typename T, SIZE_T Size = sizeof(T)>
bool operator>>(FArchive& Archive, T* Target)
{
	return operator>><T, Size>(Archive, *Target);
}

struct FArchiveSizableReader
{
	FArchive& Archive;
	SIZE_T Size;

	template <typename T>
	bool operator>>(T& Target)
	{
		if (Archive.GetArchiveState().IsError())
		{
			return false;
		}
		Archive.Serialize(&Target, Size);
		return true;
	}

	template <typename T>
	bool operator>>(T* Target)
	{
		return this->operator>>(*Target);
	}
};

inline FArchiveSizableReader operator*(FArchive& Archive, const SIZE_T Size)
{
	return FArchiveSizableReader{Archive, Size};
}

template <typename T, SIZE_T Size = sizeof(T)>
bool operator>>(const TUniquePtr<FArchive>& Archive, T& Target)
{
	return operator>><T, Size>(*Archive, Target);
}

inline FString ReadBlitzString(FArchive& Archive)
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
