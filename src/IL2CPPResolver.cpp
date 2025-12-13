#include "../include/IL2CPPResolver.h"
#include <Psapi.h>
namespace IL2CPPResolver {
    ModuleResolver::ModuleResolver()
        : gameAssemblyModule(nullptr), baseAddress(0), moduleSize(0), initialized(false) {
    }

    ModuleResolver::~ModuleResolver() {
    }

    bool ModuleResolver::Initialize() {
        if (initialized) {
            return true;
        }

        gameAssemblyModule = GetModuleHandleA("GameAssembly.dll");
        if (!gameAssemblyModule) {
            return false;
        }

        baseAddress = reinterpret_cast<uintptr_t>(gameAssemblyModule);

        MODULEINFO moduleInfo;
        if (!GetModuleInformation(GetCurrentProcess(), gameAssemblyModule, &moduleInfo, sizeof(MODULEINFO))) {
            return false;
        }

        moduleSize = moduleInfo.SizeOfImage;
        CacheExportedFunctions();

        initialized = true;
        return true;
    }

    bool ModuleResolver::IsInitialized() const {
        return initialized;
    }

    HMODULE ModuleResolver::GetModuleHandle() const {
        return gameAssemblyModule;
    }

    uintptr_t ModuleResolver::GetBaseAddress() const {
        return baseAddress;
    }

    size_t ModuleResolver::GetModuleSize() const {
        return moduleSize;
    }

    uintptr_t ModuleResolver::GetExportedFunction(const std::string& functionName) {
        auto it = exportedFunctions.find(functionName);
        if (it != exportedFunctions.end()) {
            return it->second;
        }

        FARPROC funcAddress = GetProcAddress(gameAssemblyModule, functionName.c_str());
        if (funcAddress) {
            uintptr_t address = reinterpret_cast<uintptr_t>(funcAddress);
            exportedFunctions[functionName] = address;
            return address;
        }

        return 0;
    }

    uintptr_t ModuleResolver::FindPattern(const uint8_t* pattern, const char* mask, uintptr_t startAddress) {
        if (startAddress == 0) {
            startAddress = baseAddress;
        }

        size_t patternLength = strlen(mask);
        uintptr_t endAddress = baseAddress + moduleSize - patternLength;

        for (uintptr_t current = startAddress; current < endAddress; current++) {
            if (ComparePattern(reinterpret_cast<const uint8_t*>(current), pattern, mask)) {
                return current;
            }
        }

        return 0;
    }

    uintptr_t ModuleResolver::ResolveRelativeAddress(uintptr_t instructionAddress, int32_t instructionSize) {
        int32_t relativeOffset = *reinterpret_cast<int32_t*>(instructionAddress);
        return instructionAddress + instructionSize + relativeOffset;
    }

    void ModuleResolver::CacheExportedFunctions() {
        exportedFunctions.clear();

        IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(baseAddress);
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
            return;
        }

        IMAGE_NT_HEADERS* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(baseAddress + dosHeader->e_lfanew);
        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) {
            return;
        }

        IMAGE_EXPORT_DIRECTORY* exportDirectory = reinterpret_cast<IMAGE_EXPORT_DIRECTORY*>(
            baseAddress + ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
            );

        if (!exportDirectory) {
            return;
        }

        DWORD* nameTable = reinterpret_cast<DWORD*>(baseAddress + exportDirectory->AddressOfNames);
        DWORD* addressTable = reinterpret_cast<DWORD*>(baseAddress + exportDirectory->AddressOfFunctions);
        WORD* ordinalTable = reinterpret_cast<WORD*>(baseAddress + exportDirectory->AddressOfNameOrdinals);

        for (DWORD i = 0; i < exportDirectory->NumberOfNames; i++) {
            const char* functionName = reinterpret_cast<const char*>(baseAddress + nameTable[i]);
            WORD ordinal = ordinalTable[i];
            uintptr_t functionAddress = baseAddress + addressTable[ordinal];

            exportedFunctions[functionName] = functionAddress;
        }
    }

    bool ModuleResolver::ComparePattern(const uint8_t* data, const uint8_t* pattern, const char* mask) {
        for (; *mask; ++mask, ++data, ++pattern) {
            if (*mask == 'x' && *data != *pattern) {
                return false;
            }
        }
        return true;
    }

    DomainResolver::DomainResolver()
        : getDomainFunc(nullptr), getAssembliesFunc(nullptr), currentDomain(nullptr) {
    }

    DomainResolver::~DomainResolver() {
    }

    bool DomainResolver::Initialize(ModuleResolver& moduleResolver) {
        uintptr_t getDomainAddress = moduleResolver.GetExportedFunction("il2cpp_domain_get");
        if (!getDomainAddress) {
            return false;
        }

        uintptr_t getAssembliesAddress = moduleResolver.GetExportedFunction("il2cpp_domain_get_assemblies");
        if (!getAssembliesAddress) {
            return false;
        }

        getDomainFunc = reinterpret_cast<GetDomainFunc>(getDomainAddress);
        getAssembliesFunc = reinterpret_cast<GetAssembliesFunc>(getAssembliesAddress);

        currentDomain = getDomainFunc();
        if (!currentDomain) {
            return false;
        }

        return true;
    }

    Il2CppDomain* DomainResolver::GetDomain() {
        return currentDomain;
    }

    std::vector<Il2CppAssembly*> DomainResolver::GetAssemblies() {
        if (assemblies.empty() && currentDomain && getAssembliesFunc) {
            size_t assemblyCount = 0;
            Il2CppAssembly** assemblyArray = getAssembliesFunc(currentDomain, &assemblyCount);

            assemblies.clear();
            for (size_t i = 0; i < assemblyCount; i++) {
                assemblies.push_back(assemblyArray[i]);
            }
        }

        return assemblies;
    }

    Il2CppAssembly* DomainResolver::GetAssembly(const std::string& assemblyName) {
        auto allAssemblies = GetAssemblies();

        for (auto assembly : allAssemblies) {
            if (assembly && assembly->image && assembly->image->name) {
                std::string currentName = assembly->image->name;
                if (currentName == assemblyName || currentName == assemblyName + ".dll") {
                    return assembly;
                }
            }
        }

        return nullptr;
    }

    ImageResolver::ImageResolver()
        : getClassFromNameFunc(nullptr), getClassCountFunc(nullptr), getClassFunc(nullptr) {
    }

    ImageResolver::~ImageResolver() {
    }

    bool ImageResolver::Initialize(ModuleResolver& moduleResolver) {
        uintptr_t getClassFromNameAddress = moduleResolver.GetExportedFunction("il2cpp_class_from_name");
        if (!getClassFromNameAddress) {
            return false;
        }

        uintptr_t getClassCountAddress = moduleResolver.GetExportedFunction("il2cpp_image_get_class_count");
        if (!getClassCountAddress) {
            return false;
        }

        uintptr_t getClassAddress = moduleResolver.GetExportedFunction("il2cpp_image_get_class");
        if (!getClassAddress) {
            return false;
        }

        getClassFromNameFunc = reinterpret_cast<GetClassFromNameFunc>(getClassFromNameAddress);
        getClassCountFunc = reinterpret_cast<GetClassCountFunc>(getClassCountAddress);
        getClassFunc = reinterpret_cast<GetClassFunc>(getClassAddress);

        return true;
    }

    Il2CppClass* ImageResolver::GetClassFromName(const Il2CppImage* image, const std::string& namespaze, const std::string& className) {
        if (!image || !getClassFromNameFunc) {
            return nullptr;
        }

        return getClassFromNameFunc(image, namespaze.c_str(), className.c_str());
    }

    std::vector<const Il2CppClass*> ImageResolver::GetAllClasses(const Il2CppImage* image) {
        std::vector<const Il2CppClass*> classes;

        if (!image || !getClassCountFunc || !getClassFunc) {
            return classes;
        }

        size_t classCount = getClassCountFunc(image);
        for (size_t i = 0; i < classCount; i++) {
            const Il2CppClass* klass = getClassFunc(image, i);
            if (klass) {
                classes.push_back(klass);
            }
        }

        return classes;
    }

    ClassResolver::ClassResolver()
        : getMethodFromNameFunc(nullptr), getFieldFromNameFunc(nullptr),
        getPropertyFromNameFunc(nullptr), getParentFunc(nullptr), getMethodsFunc(nullptr) {
    }

    ClassResolver::~ClassResolver() {
    }

    bool ClassResolver::Initialize(ModuleResolver& moduleResolver) {
        uintptr_t getMethodFromNameAddress = moduleResolver.GetExportedFunction("il2cpp_class_get_method_from_name");
        if (!getMethodFromNameAddress) {
            return false;
        }

        uintptr_t getFieldFromNameAddress = moduleResolver.GetExportedFunction("il2cpp_class_get_field_from_name");
        if (!getFieldFromNameAddress) {
            return false;
        }

        uintptr_t getPropertyFromNameAddress = moduleResolver.GetExportedFunction("il2cpp_class_get_property_from_name");
        if (!getPropertyFromNameAddress) {
            return false;
        }

        uintptr_t getParentAddress = moduleResolver.GetExportedFunction("il2cpp_class_get_parent");
        if (!getParentAddress) {
            return false;
        }

        uintptr_t getMethodsAddress = moduleResolver.GetExportedFunction("il2cpp_class_get_methods");
        if (!getMethodsAddress) {
            return false;
        }

        getMethodFromNameFunc = reinterpret_cast<GetMethodFromNameFunc>(getMethodFromNameAddress);
        getFieldFromNameFunc = reinterpret_cast<GetFieldFromNameFunc>(getFieldFromNameAddress);
        getPropertyFromNameFunc = reinterpret_cast<GetPropertyFromNameFunc>(getPropertyFromNameAddress);
        getParentFunc = reinterpret_cast<GetParentFunc>(getParentAddress);
        getMethodsFunc = reinterpret_cast<GetMethodsFunc>(getMethodsAddress);

        return true;
    }

    MethodInfo* ClassResolver::GetMethodFromName(Il2CppClass* klass, const std::string& methodName, int parameterCount) {
        if (!klass || !getMethodFromNameFunc) {
            return nullptr;
        }

        return getMethodFromNameFunc(klass, methodName.c_str(), parameterCount);
    }

    FieldInfo* ClassResolver::GetFieldFromName(Il2CppClass* klass, const std::string& fieldName) {
        if (!klass || !getFieldFromNameFunc) {
            return nullptr;
        }

        return getFieldFromNameFunc(klass, fieldName.c_str());
    }

    PropertyInfo* ClassResolver::GetPropertyFromName(Il2CppClass* klass, const std::string& propertyName) {
        if (!klass || !getPropertyFromNameFunc) {
            return nullptr;
        }

        return getPropertyFromNameFunc(klass, propertyName.c_str());
    }

    Il2CppClass* ClassResolver::GetParent(Il2CppClass* klass) {
        if (!klass || !getParentFunc) {
            return nullptr;
        }

        return getParentFunc(klass);
    }

    std::vector<const MethodInfo*> ClassResolver::GetAllMethods(Il2CppClass* klass) {
        std::vector<const MethodInfo*> methods;

        if (!klass || !getMethodsFunc) {
            return methods;
        }

        void* iterator = nullptr;
        size_t methodCount = 0;
        const MethodInfo* method = getMethodsFunc(klass, &iterator, &methodCount);

        while (method) {
            methods.push_back(method);
            method = getMethodsFunc(klass, &iterator, &methodCount);
        }

        return methods;
    }

    MethodResolver::MethodResolver()
        : runtimeInvokeFunc(nullptr), objectNewFunc(nullptr) {
    }

    MethodResolver::~MethodResolver() {
    }

    bool MethodResolver::Initialize(ModuleResolver& moduleResolver) {
        uintptr_t runtimeInvokeAddress = moduleResolver.GetExportedFunction("il2cpp_runtime_invoke");
        if (!runtimeInvokeAddress) {
            return false;
        }

        uintptr_t objectNewAddress = moduleResolver.GetExportedFunction("il2cpp_object_new");
        if (!objectNewAddress) {
            return false;
        }

        runtimeInvokeFunc = reinterpret_cast<RuntimeInvokeFunc>(runtimeInvokeAddress);
        objectNewFunc = reinterpret_cast<ObjectNewFunc>(objectNewAddress);

        return true;
    }

    void* MethodResolver::RuntimeInvoke(const MethodInfo* method, void* obj, void** params) {
        if (!method || !runtimeInvokeFunc) {
            return nullptr;
        }

        void* exception = nullptr;
        return runtimeInvokeFunc(method, obj, params, &exception);
    }

    Il2CppObject* MethodResolver::CreateInstance(Il2CppClass* klass) {
        if (!klass || !objectNewFunc) {
            return nullptr;
        }

        return objectNewFunc(klass);
    }

    StringResolver::StringResolver()
        : newStringFunc(nullptr), newStringLenFunc(nullptr) {
    }

    StringResolver::~StringResolver() {
    }

    bool StringResolver::Initialize(ModuleResolver& moduleResolver) {
        uintptr_t newStringAddress = moduleResolver.GetExportedFunction("il2cpp_string_new");
        if (!newStringAddress) {
            return false;
        }

        uintptr_t newStringLenAddress = moduleResolver.GetExportedFunction("il2cpp_string_new_len");
        if (!newStringLenAddress) {
            return false;
        }

        newStringFunc = reinterpret_cast<NewStringFunc>(newStringAddress);
        newStringLenFunc = reinterpret_cast<NewStringLenFunc>(newStringLenAddress);

        return true;
    }

    Il2CppString* StringResolver::CreateString(const std::wstring& str) {
        if (!newStringFunc) {
            return nullptr;
        }

        return newStringFunc(str.c_str());
    }

    Il2CppString* StringResolver::CreateString(const wchar_t* str, uint32_t length) {
        if (!newStringLenFunc) {
            return nullptr;
        }

        return newStringLenFunc(str, length);
    }

    std::wstring StringResolver::ToString(Il2CppString* str) {
        if (!str) {
            return L"";
        }

        return std::wstring(str->chars, str->length);
    }

    ObfuscationHandler::ObfuscationHandler()
        : isObfuscated(false) {
    }

    ObfuscationHandler::~ObfuscationHandler() {
    }

    bool ObfuscationHandler::DetectObfuscation(ModuleResolver& moduleResolver) {
        uint8_t pattern1[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
        const char* mask1 = "xxxxx";

        uint8_t pattern2[] = { 0xFF, 0xFF, 0xFF };
        const char* mask2 = "xxx";

        uintptr_t result1 = moduleResolver.FindPattern(pattern1, mask1);
        uintptr_t result2 = moduleResolver.FindPattern(pattern2, mask2);

        isObfuscated = (result1 != 0 || result2 != 0);
        return isObfuscated;
    }

    bool ObfuscationHandler::BuildNameMappings(DomainResolver& domainResolver, ImageResolver& imageResolver) {
        auto assemblies = domainResolver.GetAssemblies();

        for (auto assembly : assemblies) {
            if (!assembly || !assembly->image) {
                continue;
            }

            auto classes = imageResolver.GetAllClasses(assembly->image);
            for (auto klass : classes) {
                if (!klass || !klass->name) {
                    continue;
                }

                std::string className = klass->name;
                if (IsObfuscatedName(className)) {
                    std::string deobfuscated = DeobfuscateName(className);
                    AddClassMapping(className, deobfuscated, reinterpret_cast<uintptr_t>(klass));
                }
            }
        }

        return true;
    }

    std::string ObfuscationHandler::ResolveClassName(const std::string& obfuscatedName) {
        auto it = classNameMappings.find(obfuscatedName);
        if (it != classNameMappings.end()) {
            return it->second.originalName;
        }
        return obfuscatedName;
    }

    std::string ObfuscationHandler::ResolveMethodName(const std::string& obfuscatedName) {
        auto it = methodNameMappings.find(obfuscatedName);
        if (it != methodNameMappings.end()) {
            return it->second.originalName;
        }
        return obfuscatedName;
    }

    std::string ObfuscationHandler::ResolveFieldName(const std::string& obfuscatedName) {
        auto it = fieldNameMappings.find(obfuscatedName);
        if (it != fieldNameMappings.end()) {
            return it->second.originalName;
        }
        return obfuscatedName;
    }

    bool ObfuscationHandler::IsObfuscated() const {
        return isObfuscated;
    }

    void ObfuscationHandler::AddClassMapping(const std::string& obfuscated, const std::string& original, uintptr_t address) {
        NameMapping mapping;
        mapping.obfuscatedName = obfuscated;
        mapping.originalName = original;
        mapping.address = address;
        classNameMappings[obfuscated] = mapping;
    }

    void ObfuscationHandler::AddMethodMapping(const std::string& obfuscated, const std::string& original, uintptr_t address) {
        NameMapping mapping;
        mapping.obfuscatedName = obfuscated;
        mapping.originalName = original;
        mapping.address = address;
        methodNameMappings[obfuscated] = mapping;
    }

    void ObfuscationHandler::AddFieldMapping(const std::string& obfuscated, const std::string& original, uintptr_t address) {
        NameMapping mapping;
        mapping.obfuscatedName = obfuscated;
        mapping.originalName = original;
        mapping.address = address;
        fieldNameMappings[obfuscated] = mapping;
    }

    bool ObfuscationHandler::IsObfuscatedName(const std::string& name) {
        if (name.empty()) {
            return false;
        }

        bool hasOnlyHex = true;
        bool hasRandomChars = false;
        int shortNameThreshold = 3;

        for (char c : name) {
            if (!isxdigit(c) && c != '_') {
                hasOnlyHex = false;
            }
            if (!isalnum(c) && c != '_') {
                hasRandomChars = true;
            }
        }

        if (name.length() <= shortNameThreshold && hasOnlyHex) {
            return true;
        }

        if (hasRandomChars) {
            return true;
        }

        return false;
    }

    std::string ObfuscationHandler::DeobfuscateName(const std::string& obfuscatedName) {
        return "Deobfuscated_" + obfuscatedName;
    }

    IL2CPPInternalResolver::IL2CPPInternalResolver()
        : initialized(false) {
    }

    IL2CPPInternalResolver::~IL2CPPInternalResolver() {
    }

    bool IL2CPPInternalResolver::Initialize() {
        if (initialized) {
            return true;
        }

        if (!moduleResolver.Initialize()) {
            return false;
        }

        if (!domainResolver.Initialize(moduleResolver)) {
            return false;
        }

        if (!imageResolver.Initialize(moduleResolver)) {
            return false;
        }

        if (!classResolver.Initialize(moduleResolver)) {
            return false;
        }

        if (!methodResolver.Initialize(moduleResolver)) {
            return false;
        }

        if (!stringResolver.Initialize(moduleResolver)) {
            return false;
        }

        obfuscationHandler.DetectObfuscation(moduleResolver);
        if (obfuscationHandler.IsObfuscated()) {
            obfuscationHandler.BuildNameMappings(domainResolver, imageResolver);
        }

        initialized = true;
        return true;
    }

    bool IL2CPPInternalResolver::IsInitialized() const {
        return initialized;
    }

    Il2CppDomain* IL2CPPInternalResolver::GetDomain() {
        return domainResolver.GetDomain();
    }

    std::vector<Il2CppAssembly*> IL2CPPInternalResolver::GetAssemblies() {
        return domainResolver.GetAssemblies();
    }

    Il2CppAssembly* IL2CPPInternalResolver::GetAssembly(const std::string& assemblyName) {
        return domainResolver.GetAssembly(assemblyName);
    }

    Il2CppImage* IL2CPPInternalResolver::GetImage(const std::string& assemblyName) {
        Il2CppAssembly* assembly = GetAssembly(assemblyName);
        return assembly ? assembly->image : nullptr;
    }

    Il2CppClass* IL2CPPInternalResolver::GetClass(const std::string& assemblyName, const std::string& namespaze, const std::string& className) {
        Il2CppImage* image = GetImage(assemblyName);
        if (!image) {
            return nullptr;
        }

        return imageResolver.GetClassFromName(image, namespaze, className);
    }

    MethodInfo* IL2CPPInternalResolver::GetMethod(Il2CppClass* klass, const std::string& methodName, int parameterCount) {
        return classResolver.GetMethodFromName(klass, methodName, parameterCount);
    }

    FieldInfo* IL2CPPInternalResolver::GetField(Il2CppClass* klass, const std::string& fieldName) {
        return classResolver.GetFieldFromName(klass, fieldName);
    }

    PropertyInfo* IL2CPPInternalResolver::GetProperty(Il2CppClass* klass, const std::string& propertyName) {
        return classResolver.GetPropertyFromName(klass, propertyName);
    }

    void* IL2CPPInternalResolver::InvokeMethod(const MethodInfo* method, void* instance, void** args) {
        return methodResolver.RuntimeInvoke(method, instance, args);
    }

    Il2CppObject* IL2CPPInternalResolver::CreateInstance(const std::string& assemblyName, const std::string& namespaze, const std::string& className) {
        Il2CppClass* klass = GetClass(assemblyName, namespaze, className);
        if (!klass) {
            return nullptr;
        }

        return methodResolver.CreateInstance(klass);
    }

    Il2CppString* IL2CPPInternalResolver::CreateString(const std::wstring& str) {
        return stringResolver.CreateString(str);
    }

    std::wstring IL2CPPInternalResolver::ToString(Il2CppString* str) {
        return stringResolver.ToString(str);
    }

    ModuleResolver& IL2CPPInternalResolver::GetModuleResolver() {
        return moduleResolver;
    }

    DomainResolver& IL2CPPInternalResolver::GetDomainResolver() {
        return domainResolver;
    }

    ImageResolver& IL2CPPInternalResolver::GetImageResolver() {
        return imageResolver;
    }

    ClassResolver& IL2CPPInternalResolver::GetClassResolver() {
        return classResolver;
    }

    MethodResolver& IL2CPPInternalResolver::GetMethodResolver() {
        return methodResolver;
    }

    StringResolver& IL2CPPInternalResolver::GetStringResolver() {
        return stringResolver;
    }

    ObfuscationHandler& IL2CPPInternalResolver::GetObfuscationHandler() {
        return obfuscationHandler;
    }

}