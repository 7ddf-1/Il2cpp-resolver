#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
namespace IL2CPPResolver {

    struct MethodInfo;
    struct FieldInfo;
    struct PropertyInfo;
    struct Il2CppClass;
    struct Il2CppImage;
    struct Il2CppAssembly;
    struct Il2CppDomain;
    struct Il2CppType;
    struct Il2CppObject;
    struct Il2CppString;

    typedef void* (*Il2CppMethodPointer)(void*, void*, void*);

    struct Il2CppType {
        void* data;
        uint16_t attrs;
        uint8_t type;
        uint8_t numMods;
        uint8_t byRef;
        uint8_t pinned;
    };

    struct MethodInfo {
        Il2CppMethodPointer methodPointer;
        void* invokerMethod;
        const char* name;
        Il2CppClass* klass;
        const Il2CppType* returnType;
        const void* parameters;
        void* rgctxData;
        void* genericData;
        uint32_t token;
        uint16_t flags;
        uint16_t iflags;
        uint16_t slot;
        uint8_t parametersCount;
        uint8_t isGeneric;
        uint8_t isInflated;
    };

    struct FieldInfo {
        const char* name;
        const Il2CppType* type;
        Il2CppClass* parent;
        int32_t offset;
        uint32_t token;
    };

    struct PropertyInfo {
        Il2CppClass* parent;
        const char* name;
        const MethodInfo* get;
        const MethodInfo* set;
        uint32_t attrs;
        uint32_t token;
    };

    struct Il2CppClass {
        const void* image;
        void* gcDesc;
        const char* name;
        const char* namespaze;
        Il2CppType byvalArg;
        Il2CppType thisArg;
        Il2CppClass* elementClass;
        Il2CppClass* castClass;
        Il2CppClass* declaringType;
        Il2CppClass* parent;
        void* genericClass;
        void* typeMetadataHandle;
        void* interopData;
        Il2CppClass* klass;
        FieldInfo* fields;
        void* events;
        PropertyInfo* properties;
        MethodInfo** methods;
        Il2CppClass** nestedTypes;
        Il2CppClass** implementedInterfaces;
        void* interfaceOffsets;
        void* staticFields;
        void* rgctxData;
        Il2CppClass** typeHierarchy;
        uint32_t instanceSize;
        uint32_t actualSize;
        uint32_t elementSize;
        int32_t nativeSize;
        uint32_t staticFieldsSize;
        uint32_t threadStaticFieldsSize;
        int32_t threadStaticFieldsOffset;
        uint32_t flags;
        uint32_t token;
        uint16_t methodCount;
        uint16_t propertyCount;
        uint16_t fieldCount;
        uint16_t eventCount;
        uint16_t nestedTypesCount;
        uint16_t vtableCount;
        uint16_t interfacesCount;
        uint16_t interfaceOffsetsCount;
        uint8_t typeHierarchyDepth;
        uint8_t genericRecursionDepth;
        uint8_t rank;
        uint8_t minimumAlignment;
        uint8_t naturalAlignment;
        uint8_t packingSize;
        uint8_t initialized;
        uint8_t enumtype;
        uint8_t isGeneric;
        uint8_t hasReferences;
        uint8_t hasStaticRefs;
        uint8_t hasException;
        uint8_t hasFinalize;
        uint8_t hasCctor;
        uint8_t isBlittable;
        uint8_t isImport;
    };

    struct Il2CppImage {
        const char* name;
        const char* nameNoExt;
        Il2CppAssembly* assembly;
        uint32_t typeStart;
        uint32_t typeCount;
        uint32_t exportedTypeStart;
        uint32_t exportedTypeCount;
        uint32_t customAttributeStart;
        uint32_t customAttributeCount;
        uint32_t entryPointIndex;
        void* nameToClassHashTable;
        uint32_t token;
        uint8_t dynamic;
    };

    struct Il2CppAssembly {
        Il2CppImage* image;
        uint32_t token;
        int32_t referencedAssemblyStart;
        int32_t referencedAssemblyCount;
        void* aname;
    };

    struct Il2CppDomain {
        Il2CppAssembly** assemblies;
        uint32_t assemblyCount;
        void* assembliesMap;
        void* codeGenModules;
        void* typeAssertions;
        void* typeAssertionsMap;
        void* agentInfo;
    };

    struct Il2CppObject {
        Il2CppClass* klass;
        void* monitor;
    };

    struct Il2CppString {
        Il2CppObject object;
        int32_t length;
        wchar_t chars[1];
    };

    class ModuleResolver {
    private:
        HMODULE gameAssemblyModule;
        uintptr_t baseAddress;
        size_t moduleSize;
        std::unordered_map<std::string, uintptr_t> exportedFunctions;
        bool initialized;

    public:
        ModuleResolver();
        ~ModuleResolver();

        bool Initialize();
        bool IsInitialized() const;
        HMODULE GetModuleHandle() const;
        uintptr_t GetBaseAddress() const;
        size_t GetModuleSize() const;
        uintptr_t GetExportedFunction(const std::string& functionName);
        uintptr_t FindPattern(const uint8_t* pattern, const char* mask, uintptr_t startAddress = 0);
        uintptr_t ResolveRelativeAddress(uintptr_t instructionAddress, int32_t instructionSize);

    private:
        void CacheExportedFunctions();
        bool ComparePattern(const uint8_t* data, const uint8_t* pattern, const char* mask);
    };

    class DomainResolver {
    private:
        typedef Il2CppDomain* (*GetDomainFunc)();
        typedef Il2CppAssembly** (*GetAssembliesFunc)(const Il2CppDomain*, size_t*);

        GetDomainFunc getDomainFunc;
        GetAssembliesFunc getAssembliesFunc;
        Il2CppDomain* currentDomain;
        std::vector<Il2CppAssembly*> assemblies;

    public:
        DomainResolver();
        ~DomainResolver();

        bool Initialize(ModuleResolver& moduleResolver);
        Il2CppDomain* GetDomain();
        std::vector<Il2CppAssembly*> GetAssemblies();
        Il2CppAssembly* GetAssembly(const std::string& assemblyName);
    };

    class ImageResolver {
    private:
        typedef Il2CppClass* (*GetClassFromNameFunc)(const Il2CppImage*, const char*, const char*);
        typedef size_t(*GetClassCountFunc)(const Il2CppImage*);
        typedef const Il2CppClass* (*GetClassFunc)(const Il2CppImage*, size_t);

        GetClassFromNameFunc getClassFromNameFunc;
        GetClassCountFunc getClassCountFunc;
        GetClassFunc getClassFunc;

    public:
        ImageResolver();
        ~ImageResolver();

        bool Initialize(ModuleResolver& moduleResolver);
        Il2CppClass* GetClassFromName(const Il2CppImage* image, const std::string& namespaze, const std::string& className);
        std::vector<const Il2CppClass*> GetAllClasses(const Il2CppImage* image);
    };

    class ClassResolver {
    private:
        typedef MethodInfo* (*GetMethodFromNameFunc)(Il2CppClass*, const char*, int);
        typedef FieldInfo* (*GetFieldFromNameFunc)(Il2CppClass*, const char*);
        typedef PropertyInfo* (*GetPropertyFromNameFunc)(Il2CppClass*, const char*);
        typedef Il2CppClass* (*GetParentFunc)(Il2CppClass*);
        typedef const MethodInfo* (*GetMethodsFunc)(Il2CppClass*, void**, size_t*);

        GetMethodFromNameFunc getMethodFromNameFunc;
        GetFieldFromNameFunc getFieldFromNameFunc;
        GetPropertyFromNameFunc getPropertyFromNameFunc;
        GetParentFunc getParentFunc;
        GetMethodsFunc getMethodsFunc;

    public:
        ClassResolver();
        ~ClassResolver();

        bool Initialize(ModuleResolver& moduleResolver);
        MethodInfo* GetMethodFromName(Il2CppClass* klass, const std::string& methodName, int parameterCount);
        FieldInfo* GetFieldFromName(Il2CppClass* klass, const std::string& fieldName);
        PropertyInfo* GetPropertyFromName(Il2CppClass* klass, const std::string& propertyName);
        Il2CppClass* GetParent(Il2CppClass* klass);
        std::vector<const MethodInfo*> GetAllMethods(Il2CppClass* klass);
    };

    class MethodResolver {
    private:
        typedef void* (*RuntimeInvokeFunc)(const MethodInfo*, void*, void**, void**);
        typedef Il2CppObject* (*ObjectNewFunc)(Il2CppClass*);

        RuntimeInvokeFunc runtimeInvokeFunc;
        ObjectNewFunc objectNewFunc;

    public:
        MethodResolver();
        ~MethodResolver();

        bool Initialize(ModuleResolver& moduleResolver);
        void* RuntimeInvoke(const MethodInfo* method, void* obj, void** params);
        Il2CppObject* CreateInstance(Il2CppClass* klass);
    };

    class StringResolver {
    private:
        typedef Il2CppString* (*NewStringFunc)(const wchar_t*);
        typedef Il2CppString* (*NewStringLenFunc)(const wchar_t*, uint32_t);

        NewStringFunc newStringFunc;
        NewStringLenFunc newStringLenFunc;

    public:
        StringResolver();
        ~StringResolver();

        bool Initialize(ModuleResolver& moduleResolver);
        Il2CppString* CreateString(const std::wstring& str);
        Il2CppString* CreateString(const wchar_t* str, uint32_t length);
        std::wstring ToString(Il2CppString* str);
    };

    class ObfuscationHandler {
    private:
        struct NameMapping {
            std::string obfuscatedName;
            std::string originalName;
            uintptr_t address;
        };

        std::unordered_map<std::string, NameMapping> classNameMappings;
        std::unordered_map<std::string, NameMapping> methodNameMappings;
        std::unordered_map<std::string, NameMapping> fieldNameMappings;
        bool isObfuscated;
        std::string obfuscationPattern;

    public:
        ObfuscationHandler();
        ~ObfuscationHandler();

        bool DetectObfuscation(ModuleResolver& moduleResolver);
        bool BuildNameMappings(DomainResolver& domainResolver, ImageResolver& imageResolver);
        std::string ResolveClassName(const std::string& obfuscatedName);
        std::string ResolveMethodName(const std::string& obfuscatedName);
        std::string ResolveFieldName(const std::string& obfuscatedName);
        bool IsObfuscated() const;
        void AddClassMapping(const std::string& obfuscated, const std::string& original, uintptr_t address);
        void AddMethodMapping(const std::string& obfuscated, const std::string& original, uintptr_t address);
        void AddFieldMapping(const std::string& obfuscated, const std::string& original, uintptr_t address);

    private:
        bool IsObfuscatedName(const std::string& name);
        std::string DeobfuscateName(const std::string& obfuscatedName);
    };

    class IL2CPPInternalResolver {
    private:
        ModuleResolver moduleResolver;
        DomainResolver domainResolver;
        ImageResolver imageResolver;
        ClassResolver classResolver;
        MethodResolver methodResolver;
        StringResolver stringResolver;
        ObfuscationHandler obfuscationHandler;
        bool initialized;

    public:
        IL2CPPInternalResolver();
        ~IL2CPPInternalResolver();

        bool Initialize();
        bool IsInitialized() const;
        Il2CppDomain* GetDomain();
        std::vector<Il2CppAssembly*> GetAssemblies();
        Il2CppAssembly* GetAssembly(const std::string& assemblyName);
        Il2CppImage* GetImage(const std::string& assemblyName);
        Il2CppClass* GetClass(const std::string& assemblyName, const std::string& namespaze, const std::string& className);
        MethodInfo* GetMethod(Il2CppClass* klass, const std::string& methodName, int parameterCount = -1);
        FieldInfo* GetField(Il2CppClass* klass, const std::string& fieldName);
        PropertyInfo* GetProperty(Il2CppClass* klass, const std::string& propertyName);
        void* InvokeMethod(const MethodInfo* method, void* instance, void** args);
        Il2CppObject* CreateInstance(const std::string& assemblyName, const std::string& namespaze, const std::string& className);
        Il2CppString* CreateString(const std::wstring& str);
        std::wstring ToString(Il2CppString* str);

        template<typename T>
        T GetFieldValue(Il2CppObject* instance, const std::string& fieldName);

        template<typename T>
        void SetFieldValue(Il2CppObject* instance, const std::string& fieldName, T value);

        template<typename T>
        T GetStaticFieldValue(Il2CppClass* klass, const std::string& fieldName);

        template<typename T>
        void SetStaticFieldValue(Il2CppClass* klass, const std::string& fieldName, T value);

        ModuleResolver& GetModuleResolver();
        DomainResolver& GetDomainResolver();
        ImageResolver& GetImageResolver();
        ClassResolver& GetClassResolver();
        MethodResolver& GetMethodResolver();
        StringResolver& GetStringResolver();
        ObfuscationHandler& GetObfuscationHandler();
    };

    template<typename T>
    T IL2CPPInternalResolver::GetFieldValue(Il2CppObject* instance, const std::string& fieldName) {
        if (!instance || !instance->klass) {
            return T();
        }

        FieldInfo* field = classResolver.GetFieldFromName(instance->klass, fieldName);
        if (!field) {
            return T();
        }

        uintptr_t objectAddr = reinterpret_cast<uintptr_t>(instance);
        uintptr_t fieldAddr = objectAddr + field->offset;

        return *reinterpret_cast<T*>(fieldAddr);
    }

    template<typename T>
    void IL2CPPInternalResolver::SetFieldValue(Il2CppObject* instance, const std::string& fieldName, T value) {
        if (!instance || !instance->klass) {
            return;
        }

        FieldInfo* field = classResolver.GetFieldFromName(instance->klass, fieldName);
        if (!field) {
            return;
        }

        uintptr_t objectAddr = reinterpret_cast<uintptr_t>(instance);
        uintptr_t fieldAddr = objectAddr + field->offset;

        *reinterpret_cast<T*>(fieldAddr) = value;
    }

    template<typename T>
    T IL2CPPInternalResolver::GetStaticFieldValue(Il2CppClass* klass, const std::string& fieldName) {
        if (!klass) {
            return T();
        }

        FieldInfo* field = classResolver.GetFieldFromName(klass, fieldName);
        if (!field || !klass->staticFields) {
            return T();
        }

        uintptr_t staticFieldsAddr = reinterpret_cast<uintptr_t>(klass->staticFields);
        uintptr_t fieldAddr = staticFieldsAddr + field->offset;

        return *reinterpret_cast<T*>(fieldAddr);
    }

    template<typename T>
    void IL2CPPInternalResolver::SetStaticFieldValue(Il2CppClass* klass, const std::string& fieldName, T value) {
        if (!klass) {
            return;
        }

        FieldInfo* field = classResolver.GetFieldFromName(klass, fieldName);
        if (!field || !klass->staticFields) {
            return;
        }

        uintptr_t staticFieldsAddr = reinterpret_cast<uintptr_t>(klass->staticFields);
        uintptr_t fieldAddr = staticFieldsAddr + field->offset;

        *reinterpret_cast<T*>(fieldAddr) = value;
    }

}
