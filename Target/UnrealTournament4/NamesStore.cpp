#include <windows.h>

#include "PatternFinder.hpp"
#include "NamesStore.hpp"

#include "EngineClasses.hpp"

class FNameEntry
{
public:
	__int32 Index;
	char pad_0x0004[0x4];
	FNameEntry* HashNext;
	union
	{
		char AnsiName[1024];
		wchar_t WideName[1024];
	};

	const char* GetName() const
	{
		return AnsiName;
	}
};

template<typename ElementType, int32_t MaxTotalElements, int32_t ElementsPerChunk>
class TStaticIndirectArrayThreadSafeRead
{
public:
	int32_t Num() const
	{
		return NumElements;
	}

	bool IsValidIndex(int32_t index) const
	{
		return index >= 0 && index < Num() && GetById(index) != nullptr;
	}

	ElementType const* const& GetById(int32_t index) const
	{
		return *GetItemPtr(index);
	}

private:
	ElementType const* const* GetItemPtr(int32_t Index) const
	{
		int32_t ChunkIndex = Index / ElementsPerChunk;
		int32_t WithinChunkIndex = Index % ElementsPerChunk;
		ElementType** Chunk = Chunks[ChunkIndex];
		return Chunk + WithinChunkIndex;
	}

	enum
	{
		ChunkTableSize = (MaxTotalElements + ElementsPerChunk - 1) / ElementsPerChunk
	};

	ElementType** Chunks[ChunkTableSize];
	__int32 NumElements;
	__int32 NumChunks;
};

//4.22 using TNameEntryArray = TStaticIndirectArrayThreadSafeRead<FNameEntry, 4 * 1024 * 1024, 16384>;
//总共4M个ElementType,每个Chunk16K个ElementType,总共128k个chunck
using TNameEntryArray = TStaticIndirectArrayThreadSafeRead<FNameEntry, 2 * 1024 * 1024, 1024 * 16>;

TNameEntryArray* GlobalNames = nullptr;

bool NamesStore::Initialize()
{
	auto address = FindPattern(GetModuleHandleW(L"UE4-Core-Win64-Shipping.dll"), reinterpret_cast<const unsigned char*>("\x48\x8B\x1D\x00\x00\x00\x00\x48\x85\xDB\x75\x35"), "xxx????xxxxx");
	if (address == -1)
	{
		return false;
	}

	auto offset = *reinterpret_cast<uint32_t*>(address + 3);
	GlobalNames = reinterpret_cast<decltype(GlobalNames)>(*reinterpret_cast<uintptr_t*>(address + 7 + offset));

	return true;
}

void* NamesStore::GetAddress()
{
	return GlobalNames;
}

size_t NamesStore::GetNamesNum() const
{
	return GlobalNames->Num();
}

bool NamesStore::IsValid(size_t id) const
{
	return GlobalNames->IsValidIndex(static_cast<int32_t>(id));
}

std::string NamesStore::GetById(size_t id) const
{
	return GlobalNames->GetById(static_cast<int32_t>(id))->GetName();
}
