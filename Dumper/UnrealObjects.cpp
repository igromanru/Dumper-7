#include <format>

#include "UnrealObjects.h"
#include "Offsets.h"
#include "ObjectArray.h"


void* UEFFieldClass::GetAddress()
{
	return Class;
}

UEFFieldClass::operator bool() const
{
	return Class != nullptr;
}

EFieldClassID UEFFieldClass::GetId() const
{
	return *reinterpret_cast<EFieldClassID*>(Class + Off::FFieldClass::Id);
}

EClassCastFlags UEFFieldClass::GetCastFlags() const
{
	return *reinterpret_cast<EClassCastFlags*>(Class + Off::FFieldClass::CastFlags);
}

EClassFlags UEFFieldClass::GetClassFlags() const
{
	return *reinterpret_cast<EClassFlags*>(Class + Off::FFieldClass::ClassFlags);
}

UEFFieldClass UEFFieldClass::GetSuper() const
{
	return UEFFieldClass(*reinterpret_cast<void**>(Class + Off::FFieldClass::SuperClass));
}

FName UEFFieldClass::GetFName() const
{
	return FName(Class + Off::FFieldClass::Name); //Not the real FName, but a wrapper which holds the address of a FName
}

bool UEFFieldClass::IsType(EClassCastFlags Flags) const
{
	return (Flags != EClassCastFlags::None ? (GetCastFlags() & Flags) : true);
}

std::string UEFFieldClass::GetName() const
{
	return Class ? GetFName().ToString() : "None";
}

std::string UEFFieldClass::GetValidName() const
{
	return Class ? GetFName().ToValidString() : "None";
}

std::string UEFFieldClass::GetCppName() const
{
	// This is evile dark magic code which shouldn't exist
	return "F" + GetValidName();
}

void* UEFField::GetAddress()
{
	return Field;
}

EObjectFlags UEFField::GetFlags() const
{
	return *reinterpret_cast<EObjectFlags*>(Field + Off::FField::Flags);
}

class UEObject UEFField::GetOwnerAsUObject() const
{
	if (IsOwnerUObject())
	{
		if (Settings::Internal::bUseMaskForFieldOwner)
			return (void*)(*reinterpret_cast<uintptr_t*>(Field + Off::FField::Owner) & ~0x1ull);

		return *reinterpret_cast<void**>(Field + Off::FField::Owner);
	}

	return nullptr;
}

class UEFField UEFField::GetOwnerAsFField() const
{
	if (!IsOwnerUObject())
		return *reinterpret_cast<void**>(Field + Off::FField::Owner);

	return nullptr;
}

class UEObject UEFField::GetOwnerUObject() const
{
	UEFField Field = *this;

	while (!Field.IsOwnerUObject() && Field.GetOwnerAsFField())
	{
		Field = Field.GetOwnerAsFField();
	}

	return Field.GetOwnerAsUObject();
}

UEFFieldClass UEFField::GetClass() const
{
	return UEFFieldClass(*reinterpret_cast<void**>(Field + Off::FField::Class));
}

FName UEFField::GetFName() const
{
	return FName(Field + Off::FField::Name); //Not the real FName, but a wrapper which holds the address of a FName
}

UEFField UEFField::GetNext() const
{
	return UEFField(*reinterpret_cast<void**>(Field + Off::FField::Next));
}

template<typename UEType>
UEType UEFField::Cast() const
{
	return UEType(Field);
}

bool UEFField::IsOwnerUObject() const
{
	if (Settings::Internal::bUseMaskForFieldOwner)
	{
		return *reinterpret_cast<uintptr_t*>(Field + Off::FField::Owner) & 0x1;
	}

	return *reinterpret_cast<bool*>(Field + Off::FField::Owner + 0x8);
}

bool UEFField::IsA(EClassCastFlags Flags) const
{
	return (Flags != EClassCastFlags::None ? GetClass().IsType(Flags) : true);
}

std::string UEFField::GetName() const
{
	return Field ? GetFName().ToString() : "None";
}

std::string UEFField::GetValidName() const
{
	return Field ? GetFName().ToValidString() : "None";
}

std::string UEFField::GetCppName() const
{
	static UEClass ActorClass = ObjectArray::FindClassFast("Actor");
	static UEClass InterfaceClass = ObjectArray::FindClassFast("Interface");

	std::string Temp = GetValidName();

	if (IsA(EClassCastFlags::Class))
	{
		if (Cast<UEClass>().HasType(ActorClass)) 
		{
			return 'A' + Temp;
		}
		else if (Cast<UEClass>().HasType(InterfaceClass)) 
		{
			return 'I' + Temp;
		}

		return 'U' + Temp;
	}

	return 'F' + Temp;
}

UEFField::operator bool() const
{
	return Field != nullptr && reinterpret_cast<void*>(Field + Off::FField::Class) != nullptr;
}

bool UEFField::operator==(const UEFField& Other) const
{
	return Field == Other.Field;
}

bool UEFField::operator!=(const UEFField& Other) const
{
	return Field != Other.Field;
}

void(*UEObject::PE)(void*, void*, void*) = nullptr;

void* UEObject::GetAddress()
{
	return Object;
}

void* UEObject::GetVft() const
{
	return *reinterpret_cast<void**>(Object);
}

EObjectFlags UEObject::GetFlags() const
{
	return *reinterpret_cast<EObjectFlags*>(Object + Off::UObject::Flags);
}

int32 UEObject::GetIndex() const
{
	return *reinterpret_cast<int32*>(Object + Off::UObject::Index);
}

UEClass UEObject::GetClass() const
{
	return UEClass(*reinterpret_cast<void**>(Object + Off::UObject::Class));
}

FName UEObject::GetFName() const
{
	return FName(Object + Off::UObject::Name); //Not the real FName, but a wrapper which holds the address of a FName
}

UEObject UEObject::GetOuter() const
{
	return UEObject(*reinterpret_cast<void**>(Object + Off::UObject::Outer));
}

int32 UEObject::GetPackageIndex() const
{
	return GetOutermost().GetIndex();
}

bool UEObject::HasAnyFlags(EObjectFlags Flags) const
{
	return GetFlags() & Flags;
}

bool UEObject::IsA(EClassCastFlags TypeFlags) const
{
	return (TypeFlags != EClassCastFlags::None ? GetClass().IsType(TypeFlags) : true);
}

bool UEObject::IsA(UEClass Class) const
{
	if (!Class)
		return false;

	for (UEClass Clss = GetClass(); Clss; Clss = Clss.GetSuper().Cast<UEClass>())
	{
		if (Clss == Class)
			return true;
	}

	return false;
}

UEObject UEObject::GetOutermost() const
{
	UEObject Outermost = *this;

	for (UEObject Outer = *this; Outer; Outer = Outer.GetOuter())
	{
		Outermost = Outer;
	}

	return Outermost;
}

std::string UEObject::StringifyObjFlags() const
{
	return *this ? StringifyObjectFlags(GetFlags()) : "NoFlags";
}

std::string UEObject::GetName() const
{
	return Object ? GetFName().ToString() : "None";
}

std::string UEObject::GetValidName() const
{
	return Object ? GetFName().ToValidString() : "None";
}

std::string UEObject::GetCppName() const
{
	static UEClass ActorClass = nullptr;
	static UEClass InterfaceClass = nullptr;
	
	if (ActorClass == nullptr)
		ActorClass = ObjectArray::FindClassFast("Actor");
	
	if (InterfaceClass == nullptr)
		InterfaceClass = ObjectArray::FindClassFast("Interface");

	std::string Temp = GetValidName();

	if (IsA(EClassCastFlags::Class))
	{
		if (Cast<UEClass>().HasType(ActorClass))
		{
			return 'A' + Temp;
		}
		else if (Cast<UEClass>().HasType(InterfaceClass))
		{
			return 'I' + Temp;
		}

		return 'U' + Temp;
	}

	return 'F' + Temp;
}

std::string UEObject::GetFullName(int32& OutNameLength)
{
	if (*this)
	{
		std::string Temp;

		for (UEObject Outer = GetOuter(); Outer; Outer = Outer.GetOuter())
		{
			Temp = Outer.GetName() + "." + Temp;
		}

		std::string Name = GetName();
		OutNameLength = Name.size() + 1;

		Name = GetClass().GetName() + " " + Temp + Name;

		return Name;
	}

	return "None";
}

std::string UEObject::GetFullName() const
{
	if (*this)
	{
		std::string Temp;

		for (UEObject Outer = GetOuter(); Outer; Outer = Outer.GetOuter())
		{
			Temp = Outer.GetName() + "." + Temp;
		}

		std::string Name = GetClass().GetName();
		Name += " ";
		Name += Temp;
		Name += GetName();

		return Name;
	}

	return "None";
}

UEObject::operator bool() const
{
	// if an object is 0x10000F000 it passes the nullptr check
	return Object != nullptr && reinterpret_cast<void*>(Object + Off::UObject::Class) != nullptr;
}

UEObject::operator uint8*()
{
	return Object;
}

bool UEObject::operator==(const UEObject& Other) const
{
	return Object == Other.Object;
}

bool UEObject::operator!=(const UEObject& Other) const
{
	return Object != Other.Object;
}

void UEObject::ProcessEvent(UEFunction Func, void* Params)
{
	void** VFT = *reinterpret_cast<void***>(GetAddress());

	void(*Prd)(void*, void*, void*) = decltype(Prd)(VFT[Off::InSDK::ProcessEvent::PEIndex]);

	Prd(Object, Func.GetAddress(), Params);
}

UEField UEField::GetNext() const
{
	return UEField(*reinterpret_cast<void**>(Object + Off::UField::Next));
}

bool UEField::IsNextValid() const
{
	return (bool)GetNext();
}

std::vector<std::pair<FName, int64>> UEEnum::GetNameValuePairs() const
{
	struct alignas(0x4) Name08Byte { uint8 Pad[0x08]; };
	struct alignas(0x4) Name16Byte { uint8 Pad[0x10]; };

	std::vector<std::pair<FName, int64>> Ret;

	if (!Settings::Internal::bIsEnumNameOnly)
	{
		if (Settings::Internal::bUseCasePreservingName)
		{
			auto& Names = *reinterpret_cast<TArray<TPair<Name16Byte, int64>>*>(Object + Off::UEnum::Names);

			for (int i = 0; i < Names.Num(); i++)
			{
				Ret.push_back({ FName(&Names[i].First), Names[i].Second });
			}
		}
		else
		{
			auto& Names = *reinterpret_cast<TArray<TPair<Name08Byte, int64>>*>(Object + Off::UEnum::Names);

			for (int i = 0; i < Names.Num(); i++)
			{
				Ret.push_back({ FName(&Names[i].First), Names[i].Second });
			}
		}
	}
	else
	{
		auto& NameOnly = *reinterpret_cast<TArray<FName>*>(Object + Off::UEnum::Names);

		if (Settings::Internal::bUseCasePreservingName)
		{
			auto& Names = *reinterpret_cast<TArray<Name16Byte>*>(Object + Off::UEnum::Names);

			for (int i = 0; i < Names.Num(); i++)
			{
				Ret.push_back({ FName(&Names[i]), i });
			}
		}
		else
		{
			auto& Names = *reinterpret_cast<TArray<Name08Byte>*>(Object + Off::UEnum::Names);

			for (int i = 0; i < Names.Num(); i++)
			{
				Ret.push_back({ FName(&Names[i]), i });
			}
		}
	}

	return Ret;
}

std::string UEEnum::GetSingleName(int32 Index) const
{
	return GetNameValuePairs()[Index].first.ToString();
}

std::string UEEnum::GetEnumPrefixedName() const
{
	std::string Temp = GetValidName();

	return Temp[0] == 'E' ? Temp : 'E' + Temp;
}

std::string UEEnum::GetEnumTypeAsStr() const
{
	return "enum class " + GetEnumPrefixedName();
}

UEStruct UEStruct::GetSuper() const
{
	return UEStruct(*reinterpret_cast<void**>(Object + Off::UStruct::SuperStruct));
}

UEField UEStruct::GetChild() const
{
	return UEField(*reinterpret_cast<void**>(Object + Off::UStruct::Children));
}

UEFField UEStruct::GetChildProperties() const
{
	return UEFField(*reinterpret_cast<void**>(Object + Off::UStruct::ChildProperties));
}

int32 UEStruct::GetMinAlignment() const
{
	return *reinterpret_cast<int32*>(Object + Off::UStruct::MinAlignemnt);
}

int32 UEStruct::GetStructSize() const
{
	return *reinterpret_cast<int32*>(Object + Off::UStruct::Size);
}

std::vector<UEProperty> UEStruct::GetProperties() const
{
	std::vector<UEProperty> Properties;

	if (Settings::Internal::bUseFProperty)
	{
		for (UEFField Field = GetChildProperties(); Field; Field = Field.GetNext())
		{
			if (Field.IsA(EClassCastFlags::Property))
				Properties.push_back(Field.Cast<UEProperty>());
		}

		return Properties;
	}

	for (UEField Field = GetChild(); Field; Field = Field.GetNext())
	{
		if (Field.IsA(EClassCastFlags::Property))
			Properties.push_back(Field.Cast<UEProperty>());
	}

	return Properties;
}

std::vector<UEFunction> UEStruct::GetFunctions() const
{
	std::vector<UEFunction> Functions;

	for (UEField Field = GetChild(); Field; Field = Field.GetNext())
	{
		if (Field.IsA(EClassCastFlags::Function))
			Functions.push_back(Field.Cast<UEFunction>());
	}

	return Functions;
}

const TArray<uint8>& UEStruct::GetScriptBytes() const
{
	return *reinterpret_cast<const TArray<uint8>*>(Object + Off::UStruct::Script);
}

UEProperty UEStruct::FindMember(const std::string& MemberName, EClassCastFlags TypeFlags) const
{
	if (!Object)
		return nullptr;

	if (Settings::Internal::bUseFProperty)
	{
		for (UEFField Field = GetChildProperties(); Field; Field = Field.GetNext())
		{
			if (Field.IsA(TypeFlags) && Field.GetName() == MemberName)
			{
				return Field.Cast<UEProperty>();
			}
		}
	}

	for (UEField Field = GetChild(); Field; Field = Field.GetNext())
	{
		if (Field.IsA(TypeFlags) && Field.GetName() == MemberName)
		{
			return Field.Cast<UEProperty>();
		}
	}

	return nullptr;
}

bool UEStruct::HasMembers() const
{
	if (!Object)
		return false;

	if (Settings::Internal::bUseFProperty)
	{
		for (UEFField Field = GetChildProperties(); Field; Field = Field.GetNext())
		{
			if (Field.IsA(EClassCastFlags::Property))
				return true;
		}
	}
	else
	{
		for (UEField F = GetChild(); F; F = F.GetNext())
		{
			if (F.IsA(EClassCastFlags::Property))
				return true;
		}
	}

	return false;
}

EClassCastFlags UEClass::GetCastFlags() const
{
	return *reinterpret_cast<EClassCastFlags*>(Object + Off::UClass::CastFlags);
}

std::string UEClass::StringifyCastFlags() const
{
	return StringifyClassCastFlags(GetCastFlags());
}

bool UEClass::IsType(EClassCastFlags TypeFlag) const
{
	return (TypeFlag != EClassCastFlags::None ? (GetCastFlags() & TypeFlag) : true);
}

bool UEClass::HasType(UEClass TypeClass) const
{
	if (TypeClass == nullptr)
		return false;

	for (UEStruct S = *this; S; S = S.GetSuper())
	{
		if (S == TypeClass)
			return true;
	}

	return false;
}

UEObject UEClass::GetDefaultObject() const
{
	return UEObject(*reinterpret_cast<void**>(Object + Off::UClass::ClassDefaultObject));
}

UEFunction UEClass::GetFunction(const std::string& ClassName, const std::string& FuncName) const
{
	for (UEStruct Struct = *this; Struct; Struct = Struct.GetSuper())
	{
		if (Struct.GetName() != ClassName)
			continue;

		for (UEField Field = Struct.GetChild(); Field; Field = Field.GetNext())
		{
			if (Field.IsA(EClassCastFlags::Function) && Field.GetName() == FuncName)
			{
				return Field.Cast<UEFunction>();
			}	
		}

	}

	return nullptr;
}

EFunctionFlags UEFunction::GetFunctionFlags() const
{
	return *reinterpret_cast<EFunctionFlags*>(Object + Off::UFunction::FunctionFlags);
}

bool UEFunction::HasFlags(EFunctionFlags FuncFlags) const
{
	return GetFunctionFlags() & FuncFlags;
}

void* UEFunction::GetExecFunction() const
{
	return *reinterpret_cast<void**>(Object + Off::UFunction::ExecFunction);
}

UEProperty UEFunction::GetReturnProperty() const
{
	for (auto Prop : GetProperties())
	{
		if (Prop.HasPropertyFlags(EPropertyFlags::ReturnParm))
			return Prop;
	}

	return nullptr;
}


std::string UEFunction::StringifyFlags(const char* Seperator)  const
{
	return StringifyFunctionFlags(GetFunctionFlags(), Seperator);
}

std::string UEFunction::GetParamStructName() const
{
	return GetOuter().GetCppName() + "_" + GetValidName() + "_Params";
}

struct FScriptBytecodeReader
{
public:
	/* if (SCRIPT_LIMIT_BYTECODE_TO_64KB) CodeSkipSizeType = uint16 */
	using CodeSkipSizeType = uint32;

private:
	const TArray<uint8>& ScriptBytes;
	int32 CurrentPos;

public:
	FScriptBytecodeReader(const TArray<uint8>& Script, int32 StartIdx = 0)
		: ScriptBytes(Script), CurrentPos(StartIdx)
	{
	}

public:
	inline bool HasMoreInstructions() const
	{
		return CurrentPos < ScriptBytes.Num();
	}

public:
	inline void SkipBytes(int32 Count)
	{
		CurrentPos += Count;
	}

	template<typename T>
	inline T ReadAnyValue()
	{
		if ((CurrentPos + sizeof(T)) > ScriptBytes.Num())
			return T();

		T Ret = *reinterpret_cast<const T*>(&ScriptBytes[CurrentPos]);
		CurrentPos += sizeof(T);

		return Ret;
	}

	inline EExprToken ReadIntruction()
	{
		return ReadAnyValue<EExprToken>();
	}

	inline uint32 ReadCodeSkipCount()
	{
		return ReadAnyValue<CodeSkipSizeType>();
	}

	inline void* ReadPtr()
	{
		return ReadAnyValue<void*>();
	}

	inline UEProperty ReadProperty()
	{
		return UEProperty(ReadPtr());
	}

	inline UEObject ReadObject()
	{
		return UEObject(ReadPtr());
	}

	inline const FName ReadName()
	{
		const FName Name = FName(&ScriptBytes[CurrentPos]);

		/* sizeof(FScriptName) is always 0xC, it stors ComparisonIndex, DisplayIndex, Number (in this order) */
		CurrentPos += 0xC;

		return Name;
	}

	inline std::string ReadString()
	{
		std::string Str;

		for (int i = 0; i < 1024; i++)
		{
			char C = ReadAnyValue<char>();

			Str.push_back(C);

			if (C == '\0')
				break;
		}

		return Str;
	}

	inline std::wstring ReadUnicodeString()
	{
		std::wstring Str;

		for (int i = 0; i < 1024; i++)
		{
			wchar_t C = ReadAnyValue<wchar_t>();

			Str.push_back(C);

			if (C == L'\0')
				break;
		}

		return Str;
	}
};

std::string UEFunction::DisassembleInstruction(void* ByteCodeReader, bool bIsInitialOpcode) const
{
	static auto SetOpcodeNameIfEmpty = [](std::string& OpString, const char* OpcodeName)
	{
		if (OpString.empty())
			OpString += OpcodeName;
	};

	FScriptBytecodeReader& Reader = *reinterpret_cast<FScriptBytecodeReader*>(ByteCodeReader);

	EExprToken Instruction = Reader.ReadIntruction();

	std::string Ret;

	switch (Instruction)
	{
	case EExprToken::Cast:
	{
		uint8 CastType = Reader.ReadAnyValue<uint8>(); // 0x1
		const std::string SubExpression = DisassembleInstruction(&Reader); // Unknown-/Dynamic-Size

		Ret += std::format("Cast: SUBEXPR -> \"{}\"", SubExpression);
		break;
	}
	case EExprToken::ObjToInterfaceCast:
		SetOpcodeNameIfEmpty(Ret, "ObjToInterfaceCast"); [[fallthrough]];
	case EExprToken::CrossInterfaceCast:
		SetOpcodeNameIfEmpty(Ret, "CrossInterfaceCast"); [[fallthrough]];
	case EExprToken::InterfaceToObjCast:
	{
		const UEObject Object = Reader.ReadObject(); // 0x8, even on x32
		const std::string SubExpression = DisassembleInstruction(&Reader); // Unknown-/Dynamic-Size

		SetOpcodeNameIfEmpty(Ret, "InterfaceToObjCast");
		Ret += std::format(": OJB -> \"{}\", SUBEXPR -> {}", Object.GetName(), SubExpression);
		break;
	}
	case EExprToken::Let:
	{
		const UEProperty Property = Reader.ReadProperty(); // 0x8, even on x32

		Ret += std::format("Let: PROP -> \"{}\"", Property.GetName());
		break;
	}
	case EExprToken::LetObj:
		SetOpcodeNameIfEmpty(Ret, "LetObj"); [[fallthrough]];
	case EExprToken::LetWeakObjPtr:
		SetOpcodeNameIfEmpty(Ret, "LetWeakObjPtr"); [[fallthrough]];
	case EExprToken::LetBool:
		SetOpcodeNameIfEmpty(Ret, "LetBool"); [[fallthrough]];
	case EExprToken::LetDelegate:
		SetOpcodeNameIfEmpty(Ret, "LetDelegate"); [[fallthrough]];
	case EExprToken::LetMulticastDelegate:
	{
		const std::string GetVarExpr = DisassembleInstruction(&Reader); // Unknown-/Dynamic-Size
		const std::string AssignmentExpr = DisassembleInstruction(&Reader); // Unknown-/Dynamic-Size

		SetOpcodeNameIfEmpty(Ret, "LetMulticastDelegate");
		Ret += std::format(": GETVAR_EXPR -> \"{}\", ASSIGN_EXPR -> \"{}\"", GetVarExpr, AssignmentExpr);
		break;
	}
	case EExprToken::LetValueOnPersistentFrame:
	{
		const UEProperty Property = Reader.ReadProperty(); // 0x8, even on x32
		const std::string AssignmentExpr = DisassembleInstruction(&Reader); // Unknown-/Dynamic-Size

		Ret += std::format("LetValueOnPersistentFrame: PROP -> \"{}\", ASSIGN_EXPR -> \"{}\"", Property.GetName(), AssignmentExpr);
		break;
	}
	case EExprToken::StructMemberContext:
	{
		const UEProperty Property = Reader.ReadProperty(); // 0x8, even on x32
		const std::string AssignmentExpr = DisassembleInstruction(&Reader); // Unknown-/Dynamic-Size

		Ret += std::format("StructMemberContext: PROP -> \"{}\", ASSIGN_EXPR -> \"{}\"", Property.GetName(), AssignmentExpr);
		break;
	}
	case EExprToken::Jump:
	{
		const uint32 CodeSkipCount = Reader.ReadCodeSkipCount(); // 0x4 by default, 0x2 with SCRIPT_LIMIT_BYTECODE_TO_64KB

		Ret += std::format("Jump: SKIP_COUNT -> 0x{:X}", CodeSkipCount);
		break;
	}
	case EExprToken::ComputedJump:
	{
		const std::string CalcJmpOffsetExpr = DisassembleInstruction(&Reader); // Unknown-/Dynamic-Size

		Ret += std::format("ComputedJump: CALC_JMP_OFF_EXPR -> \"{}\"", CalcJmpOffsetExpr);
		break;
	}
	case EExprToken::LocalVariable:
		SetOpcodeNameIfEmpty(Ret, "LocalVariable"); [[fallthrough]];
	case EExprToken::InstanceVariable:
		SetOpcodeNameIfEmpty(Ret, "InstanceVariable"); [[fallthrough]];
	case EExprToken::DefaultVariable:
		SetOpcodeNameIfEmpty(Ret, "DefaultVariable"); [[fallthrough]];
	case EExprToken::LocalOutVariable:
		SetOpcodeNameIfEmpty(Ret, "LocalOutVariable"); [[fallthrough]];
	case EExprToken::ClassSparseDataVariable:
		SetOpcodeNameIfEmpty(Ret, "ClassSparseDataVariable"); [[fallthrough]];
	case EExprToken::PropertyConst:
	{
		const UEProperty Property = Reader.ReadProperty(); // 0x8, even on x32

		SetOpcodeNameIfEmpty(Ret, "PropertyConst");
		Ret += std::format(": PROP -> \"{}\"", Property.GetName());
		break;
	}
	case EExprToken::InterfaceContext:
	{
		const std::string GetInerfaceValueExpr = DisassembleInstruction(&Reader); // Unknown-/Dynamic-Size

		Ret += std::format("InterfaceContext: GET_VAL_EXPR -> \"{}\"", GetInerfaceValueExpr);
		break;
	}
	case EExprToken::PushExecutionFlow:
	{
		const uint32 CodeSkipCount = Reader.ReadCodeSkipCount(); // 0x4 by default, 0x2 with SCRIPT_LIMIT_BYTECODE_TO_64KB

		Ret += std::format("PushExecutionFlow: LOC_TO_PUSH -> 0x{:X}", CodeSkipCount);
		break;
	}
	case EExprToken::NothingInt32:
	{
		const uint32 NothingValue = Reader.ReadAnyValue<int32>(); // 0x4

		Ret += std::format("NothingInt32: NOTHING_VALUE -> 0x{:X}", NothingValue);
		break;
	}
	case EExprToken::Nothing:
	{
		Ret += "Nothing;";
		break;
	}
	case EExprToken::EndOfScript:
	{
		Ret += "EndOfScript;";
		break;
	}
	case EExprToken::EndFunctionParms:
	{
		Ret += "EndFunctionParms";
		break;
	}
	case EExprToken::EndStructConst:
	{
		Ret += "EndStructConst";
		break;
	}
	case EExprToken::EndArray:
	{
		Ret += "EndArray";
		break;
	}
	case EExprToken::EndArrayConst:
	{
		Ret += "EndArrayConst";
		break;
	}
	case EExprToken::EndSet:
	{
		Ret += "EndSet";
		break;
	}
	case EExprToken::EndMap:
	{
		Ret += "EndMap";
		break;
	}
	case EExprToken::EndSetConst:
	{
		Ret += "EndSetConst";
		break;
	}
	case EExprToken::EndMapConst:
	{
		Ret += "EndMapConst";
		break;
	}
	case EExprToken::EndParmValue:
	{
		Ret += "EndParmValue";
		break;
	}
	case EExprToken::DeprecatedOp4A:
	{
		Ret += "DeprecatedOp4A";
		break;
	}
	case EExprToken::IntZero:
	{
		Ret += "IntZero";
		break;
	}
	case EExprToken::IntOne:
	{
		Ret += "IntOne";
		break;
	}
	case EExprToken::True:
	{
		Ret += "True";
		break;
	}
	case EExprToken::False:
	{
		Ret += "False";
		break;
	}
	case EExprToken::NoObject:
	{
		Ret += "NoObject";
		break;
	}
	case EExprToken::NoInterface:
	{
		Ret += "NoInterface";
		break;
	}
	case EExprToken::Self:
	{
		Ret += "Self";
		break;
	}
	case EExprToken::PopExecutionFlow:
	{
		Ret += "PopExecutionFlow";
		break;
	}
	case EExprToken::WireTracepoint:
	{
		Ret += "WireTracepoint";
		break;
	}
	case EExprToken::Tracepoint:
	{
		Ret += "Tracepoint";
		break;
	}
	case EExprToken::Breakpoint:
	{
		Ret += "Breakpoint";
		break;
	}
	case EExprToken::InstrumentationEvent:
	{
		EScriptInstrumentation Event = Reader.ReadAnyValue<EScriptInstrumentation>(); // 0x1

		if (Event == EScriptInstrumentation::InlineEvent)
			Reader.SkipBytes(Off::InSDK::Name::FNameSize); // 0x4/0x8/0xC

		Reader.SkipBytes(0x1); // 0x1

		Ret += "InstrumentationEvent";
		break;
	}
	case EExprToken::Return:
	{
		Ret += DisassembleInstruction(&Reader);  // Unknown-/Dynamic-Size
		break;
	}
	case EExprToken::CallMath:
		SetOpcodeNameIfEmpty(Ret, "CallMath"); [[fallthrough]];
	case EExprToken::LocalFinalFunction:
		SetOpcodeNameIfEmpty(Ret, "LocalFinalFunction"); [[fallthrough]];
	case EExprToken::FinalFunction:
	{
		const UEObject Function = Reader.ReadObject();
		/* UEs Serializer now disassembles the code until it hits a "EndFunction" opcode. We're doing nothing, as subsequent disassembling operations will take care of this. */

		SetOpcodeNameIfEmpty(Ret, "FinalFunction");
		Ret += std::format(": FUNC -> \"{}\"", Function.GetName());
		break;
	}
	case EExprToken::LocalVirtualFunction:
		SetOpcodeNameIfEmpty(Ret, "LocalVirtualFunction"); [[fallthrough]];
	case EExprToken::VirtualFunction:
	{
		const FName VFuncName = Reader.ReadName(); // sizeof(FName) (0x4/0x8/0xC)

		SetOpcodeNameIfEmpty(Ret, "VirtualFunction");
		Ret += std::format(": CALLED_VFUNC -> \"{}\"", VFuncName.ToString());
		break;
	}
	case EExprToken::CallMulticastDelegate:
	{
		const UEObject Object = Reader.ReadObject(); // 0x8

		Ret += std::format("CallMulticastDelegate: DELEGATE -> \"{}\"", Object.GetName());
		break;
	}
	case EExprToken::ClassContext:
		SetOpcodeNameIfEmpty(Ret, "ClassContext"); [[fallthrough]];
	case EExprToken::Context:
		SetOpcodeNameIfEmpty(Ret, "Context"); [[fallthrough]];
	case EExprToken::Context_FailSilent:
	{
		const std::string OjbExpr = DisassembleInstruction(&Reader); // Unknown/Dynamic
		const uint32 SkipOffset = Reader.ReadCodeSkipCount(); // 0x2/0x4
		const UEProperty DataProperty = Reader.ReadProperty(); // 0x8
		const std::string ContextExpr = DisassembleInstruction(&Reader); // Unknown/Dynamic

		SetOpcodeNameIfEmpty(Ret, "Context_FailSilent");
		Ret += std::format(": OBJ_EXPR -> \"{}\", DATA_PROP -> \"{}\", CONTEXT_EXPR -> \"{}\"", OjbExpr, DataProperty.GetName(), ContextExpr);
		break;
	}
	case EExprToken::AddMulticastDelegate:
		SetOpcodeNameIfEmpty(Ret, "AddMulticastDelegate"); [[fallthrough]];
	case EExprToken::RemoveMulticastDelegate:
	{
		const std::string GetTargetExpr = DisassembleInstruction(&Reader); // Unknown/Dynamic
		const std::string GetSourceExpr = DisassembleInstruction(&Reader); // Unknown/Dynamic

		SetOpcodeNameIfEmpty(Ret, "RemoveMulticastDelegate");
		Ret += std::format(": TARGET_EXPR -> \"{}\", SOURCE_EXPR -> \"{}\"", GetTargetExpr, GetSourceExpr);
		break;
	}
	case EExprToken::ClearMulticastDelegate:
	{
		const std::string GetTargetExpr = DisassembleInstruction(&Reader); // Unknown/Dynamic

		Ret += std::format("ClearMulticastDelegate: TARGET_EXPR -> \"{}\"", GetTargetExpr);
		break;
	}
	case EExprToken::IntConst:
	{
		const int32 IntegerConstant = Reader.ReadAnyValue<int32>(); // 0x4

		Ret += std::format("IntConst: CONST_INT -> 0x{:X}", IntegerConstant);
		break;
	}
	case EExprToken::Int64Const:
	{
		const int64 IntegerConstant = Reader.ReadAnyValue<int64>(); // 0x8

		Ret += std::format("Int64Const: CONST_INT64 -> 0x{:X}", IntegerConstant);
		break;
	}
	case EExprToken::UInt64Const:
	{
		const uint64 IntegerConstant = Reader.ReadAnyValue<uint64>(); // 0x8

		Ret += std::format("UInt64Const: CONST_UINT64 -> 0x{:X}", IntegerConstant);
		break;
	}
	case EExprToken::SkipOffsetConst:
	{
		const uint32 IntegerConstant = Reader.ReadCodeSkipCount(); // 0x2/0x4

		Ret += std::format("SkipOffsetConst: SKIP_VAL -> 0x{:X}", IntegerConstant);
		break;
	}
	case EExprToken::FloatConst:
	{
		const float FloatConstant = Reader.ReadAnyValue<float>(); // 0x4

		Ret += std::format("FloatConst: FLOAT: -> {}", FloatConstant);
		break;
	}
	case EExprToken::DoubleConst:
	{
		const double DoubleConstant = Reader.ReadAnyValue<double>(); // 0x4

		Ret += std::format("DoubleConst: DOUBLE -> {}", DoubleConstant);
		break;
	}
	case EExprToken::StringConst:
	{
		const std::string String = Reader.ReadString(); // Dynamic size

		Ret += std::format("StringConst: STR -> \"{}\"", String);
		break;
	}
	case EExprToken::UnicodeStringConst:
	{
		const std::wstring String = Reader.ReadUnicodeString(); // Dynamic size

		//Ret += std::format("UnicodeStringConst: WSTR -> \"{}\"", String);
		Ret += std::format("UnicodeStringConst: WSTR -> \"Idk how to print a wstring, help!\"");
		break;
	}
	case EExprToken::TextConst:
	{
		EBlueprintTextLiteralType TextType = Reader.ReadAnyValue<EBlueprintTextLiteralType>();

		switch (TextType)
		{
		case EBlueprintTextLiteralType::LocalizedText:
		{
			const std::string Source = DisassembleInstruction(&Reader); // Unknown/Dynamic
			const std::string Text = DisassembleInstruction(&Reader); // Unknown/Dynamic
			const std::string Namespace = DisassembleInstruction(&Reader); // Unknown/Dynamic

			Ret += std::format("TextConst [Localized]: SRC: \"{}\", TXT: \"{}\", NAMESPACE: \"{}\"", Source, Text, Namespace);
			break;
		}
		case EBlueprintTextLiteralType::InvariantText:
			SetOpcodeNameIfEmpty(Ret, "InvariantText"); [[fallthrough]];
		case EBlueprintTextLiteralType::LiteralString:
		{
			const std::string StringLiteral = DisassembleInstruction(&Reader); // Unknown/Dynamic

			SetOpcodeNameIfEmpty(Ret, "LiteralString");
			Ret += std::format(": STR: \"{}\"", StringLiteral);
			break;
		}
		case EBlueprintTextLiteralType::StringTableEntry:
		{
			Reader.SkipBytes(0x8); // sizeof(ScriptPointerType) always 0x8 (even on x32)
			const std::string Expr1 = DisassembleInstruction(&Reader); // Unknown/Dynamic
			const std::string Expr2 = DisassembleInstruction(&Reader); // Unknown/Dynamic

			Ret += std::format("TextConst [StringTableEntry]: EXPR1: \"{}\", EXPR2: \"{}\"", Expr1, Expr2);
			break;
		}
		default:
			Ret += "TextConst [UNK/EMPTY]";
			break;
		}

		break;
	}
	case EExprToken::ObjectConst:
	{
		const UEObject Object = Reader.ReadObject();

		Ret += std::format("ObjectConst: OBJ -> \"{}\"", Object.GetName());
		break;
	}
	case EExprToken::SoftObjectConst:
	{
		const std::string StrExpr = DisassembleInstruction(&Reader); // Unknown/Dynamic

		Ret += std::format("SoftObjectConst: STR_EXPR -> \"{}\"", StrExpr);
		break;
	}
	case EExprToken::FieldPathConst:
	{
		const std::string SubExpr = DisassembleInstruction(&Reader); // Unknown/Dynamic

		Ret += std::format("FieldPathConst: SUB_EXPR -> \"{}\"", SubExpr);
		break;
	}
	case EExprToken::NameConst:
	{
		const FName Name = Reader.ReadName(); // sizeof(FName) 0x4/0x8/0xC

		Ret += std::format("NameConst: NAME -> \"{}\"", Name.ToString());
		break;
	}
	case EExprToken::RotationConst:
	{
		if (Settings::Internal::bUseLargeWorldCoordinates)
		{
			double Pitch = Reader.ReadAnyValue<double>(); // 0x8
			double Yaw   = Reader.ReadAnyValue<double>(); // 0x8
			double Roll  = Reader.ReadAnyValue<double>(); // 0x8

			Ret += std::format("VectorConst: PITCH -> {}, YAW -> {}, ROLL -> {}", Pitch, Yaw, Roll);
		}
		else
		{
			float Pitch = Reader.ReadAnyValue<float>(); // 0x4
			float Yaw   = Reader.ReadAnyValue<float>(); // 0x4
			float Roll  = Reader.ReadAnyValue<float>(); // 0x4

			Ret += std::format("VectorConst: PITCH -> {}, YAW -> {}, ROLL -> {}", Pitch, Yaw, Roll);
		}

		break;
	}
	case EExprToken::VectorConst:
	{
		if (Settings::Internal::bUseLargeWorldCoordinates)
		{
			double X = Reader.ReadAnyValue<double>(); // 0x8
			double Y = Reader.ReadAnyValue<double>(); // 0x8
			double Z = Reader.ReadAnyValue<double>(); // 0x8

			Ret += std::format("VectorConst: X -> {}, Y -> {}, Z -> {}", X, Y, Z);
		}
		else
		{
			float X = Reader.ReadAnyValue<float>(); // 0x4
			float Y = Reader.ReadAnyValue<float>(); // 0x4
			float Z = Reader.ReadAnyValue<float>(); // 0x4

			Ret += std::format("VectorConst: X -> {}, Y -> {}, Z -> {}", X, Y, Z);
		}

		break;
	}
	case EExprToken::Vector3fConst:
	{
		float X = Reader.ReadAnyValue<float>(); // 0x4
		float Y = Reader.ReadAnyValue<float>(); // 0x4
		float Z = Reader.ReadAnyValue<float>(); // 0x4

		Ret += std::format("Vector3fConst: X -> {}, Y -> {}, Z -> {}", X, Y, Z);
		break;
	}
	case EExprToken::TransformConst:
	{
		if (Settings::Internal::bUseLargeWorldCoordinates)
		{
			/* FQuat */
			double Rotation_X = Reader.ReadAnyValue<double>(); // 0x8
			double Rotation_Y = Reader.ReadAnyValue<double>(); // 0x8
			double Rotation_Z = Reader.ReadAnyValue<double>(); // 0x8
			double Rotation_W = Reader.ReadAnyValue<double>(); // 0x8

			/* FVector */
			double Translation_X = Reader.ReadAnyValue<double>(); // 0x8
			double Translation_Y = Reader.ReadAnyValue<double>(); // 0x8
			double Translation_Z = Reader.ReadAnyValue<double>(); // 0x8

			/* FVector */
			double Scale_X = Reader.ReadAnyValue<double>(); // 0x8
			double Scale_Y = Reader.ReadAnyValue<double>(); // 0x8
			double Scale_Z = Reader.ReadAnyValue<double>(); // 0x8

			Ret += std::format("TransformConst: ROT -> {{ {}, {}, {}, {} }}, TRANS -> {{ {}, {}, {} }}, SCALE -> {{ {}, {}, {} }}",
				Rotation_X, Rotation_Y, Rotation_Z, Rotation_W, Translation_X, Translation_Y, Translation_Z, Scale_X, Scale_Y, Scale_Z);
		}
		else
		{
			/* FQuat */
			float Rotation_X = Reader.ReadAnyValue<float>(); // 0x4
			float Rotation_Y = Reader.ReadAnyValue<float>(); // 0x4
			float Rotation_Z = Reader.ReadAnyValue<float>(); // 0x4
			float Rotation_W = Reader.ReadAnyValue<float>(); // 0x4

			/* FVector */
			float Translation_X = Reader.ReadAnyValue<float>(); // 0x4
			float Translation_Y = Reader.ReadAnyValue<float>(); // 0x4
			float Translation_Z = Reader.ReadAnyValue<float>(); // 0x4

			/* FVector */
			float Scale_X = Reader.ReadAnyValue<float>(); // 0x4
			float Scale_Y = Reader.ReadAnyValue<float>(); // 0x4
			float Scale_Z = Reader.ReadAnyValue<float>(); // 0x4

			Ret += std::format("TransformConst: ROT -> {{ {}, {}, {} , {} }}, TRANS -> {{ {}, {}, {} }}, SCALE -> {{ {}, {}, {} }}",
				Rotation_X, Rotation_Y, Rotation_Z, Rotation_W, Translation_X, Translation_Y, Translation_Z, Scale_X, Scale_Y, Scale_Z);
		}

		break;
	}
	case EExprToken::StructConst:
	{
		UEObject Struct = Reader.ReadObject(); // 0x8
		const int32 StructSize = Reader.ReadAnyValue<int32>(); // 0x4
		// Ignore while(!Opcode == StructEnd) DisassembleInstruction(). Following instructions are likely diassembled anyways.

		Ret += std::format("StructConst: STRUCT -> \"{}\", SIZE -> 0x{:X}", Struct.GetName(), StructSize);
		break;
	}
	case EExprToken::SetArray:
	{
		const std::string TargetSubExpr = DisassembleInstruction(&Reader);

		Ret += std::format("SetArray: SUB_EXPR -> \"{}\"", TargetSubExpr);
		break;
	}
	case EExprToken::SetSet:
	{
		const std::string TargetSubExpr = DisassembleInstruction(&Reader);
		const int32 NumElements = Reader.ReadAnyValue<int32>(); // 0x4

		Ret += std::format("SetSet: SUB_EXPR -> \"{}\", NUM_ELEM -> 0x{:X}", TargetSubExpr, NumElements);
		break;
	}
	case EExprToken::SetMap:
	{
		const std::string TargetSubExpr = DisassembleInstruction(&Reader);
		const int32 NumElements = Reader.ReadAnyValue<int32>(); // 0x4

		Ret += std::format("SetMap: SUB_EXPR -> \"{}\", NUM_ELEM -> 0x{:X}", TargetSubExpr, NumElements);
		break;
	}
	case EExprToken::ArrayConst:
	{
		const UEProperty ArrayProperty = Reader.ReadProperty(); // 0x8
		const int32 NumElements = Reader.ReadAnyValue<int32>(); // 0x4

		Ret += std::format("ArrayConst: ARR_PROP -> \"{}\", NUM_ELEM -> 0x{:X}", ArrayProperty.GetName(), NumElements);
		break;
	}
	case EExprToken::SetConst:
	{
		const UEProperty SetProperty = Reader.ReadProperty(); // 0x8
		const int32 NumElements = Reader.ReadAnyValue<int32>(); // 0x4

		Ret += std::format("SetConst: SET_PROP -> \"{}\", NUM_ELEM -> 0x{:X}", SetProperty.GetName(), NumElements);
		break;
	}
	case EExprToken::MapConst:
	{
		const UEProperty KeyProperty = Reader.ReadProperty(); // 0x8
		const UEProperty ValueProperty = Reader.ReadProperty(); // 0x8
		const int32 NumElements = Reader.ReadAnyValue<int32>(); // 0x4

		Ret += std::format("MapConst: KEY_PROP -> \"{}\", VAL_PROP -> \"{}\", NUM_ELEM -> 0x{:X}", KeyProperty.GetName(), ValueProperty.GetName(), NumElements);
		break;
	}
	case EExprToken::BitFieldConst:
	{
		const UEProperty BitProperty = Reader.ReadProperty(); // 0x8
		const uint8 BitValue = Reader.ReadAnyValue<uint8>(); // 0x1

		Ret += std::format("BitFieldConst: BIT_PROP -> \"{}\", BIT_VAL -> 0x{:X}", BitProperty.GetName(), BitValue);
		break;
	}
	case EExprToken::ByteConst:
		SetOpcodeNameIfEmpty(Ret, "ByteConst"); [[fallthrough]];
	case EExprToken::IntConstByte:
	{
		const uint8 ByteValue = Reader.ReadAnyValue<uint8>(); // 0x1

		SetOpcodeNameIfEmpty(Ret, "IntConstByte");
		Ret += std::format(": BYTE_VAL -> 0x{:X}", ByteValue);
		break;
	}
	case EExprToken::MetaCast:
	{
		const UEObject NewClass = Reader.ReadObject(); // 0x8
		const std::string SourceExpr = DisassembleInstruction(&Reader);

		Ret += std::format("MetaCast: NEW_CLSS -> \"{}\", SRC_EXPR -> \"{}\"", NewClass.GetName(), SourceExpr);
		break;
	}
	case EExprToken::DynamicCast:
	{
		const UEObject NewClass = Reader.ReadObject(); // 0x8
		const std::string SourceExpr = DisassembleInstruction(&Reader); // Uk/Dyn

		Ret += std::format("DynamicCast: NEW_CLSS -> \"{}\", SRC_EXPR -> \"{}\"", NewClass.GetName(), SourceExpr);
		break;
	}
	case EExprToken::JumpIfNot:
	{
		const uint32 SkipSize = Reader.ReadCodeSkipCount(); // 0x2/0x4
		const std::string Condition = DisassembleInstruction(&Reader); // Uk/Dyn

		Ret += std::format("JumpIfNot: SKIP_SIZE -> 0x{:X}, CONDITION_EXPR -> \"{}\"", SkipSize, Condition);
		break;
	}
	case EExprToken::PopExecutionFlowIfNot:
	{
		const std::string FlowLocExpr = DisassembleInstruction(&Reader); // Uk/Dyn

		Ret += std::format("PopExecutionFlowIfNot: LOCATION_EXPR -> \"{}\"", FlowLocExpr);
		break;
	}
	case EExprToken::Assert:
	{
		const uint16 LineNumber = Reader.ReadAnyValue<uint16>(); // 0x2
		const uint8 bIsDebug = Reader.ReadAnyValue<uint8>(); // 0x1

		const std::string AssertionValueExpr = DisassembleInstruction(&Reader); // Uk/Dyn

		Ret += std::format("Assert: LINE -> 0x{:X}, ISDEBUG -> 0x{:X}, ASSERT_VAL_EXPR -> \"{}\"", LineNumber, bIsDebug, AssertionValueExpr);
		break;
	}
	case EExprToken::Skip:
	{
		const uint32 SkipSize = Reader.ReadCodeSkipCount(); // 0x2/0x4
		const std::string ExprToSkip = DisassembleInstruction(&Reader); // Uk/Dyn

		Ret += std::format("Skip: SKIP_SIZE -> 0x{:X}, SKIP_EXPR -> \"{}\"", SkipSize, ExprToSkip);
		break;
	}
	case EExprToken::InstanceDelegate:
	{
		const FName DelegateFuncName = Reader.ReadName(); // sizeof(FName) 0x4/0x8/0xC

		Ret += std::format("InstanceDelegate: FUNC_NAME -> \"{}\"", DelegateFuncName.ToString());
		break;
	}
	case EExprToken::BindDelegate:
	{
		const FName DelegateFuncName = Reader.ReadName(); // sizeof(FName) 0x4/0x8/0xC
		const std::string TargetExpr = DisassembleInstruction(&Reader); // Uk/Dyn
		const std::string SourceExpr = DisassembleInstruction(&Reader); // Uk/Dyn

		Ret += std::format("InstanceDelegate: FUNC_NAME -> \"{}\", TARGET_EXPR -> \"{}\", SRC_EXPR -> \"{}\"", DelegateFuncName.ToString(), TargetExpr, SourceExpr);
		break;
	}
	case EExprToken::SwitchValue:
	{
		const uint16 NumCases = Reader.ReadAnyValue<uint16>(); // 0x2
		const uint32 OffsetToEnd = Reader.ReadCodeSkipCount(); // 0x2/0x4

		const std::string SwitchValueExpr = DisassembleInstruction(&Reader); // Uk/Dyn

		for (int16 i = 0; i < NumCases; i++)
		{
			const std::string CaseIndexExpr = DisassembleInstruction(&Reader); // Uk/Dyn
			const uint32 OffsetToNextCase   = Reader.ReadCodeSkipCount();      // 0x2/0x4
			const std::string CaseTermExpr  = DisassembleInstruction(&Reader); // Uk/Dyn

			// Ignore for now
		}

		const std::string DefaultValueExpr = DisassembleInstruction(&Reader); // Uk/Dyn

		Ret += std::format("SwitchValue: NUM_CASES -> 0x{:X}, OFF_TO_END -> 0x{:X}, DEFAULT_VAL_EXPR -> \"{}\"", NumCases, OffsetToEnd, DefaultValueExpr);
		break;
	}
	case EExprToken::ArrayGetByRef:
	{
		const std::string TargetArrayExpr = DisassembleInstruction(&Reader); // Uk/Dyn
		const std::string TargetIndexExpr = DisassembleInstruction(&Reader); // Uk/Dyn

		Ret += std::format("SwitchValue: TARGET_ARRAY_EXPR -> \"{}\", TARGET_IDX_EXPR -> \"{}\"", TargetArrayExpr, TargetIndexExpr);
		break;
	}
	case EExprToken::AutoRtfmTransact:
	{
		const int32 TransationId = Reader.ReadAnyValue<int32>(); // 0x4
		const uint32 JumpOffset = Reader.ReadCodeSkipCount();    // 0x2/0x4

		Ret += std::format("AutoRtfmTransact: TRANS_ID -> 0x{:X}, JMP_OFFSET ->0x{:X}", TransationId, JumpOffset);
		break;
	}
	case EExprToken::AutoRtfmStopTransact:
	{
		const int32 TransationId = Reader.ReadAnyValue<int32>(); // 0x4
		const int8 StopMode = Reader.ReadAnyValue<int8>();    // 0x1

		Ret += std::format("AutoRtfmTransact: TRANS_ID -> 0x{:X}, JMP_OFFSET ->0x{:X}", TransationId, StopMode);
		break;
	}
	case EExprToken::AutoRtfmAbortIfNot:
	{
		const std::string BoolExpr = DisassembleInstruction(&Reader); // Uk/Dyn

		Ret += std::format("AutoRtfmTransact: BOOL_EXPR -> \"{}\"", BoolExpr);
		break;
	}
	default:
		Ret += "INVALID_OPCODE";
	}

	/* Debug print befor adding the new line */
	std::cout << Ret << std::endl;

	if (bIsInitialOpcode)
		Ret += '\n';

	return Ret;
}

std::string UEFunction::DumpScriptBytecode() const
{
	std::string Ret;

	const TArray<uint8>& ByteCode = GetScriptBytes();

	FScriptBytecodeReader Reader(ByteCode);

	while (Reader.HasMoreInstructions())
	{
		Ret += DisassembleInstruction(&Reader, true);
	}

	return Ret;
}

void* UEProperty::GetAddress()
{
	return Base;
}

std::pair<UEClass, UEFFieldClass> UEProperty::GetClass() const
{
	if (Settings::Internal::bUseFProperty)
	{
		return { UEClass(0), UEFField(Base).GetClass() };
	}

	return { UEObject(Base).GetClass(), UEFFieldClass(0) };
}

EClassCastFlags UEProperty::GetCastFlags() const
{
	auto [Class, FieldClass] = GetClass();
	
	return Class ? Class.GetCastFlags() : FieldClass.GetCastFlags();
}

UEProperty::operator bool() const
{
	return Base != nullptr && ((Base + Off::UObject::Class) != nullptr || (Base + Off::FField::Class) != nullptr);
}


bool UEProperty::IsA(EClassCastFlags TypeFlags) const
{
	if (GetClass().first)
		return GetClass().first.IsType(TypeFlags);
	
	return GetClass().second.IsType(TypeFlags);
}

FName UEProperty::GetFName() const
{
	if (Settings::Internal::bUseFProperty)
	{
		return FName(Base + Off::FField::Name); //Not the real FName, but a wrapper which holds the address of a FName
	}

	return FName(Base + Off::UObject::Name); //Not the real FName, but a wrapper which holds the address of a FName
}

int32 UEProperty::GetArrayDim() const
{
	return *reinterpret_cast<int32*>(Base + Off::Property::ArrayDim);
}

int32 UEProperty::GetSize() const
{
	return *reinterpret_cast<int32*>(Base + Off::Property::ElementSize);
}

int32 UEProperty::GetOffset() const
{
	return *reinterpret_cast<int32*>(Base + Off::Property::Offset_Internal);
}

EPropertyFlags UEProperty::GetPropertyFlags() const
{
	return *reinterpret_cast<EPropertyFlags*>(Base + Off::Property::PropertyFlags);
}

bool UEProperty::HasPropertyFlags(EPropertyFlags PropertyFlag) const
{
	return GetPropertyFlags() & PropertyFlag;
}

bool UEProperty::IsType(EClassCastFlags PossibleTypes) const
{
	return (static_cast<uint64>(GetCastFlags()) & static_cast<uint64>(PossibleTypes)) != 0;
}

std::string UEProperty::GetName() const
{
	return Base ? GetFName().ToString() : "None";
}

std::string UEProperty::GetValidName() const
{
	return Base ? GetFName().ToValidString() : "None";
}

int32 UEProperty::GetAlignment() const
{
	EClassCastFlags TypeFlags = (GetClass().first ? GetClass().first.GetCastFlags() : GetClass().second.GetCastFlags());

	if (TypeFlags & EClassCastFlags::ByteProperty)
	{
		return alignof(uint8); // 0x1
	}
	else if (TypeFlags & EClassCastFlags::UInt16Property)
	{
		return alignof(uint16); // 0x2
	}
	else if (TypeFlags & EClassCastFlags::UInt32Property)
	{
		return alignof(uint32); // 0x4
	}
	else if (TypeFlags & EClassCastFlags::UInt64Property)
	{
		return alignof(uint64); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::Int8Property)
	{
		return alignof(int8); // 0x1
	}
	else if (TypeFlags & EClassCastFlags::Int16Property)
	{
		return alignof(int16); // 0x2
	}
	else if (TypeFlags & EClassCastFlags::IntProperty)
	{
		return alignof(int32); // 0x4
	}
	else if (TypeFlags & EClassCastFlags::Int64Property)
	{
		return alignof(int64); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::FloatProperty)
	{
		return alignof(float); // 0x4
	}
	else if (TypeFlags & EClassCastFlags::DoubleProperty)
	{
		return alignof(double); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::ClassProperty)
	{
		return alignof(void*); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::NameProperty)
	{
		return 0x4; // FName is a bunch of int32s
	}
	else if (TypeFlags & EClassCastFlags::StrProperty)
	{
		return alignof(FString); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::TextProperty)
	{
		return 0x8; // alignof member FString
	}
	else if (TypeFlags & EClassCastFlags::BoolProperty)
	{
		return alignof(bool); // 0x1
	}
	else if (TypeFlags & EClassCastFlags::StructProperty)
	{
		return Cast<UEStructProperty>().GetUnderlayingStruct().GetMinAlignment();
	}
	else if (TypeFlags & EClassCastFlags::ArrayProperty)
	{
		return alignof(TArray<int>); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::DelegateProperty)
	{
		return alignof(int32); // 0x4
	}
	else if (TypeFlags & EClassCastFlags::WeakObjectProperty)
	{
		return 0x4; // TWeakObjectPtr is a bunch of int32s
	}
	else if (TypeFlags & EClassCastFlags::LazyObjectProperty)
	{
		return 0x4; // TLazyObjectPtr is a bunch of int32s
	}
	else if (TypeFlags & EClassCastFlags::SoftClassProperty)
	{
		return 0x8; // alignof member FString
	}
	else if (TypeFlags & EClassCastFlags::SoftObjectProperty)
	{
		return 0x8; // alignof member FString
	}
	else if (TypeFlags & EClassCastFlags::ObjectProperty)
	{
		return alignof(void*); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::MapProperty)
	{
		return alignof(TArray<int>); // 0x8, TMap contains a TArray
	}
	else if (TypeFlags & EClassCastFlags::SetProperty)
	{
		return alignof(TArray<int>); // 0x8, TSet contains a TArray
	}
	else if (TypeFlags & EClassCastFlags::EnumProperty)
	{
		UEProperty P = Cast<UEEnumProperty>().GetUnderlayingProperty();

		return P ? P.GetAlignment(): 0x1;
	}
	else if (TypeFlags & EClassCastFlags::InterfaceProperty)
	{
		return alignof(void*); // 0x8
	}
	else if (TypeFlags & EClassCastFlags::FieldPathProperty)
	{
		return 0x8; // alignof member TArray<FName> and ptr;
	}
	else if (TypeFlags & EClassCastFlags::MulticastSparseDelegateProperty)
	{
		return 0x1; // size in PropertyFixup (alignment isn't greater than size)
	}
	else if (TypeFlags & EClassCastFlags::MulticastInlineDelegateProperty)
	{
		return 0x8;  // alignof member TArray<FName>
	}
	else if (TypeFlags & EClassCastFlags::OptionalProperty)
	{
		UEProperty ValueProperty = Cast<UEOptionalProperty>().GetValueProperty();

		/* If this check is true it means, that there is no bool in this TOptional to check if the value is set */
		if (ValueProperty.GetSize() == GetSize()) [[unlikely]]
			return ValueProperty.GetAlignment();

		return  GetSize() - ValueProperty.GetSize();
	}
	
	if (Settings::Internal::bUseFProperty)
	{
		static std::unordered_map<void*, int32> UnknownProperties;

		static auto TryFindPropertyRefInOptionalToGetAlignment = [](std::unordered_map<void*, int32>& OutProperties, void* PropertyClass) -> int32
		{
			/* Search for a TOptionalProperty that contains an instance of this property */
			for (UEObject Obj : ObjectArray())
			{
				if (!Obj.IsA(EClassCastFlags::Struct))
					continue;

				for (UEProperty Prop : Obj.Cast<UEStruct>().GetProperties())
				{
					if (!Prop.IsA(EClassCastFlags::OptionalProperty))
						continue;

					UEOptionalProperty Optional = Prop.Cast<UEOptionalProperty>();

					/* Safe to use first member, as we're guaranteed to use FProperty */
					if (Optional.GetValueProperty().GetClass().second.GetAddress() == PropertyClass)
						return OutProperties.insert({ PropertyClass, Optional.GetAlignment() }).first->second;
				}
			}

			return OutProperties.insert({ PropertyClass, 0x1 }).first->second;
		};

		auto It = UnknownProperties.find(GetClass().second.GetAddress());

		/* Safe to use first member, as we're guaranteed to use FProperty */
		if (It == UnknownProperties.end())
			return TryFindPropertyRefInOptionalToGetAlignment(UnknownProperties, GetClass().second.GetAddress());

		return It->second;
	}

	return 0x1;
}

std::string UEProperty::GetCppType() const
{
	EClassCastFlags TypeFlags = (GetClass().first ? GetClass().first.GetCastFlags() : GetClass().second.GetCastFlags());

	if (TypeFlags & EClassCastFlags::ByteProperty)
	{
		return Cast<UEByteProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::UInt16Property)
	{
		return "uint16";
	}
	else if (TypeFlags &  EClassCastFlags::UInt32Property)
	{
		return "uint32";
	}
	else if (TypeFlags &  EClassCastFlags::UInt64Property)
	{
		return "uint64";
	}
	else if (TypeFlags & EClassCastFlags::Int8Property)
	{
		return "int8";
	}
	else if (TypeFlags &  EClassCastFlags::Int16Property)
	{
		return "int16";
	}
	else if (TypeFlags &  EClassCastFlags::IntProperty)
	{
		return "int32";
	}
	else if (TypeFlags &  EClassCastFlags::Int64Property)
	{
		return "int64";
	}
	else if (TypeFlags &  EClassCastFlags::FloatProperty)
	{
		return "float";
	}
	else if (TypeFlags &  EClassCastFlags::DoubleProperty)
	{
		return "double";
	}
	else if (TypeFlags &  EClassCastFlags::ClassProperty)
	{
		return Cast<UEClassProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::NameProperty)
	{
		return "class FName";
	}
	else if (TypeFlags &  EClassCastFlags::StrProperty)
	{
		return "class FString";
	}
	else if (TypeFlags & EClassCastFlags::TextProperty)
	{
		return "class FText";
	}
	else if (TypeFlags &  EClassCastFlags::BoolProperty)
	{
		return Cast<UEBoolProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::StructProperty)
	{
		return Cast<UEStructProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::ArrayProperty)
	{
		return Cast<UEArrayProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::WeakObjectProperty)
	{
		return Cast<UEWeakObjectProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::LazyObjectProperty)
	{
		return Cast<UELazyObjectProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::SoftClassProperty)
	{
		return Cast<UESoftClassProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::SoftObjectProperty)
	{
		return Cast<UESoftObjectProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::ObjectProperty)
	{
		return Cast<UEObjectProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::MapProperty)
	{
		return Cast<UEMapProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::SetProperty)
	{
		return Cast<UESetProperty>().GetCppType();
	}
	else if (TypeFlags &  EClassCastFlags::EnumProperty)
	{
		return Cast<UEEnumProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::InterfaceProperty) 
	{
		return Cast<UEInterfaceProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::FieldPathProperty)
	{
		return Cast<UEFieldPathProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::DelegateProperty)
	{
		return Cast<UEDelegateProperty>().GetCppType();
	}
	else if (TypeFlags & EClassCastFlags::OptionalProperty)
	{
	return Cast<UEOptionalProperty>().GetCppType();
	}
	else
	{
		return (GetClass().first ? GetClass().first.GetCppName() : GetClass().second.GetCppName()) + "_";;
	}
}

std::string UEProperty::StringifyFlags() const
{
	return StringifyPropertyFlags(GetPropertyFlags());
}

UEEnum UEByteProperty::GetEnum() const
{
	return UEEnum(*reinterpret_cast<void**>(Base + Off::ByteProperty::Enum));
}

std::string UEByteProperty::GetCppType() const
{
	if (UEEnum Enum = GetEnum())
	{
		return Enum.GetEnumTypeAsStr();
	}

	return "uint8";
}

uint8 UEBoolProperty::GetFieldMask() const
{
	return reinterpret_cast<Off::BoolProperty::UBoolPropertyBase*>(Base + Off::BoolProperty::Base)->FieldMask;
}

uint8 UEBoolProperty::GetBitIndex() const
{
	uint8 FieldMask = GetFieldMask();
	
	if (FieldMask != 0xFF)
	{
		if (FieldMask == 0x01) { return 0; }
		if (FieldMask == 0x02) { return 1; }
		if (FieldMask == 0x04) { return 2; }
		if (FieldMask == 0x08) { return 3; }
		if (FieldMask == 0x10) { return 4; }
		if (FieldMask == 0x20) { return 5; }
		if (FieldMask == 0x40) { return 6; }
		if (FieldMask == 0x80) { return 7; }
	}

	return 0xFF;
}

bool UEBoolProperty::IsNativeBool() const
{
	return reinterpret_cast<Off::BoolProperty::UBoolPropertyBase*>(Base + Off::BoolProperty::Base)->FieldMask == 0xFF;
}

std::string UEBoolProperty::GetCppType() const
{
	return IsNativeBool() ? "bool" : "uint8";
}

UEClass UEObjectProperty::GetPropertyClass() const
{
	return UEClass(*reinterpret_cast<void**>(Base + Off::ObjectProperty::PropertyClass));
}

std::string UEObjectProperty::GetCppType() const
{
	return std::format("class {}*", GetPropertyClass() ? GetPropertyClass().GetCppName() : "UObject");
}

UEClass UEClassProperty::GetMetaClass() const
{
	return UEClass(*reinterpret_cast<void**>(Base + Off::ClassProperty::MetaClass));
}

std::string UEClassProperty::GetCppType() const
{
	return HasPropertyFlags(EPropertyFlags::UObjectWrapper) ? std::format("TSubclassOf<class {}>", GetMetaClass().GetCppName()) : "class UClass*";
}

std::string UEWeakObjectProperty::GetCppType() const
{
	return std::format("TWeakObjectPtr<class {}>", GetPropertyClass() ? GetPropertyClass().GetCppName() : "UObject");
}

std::string UELazyObjectProperty::GetCppType() const
{
	return std::format("TLazyObjectPtr<class {}>", GetPropertyClass() ? GetPropertyClass().GetCppName() : "UObject");
}

std::string UESoftObjectProperty::GetCppType() const
{
	return std::format("TSoftObjectPtr<class {}>", GetPropertyClass() ? GetPropertyClass().GetCppName() : "UObject");
}

std::string UESoftClassProperty::GetCppType() const
{
	return std::format("TSoftClassPtr<class {}>", GetMetaClass() ? GetMetaClass().GetCppName() : GetPropertyClass().GetCppName());
}

std::string UEInterfaceProperty::GetCppType() const
{
	return std::format("TScriptInterface<class {}>", GetPropertyClass().GetCppName());
}

UEStruct UEStructProperty::GetUnderlayingStruct() const
{
	return UEStruct(*reinterpret_cast<void**>(Base + Off::StructProperty::Struct));
}

std::string UEStructProperty::GetCppType() const
{
	return std::format("struct {}", GetUnderlayingStruct().GetCppName());
}

UEProperty UEArrayProperty::GetInnerProperty() const
{
	return UEProperty(*reinterpret_cast<void**>(Base + Off::ArrayProperty::Inner));
}

std::string UEArrayProperty::GetCppType() const
{
	return std::format("TArray<{}>", GetInnerProperty().GetCppType());
}

UEFunction UEDelegateProperty::GetSignatureFunction() const
{
	return UEFunction(*reinterpret_cast<void**>(Base + Off::DelegateProperty::SignatureFunction));
}

std::string UEDelegateProperty::GetCppType() const
{
	return "TDeleage<GetCppTypeIsNotImplementedForDelegates>";
}

UEProperty UEMapProperty::GetKeyProperty() const
{
	return UEProperty(reinterpret_cast<Off::MapProperty::UMapPropertyBase*>(Base + Off::MapProperty::Base)->KeyProperty);
}

UEProperty UEMapProperty::GetValueProperty() const
{
	return UEProperty(reinterpret_cast<Off::MapProperty::UMapPropertyBase*>(Base + Off::MapProperty::Base)->ValueProperty);
}

std::string UEMapProperty::GetCppType() const
{
	return std::format("TMap<{}, {}>", GetKeyProperty().GetCppType(), GetValueProperty().GetCppType());
}

UEProperty UESetProperty::GetElementProperty() const
{
	return UEProperty(*reinterpret_cast<void**>(Base + Off::SetProperty::ElementProp));
}

std::string UESetProperty::GetCppType() const
{
	return std::format("TSet<{}>", GetElementProperty().GetCppType());
}

UEProperty UEEnumProperty::GetUnderlayingProperty() const
{
	return UEProperty(reinterpret_cast<Off::EnumProperty::UEnumPropertyBase*>(Base + Off::EnumProperty::Base)->UnderlayingProperty);
}

UEEnum UEEnumProperty::GetEnum() const
{
	return UEEnum(reinterpret_cast<Off::EnumProperty::UEnumPropertyBase*>(Base + Off::EnumProperty::Base)->Enum);
}

std::string UEEnumProperty::GetCppType() const
{
	if (GetEnum())
		return GetEnum().GetEnumTypeAsStr();

	return GetUnderlayingProperty().GetCppType();
}

UEFFieldClass UEFieldPathProperty::GetFielClass() const
{
	return UEFFieldClass(*reinterpret_cast<void**>(Base + Off::FieldPathProperty::FieldClass));
}

std::string UEFieldPathProperty::GetCppType() const
{
	return std::format("TFieldPath<struct {}>", GetFielClass().GetCppName());
}

UEProperty UEOptionalProperty::GetValueProperty() const
{
	return UEProperty(*reinterpret_cast<void**>(Base + Off::OptionalProperty::ValueProperty));
}

std::string UEOptionalProperty::GetCppType() const
{
	return std::format("TOptional<{}>", GetValueProperty().GetCppType());
}

