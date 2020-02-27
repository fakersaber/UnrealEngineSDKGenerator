#include <windows.h>

#include "PatternFinder.hpp"
#include "ObjectsStore.hpp"

#include "EngineClasses.hpp"

class FUObjectItem
{
public:
	UObject* Object; //0x0000
	__int32 Flags; //0x0008
	__int32 ClusterIndex; //0x000C
	__int32 SerialNumber; //0x0010
};


// FChunkedFixedUObjectArray 引擎结构已经改变
class TUObjectArray
{
	//FFixedUObjectArray 
public:
	//mater table to chunks of pointers
	FUObjectItem* Objects;
	int32_t MaxElements;
	int32_t NumElements;
};

class FChunkeFixedUObjectArray
{
public:
	enum {
		NumElementsPerChunk = 64 * 1024,
	};

	FUObjectItem** Objects;
	FUObjectItem** PreAllocatedObjects;
	__int32 MaxElements;
	__int32 NumElements;
	__int32 MaxChunks;
	__int32 NumChunks;
};



class FUObjectArray
{
public:

	//First index into objects array taken into account for GC
	__int32 ObjFirstGCIndex; //0x0000

	//Index pointing to last object created in range disregarded for GC
	__int32 ObjLastNonGCIndex; //0x0004

	//最大数量
	__int32 MaxObjectsNotConsideredByGC; //0x0008

	//bool类型
	__int32 OpenForDisregardForGC; //0x000C

	//Array of all live objects
	FChunkeFixedUObjectArray ObjObjects; //0x0010
};

FUObjectArray* GlobalObjects = nullptr;

bool ObjectsStore::Initialize()
{
	auto address = FindPattern(GetModuleHandleW(L"UE4-CoreUObject-Win64-Shipping.dll"), reinterpret_cast<const unsigned char*>("\x48\x8D\x0D\x00\x00\x00\x00\xC6\x05"), "xxx????xx");
	if (address == -1)
	{
		return false;
	}
	auto offset = *reinterpret_cast<uint32_t*>(address + 3);
	GlobalObjects = reinterpret_cast<decltype(GlobalObjects)>(address + 7 + offset);

	return true;
}

void* ObjectsStore::GetAddress()
{
	return GlobalObjects;
}

size_t ObjectsStore::GetObjectsNum() const
{
	return GlobalObjects->ObjObjects.NumElements;
}

UEObject ObjectsStore::GetById(size_t id) const
{
	const int32_t ChunkIndex = id / FChunkeFixedUObjectArray::NumElementsPerChunk;
	const int32_t WithinChunck = id % FChunkeFixedUObjectArray::NumElementsPerChunk;
	FUObjectItem* CurUObject = GlobalObjects->ObjObjects.Objects[ChunkIndex] + WithinChunck;
	return CurUObject->Object;
}
