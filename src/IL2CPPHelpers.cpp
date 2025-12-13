#include "../include/IL2CPPHelpers.h"
#include <cstring>
namespace IL2CPPResolver {
    namespace Helpers {
       

        bool MemoryHelper::ReadMemory(uintptr_t address, void* buffer, size_t size) {
            if (!IsValidAddress(address)) {
                return false;
            }

            DWORD oldProtection;
            if (!ProtectMemory(address, size, PAGE_EXECUTE_READWRITE, &oldProtection)) {
                return false;
            }

            memcpy(buffer, reinterpret_cast<void*>(address), size);
            ProtectMemory(address, size, oldProtection, &oldProtection);

            return true;
        }

        bool MemoryHelper::WriteMemory(uintptr_t address, const void* buffer, size_t size) {
            if (!IsValidAddress(address)) {
                return false;
            }

            DWORD oldProtection;
            if (!ProtectMemory(address, size, PAGE_EXECUTE_READWRITE, &oldProtection)) {
                return false;
            }

            memcpy(reinterpret_cast<void*>(address), buffer, size);
            ProtectMemory(address, size, oldProtection, &oldProtection);

            return true;
        }

        bool MemoryHelper::ProtectMemory(uintptr_t address, size_t size, DWORD protection, DWORD* oldProtection) {
            return VirtualProtect(reinterpret_cast<void*>(address), size, protection, oldProtection) != 0;
        }

        uintptr_t MemoryHelper::AllocateMemory(size_t size, DWORD protection) {
            void* memory = VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, protection);
            return reinterpret_cast<uintptr_t>(memory);
        }

        bool MemoryHelper::FreeMemory(uintptr_t address) {
            return VirtualFree(reinterpret_cast<void*>(address), 0, MEM_RELEASE) != 0;
        }

        bool MemoryHelper::IsValidAddress(uintptr_t address) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi)) == 0) {
                return false;
            }

            return (mbi.State == MEM_COMMIT) && (mbi.Protect != PAGE_NOACCESS);
        }

        bool MemoryHelper::IsExecutableAddress(uintptr_t address) {
            MEMORY_BASIC_INFORMATION mbi;
            if (VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi)) == 0) {
                return false;
            }

            return (mbi.State == MEM_COMMIT) &&
                (mbi.Protect == PAGE_EXECUTE ||
                    mbi.Protect == PAGE_EXECUTE_READ ||
                    mbi.Protect == PAGE_EXECUTE_READWRITE ||
                    mbi.Protect == PAGE_EXECUTE_WRITECOPY);
        }

        uintptr_t FieldHelper::GetFieldAddress(Il2CppObject* instance, FieldInfo* field) {
            if (!instance || !field) {
                return 0;
            }

            return reinterpret_cast<uintptr_t>(instance) + field->offset;
        }

        uintptr_t FieldHelper::GetStaticFieldAddress(Il2CppClass* klass, FieldInfo* field) {
            if (!klass || !field || !klass->staticFields) {
                return 0;
            }

            return reinterpret_cast<uintptr_t>(klass->staticFields) + field->offset;
        }

        FieldInfo* FieldHelper::FindField(Il2CppClass* klass, const std::string& fieldName, bool searchParents) {
            if (!klass) {
                return nullptr;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            FieldInfo* field = resolver.GetField(klass, fieldName);
            if (field) {
                return field;
            }

            if (searchParents && klass->parent) {
                return FindField(klass->parent, fieldName, true);
            }

            return nullptr;
        }

        std::vector<FieldInfo*> FieldHelper::GetAllFields(Il2CppClass* klass, bool includeParents) {
            std::vector<FieldInfo*> fields;

            if (!klass) {
                return fields;
            }

            for (uint16_t i = 0; i < klass->fieldCount; i++) {
                fields.push_back(&klass->fields[i]);
            }

            if (includeParents && klass->parent) {
                auto parentFields = GetAllFields(klass->parent, true);
                fields.insert(fields.end(), parentFields.begin(), parentFields.end());
            }

            return fields;
        }

        PropertyInfo* PropertyHelper::FindProperty(Il2CppClass* klass, const std::string& propertyName, bool searchParents) {
            if (!klass) {
                return nullptr;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            PropertyInfo* property = resolver.GetProperty(klass, propertyName);
            if (property) {
                return property;
            }

            if (searchParents && klass->parent) {
                return FindProperty(klass->parent, propertyName, true);
            }

            return nullptr;
        }

        std::vector<PropertyInfo*> PropertyHelper::GetAllProperties(Il2CppClass* klass, bool includeParents) {
            std::vector<PropertyInfo*> properties;

            if (!klass) {
                return properties;
            }

            for (uint16_t i = 0; i < klass->propertyCount; i++) {
                properties.push_back(&klass->properties[i]);
            }

            if (includeParents && klass->parent) {
                auto parentProperties = GetAllProperties(klass->parent, true);
                properties.insert(properties.end(), parentProperties.begin(), parentProperties.end());
            }

            return properties;
        }

        MethodInfo* MethodHelper::FindMethod(Il2CppClass* klass, const std::string& methodName, int parameterCount, bool searchParents) {
            if (!klass) {
                return nullptr;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            MethodInfo* method = resolver.GetMethod(klass, methodName, parameterCount);
            if (method) {
                return method;
            }

            if (searchParents && klass->parent) {
                return FindMethod(klass->parent, methodName, parameterCount, true);
            }

            return nullptr;
        }

        std::vector<MethodInfo*> MethodHelper::GetAllMethods(Il2CppClass* klass, bool includeParents) {
            std::vector<MethodInfo*> methods;

            if (!klass) {
                return methods;
            }

            for (uint16_t i = 0; i < klass->methodCount; i++) {
                methods.push_back(klass->methods[i]);
            }

            if (includeParents && klass->parent) {
                auto parentMethods = GetAllMethods(klass->parent, true);
                methods.insert(methods.end(), parentMethods.begin(), parentMethods.end());
            }

            return methods;
        }

        std::vector<MethodInfo*> MethodHelper::FindMethodsByName(Il2CppClass* klass, const std::string& methodName) {
            std::vector<MethodInfo*> result;
            auto allMethods = GetAllMethods(klass, true);

            for (auto method : allMethods) {
                if (method && method->name && std::string(method->name) == methodName) {
                    result.push_back(method);
                }
            }

            return result;
        }

        void* MethodHelper::InvokeMethodRaw(MethodInfo* method, Il2CppObject* instance, void** args) {
            if (!method) {
                return nullptr;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            return resolver.InvokeMethod(method, instance, args);
        }

        uintptr_t MethodHelper::GetMethodAddress(MethodInfo* method) {
            if (!method || !method->methodPointer) {
                return 0;
            }

            return reinterpret_cast<uintptr_t>(method->methodPointer);
        }

        bool MethodHelper::IsMethodStatic(MethodInfo* method) {
            if (!method) {
                return false;
            }

            return (method->flags & 0x0010) != 0;
        }

        bool MethodHelper::IsMethodVirtual(MethodInfo* method) {
            if (!method) {
                return false;
            }

            return (method->flags & 0x0040) != 0;
        }

        Il2CppArray* ArrayHelper::CreateArray(Il2CppClass* elementClass, uintptr_t length) {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t createArrayFunc = moduleResolver.GetExportedFunction("il2cpp_array_new");

            if (!createArrayFunc) {
                return nullptr;
            }

            typedef Il2CppArray* (*CreateArrayFunc)(Il2CppClass*, uintptr_t);
            auto createArray = reinterpret_cast<CreateArrayFunc>(createArrayFunc);

            return createArray(elementClass, length);
        }

        uintptr_t ArrayHelper::GetArrayLength(Il2CppArray* array) {
            if (!array) {
                return 0;
            }

            return array->maxLength;
        }

        uintptr_t ArrayHelper::GetArrayElementSize(Il2CppArray* array) {
            if (!array || !array->object.klass || !array->object.klass->elementClass) {
                return 0;
            }

            return array->object.klass->elementClass->instanceSize;
        }

        uintptr_t ArrayHelper::GetArrayElementAddress(Il2CppArray* array, uintptr_t index) {
            if (!array || index >= array->maxLength) {
                return 0;
            }

            uintptr_t elementSize = GetArrayElementSize(array);
            uintptr_t arrayDataOffset = offsetof(Il2CppArraySize, vector);

            return reinterpret_cast<uintptr_t>(array) + arrayDataOffset + (index * elementSize);
        }

        std::vector<void*> ArrayHelper::ArrayToVector(Il2CppArray* array) {
            std::vector<void*> result;

            if (!array) {
                return result;
            }

            uintptr_t length = GetArrayLength(array);
            for (uintptr_t i = 0; i < length; i++) {
                uintptr_t address = GetArrayElementAddress(array, i);
                result.push_back(reinterpret_cast<void*>(address));
            }

            return result;
        }

        Il2CppString* StringHelper::CreateString(const std::string& str) {
            std::wstring wstr(str.begin(), str.end());
            return CreateString(wstr);
        }

        Il2CppString* StringHelper::CreateString(const std::wstring& str) {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            return resolver.CreateString(str);
        }

        Il2CppString* StringHelper::CreateString(const char* str) {
            return CreateString(std::string(str));
        }

        Il2CppString* StringHelper::CreateString(const wchar_t* str) {
            return CreateString(std::wstring(str));
        }

        std::string StringHelper::ToString(Il2CppString* str) {
            if (!str) {
                return "";
            }

            std::wstring wstr(str->chars, str->length);
            return std::string(wstr.begin(), wstr.end());
        }

        std::wstring StringHelper::ToWString(Il2CppString* str) {
            if (!str) {
                return L"";
            }

            return std::wstring(str->chars, str->length);
        }

        int32_t StringHelper::GetStringLength(Il2CppString* str) {
            if (!str) {
                return 0;
            }

            return str->length;
        }

        bool StringHelper::CompareStrings(Il2CppString* str1, Il2CppString* str2) {
            if (!str1 || !str2) {
                return false;
            }

            if (str1->length != str2->length) {
                return false;
            }

            return wcscmp(str1->chars, str2->chars) == 0;
        }

        Il2CppString* StringHelper::ConcatenateStrings(Il2CppString* str1, Il2CppString* str2) {
            if (!str1 || !str2) {
                return nullptr;
            }

            std::wstring combined = std::wstring(str1->chars, str1->length) + std::wstring(str2->chars, str2->length);
            return CreateString(combined);
        }

        Il2CppClass* TypeHelper::GetClassFromType(const Il2CppType* type) {
            if (!type) {
                return nullptr;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t getClassFunc = moduleResolver.GetExportedFunction("il2cpp_class_from_il2cpp_type");

            if (!getClassFunc) {
                return nullptr;
            }

            typedef Il2CppClass* (*GetClassFunc)(const Il2CppType*);
            auto getClass = reinterpret_cast<GetClassFunc>(getClassFunc);

            return getClass(type);
        }

        const Il2CppType* TypeHelper::GetTypeFromClass(Il2CppClass* klass) {
            if (!klass) {
                return nullptr;
            }

            return &klass->byvalArg;
        }

        std::string TypeHelper::GetTypeName(const Il2CppType* type) {
            Il2CppClass* klass = GetClassFromType(type);
            return GetClassName(klass);
        }

        std::string TypeHelper::GetClassName(Il2CppClass* klass) {
            if (!klass || !klass->name) {
                return "";
            }

            return std::string(klass->name);
        }

        std::string TypeHelper::GetFullClassName(Il2CppClass* klass) {
            if (!klass || !klass->name) {
                return "";
            }

            std::string fullName;
            if (klass->namespaze && strlen(klass->namespaze) > 0) {
                fullName = std::string(klass->namespaze) + ".";
            }
            fullName += klass->name;

            return fullName;
        }

        bool TypeHelper::IsTypeValueType(const Il2CppType* type) {
            if (!type) {
                return false;
            }

            return type->type >= 0x02 && type->type <= 0x0D;
        }

        bool TypeHelper::IsTypePointer(const Il2CppType* type) {
            if (!type) {
                return false;
            }

            return type->type == 0x0F;
        }

        bool TypeHelper::IsTypeByRef(const Il2CppType* type) {
            if (!type) {
                return false;
            }

            return type->byRef != 0;
        }

        bool TypeHelper::IsSubclassOf(Il2CppClass* klass, Il2CppClass* baseClass) {
            if (!klass || !baseClass) {
                return false;
            }

            Il2CppClass* current = klass->parent;
            while (current) {
                if (current == baseClass) {
                    return true;
                }
                current = current->parent;
            }

            return false;
        }

        bool TypeHelper::ImplementsInterface(Il2CppClass* klass, Il2CppClass* interfaceClass) {
            if (!klass || !interfaceClass) {
                return false;
            }

            for (uint16_t i = 0; i < klass->interfacesCount; i++) {
                if (klass->implementedInterfaces[i] == interfaceClass) {
                    return true;
                }
            }

            return false;
        }

        Il2CppObject* ObjectHelper::CreateInstance(Il2CppClass* klass) {
            if (!klass) {
                return nullptr;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t objectNewFunc = moduleResolver.GetExportedFunction("il2cpp_object_new");

            if (!objectNewFunc) {
                return nullptr;
            }

            typedef Il2CppObject* (*ObjectNewFunc)(Il2CppClass*);
            auto objectNew = reinterpret_cast<ObjectNewFunc>(objectNewFunc);

            return objectNew(klass);
        }

        Il2CppObject* ObjectHelper::CreateInstance(const std::string& assemblyName, const std::string& namespaze, const std::string& className) {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            return resolver.CreateInstance(assemblyName, namespaze, className);
        }

        void ObjectHelper::DestroyInstance(Il2CppObject* instance) {
            if (!instance) {
                return;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t gcFreeFunc = moduleResolver.GetExportedFunction("il2cpp_gc_free");

            if (gcFreeFunc) {
                typedef void (*GCFreeFunc)(void*);
                auto gcFree = reinterpret_cast<GCFreeFunc>(gcFreeFunc);
                gcFree(instance);
            }
        }

        Il2CppClass* ObjectHelper::GetObjectClass(Il2CppObject* instance) {
            if (!instance) {
                return nullptr;
            }

            return instance->klass;
        }

        bool ObjectHelper::IsInstanceOf(Il2CppObject* instance, Il2CppClass* klass) {
            if (!instance || !klass) {
                return false;
            }

            Il2CppClass* instanceClass = instance->klass;
            if (instanceClass == klass) {
                return true;
            }

            return TypeHelper::IsSubclassOf(instanceClass, klass) || TypeHelper::ImplementsInterface(instanceClass, klass);
        }

        Il2CppObject* ObjectHelper::CastObject(Il2CppObject* instance, Il2CppClass* targetClass) {
            if (!IsInstanceOf(instance, targetClass)) {
                return nullptr;
            }

            return instance;
        }

        Il2CppObject* ObjectHelper::CloneObject(Il2CppObject* instance) {
            if (!instance) {
                return nullptr;
            }

            Il2CppClass* klass = instance->klass;
            Il2CppObject* clone = CreateInstance(klass);

            if (!clone) {
                return nullptr;
            }

            size_t objectSize = klass->instanceSize;
            memcpy(clone, instance, objectSize);

            return clone;
        }

        uintptr_t ObjectHelper::GetObjectSize(Il2CppObject* instance) {
            if (!instance || !instance->klass) {
                return 0;
            }

            return instance->klass->instanceSize;
        }

        Il2CppAssembly* AssemblyHelper::GetAssembly(const std::string& assemblyName) {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            return resolver.GetAssembly(assemblyName);
        }

        Il2CppImage* AssemblyHelper::GetImage(const std::string& assemblyName) {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            return resolver.GetImage(assemblyName);
        }

        std::vector<Il2CppAssembly*> AssemblyHelper::GetAllAssemblies() {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            return resolver.GetAssemblies();
        }

        std::vector<Il2CppClass*> AssemblyHelper::GetAllClasses(Il2CppImage* image) {
            std::vector<Il2CppClass*> classes;

            if (!image) {
                return classes;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ImageResolver& imageResolver = resolver.GetImageResolver();
            auto constClasses = imageResolver.GetAllClasses(image);

            for (auto klass : constClasses) {
                classes.push_back(const_cast<Il2CppClass*>(klass));
            }

            return classes;
        }

        Il2CppClass* AssemblyHelper::FindClass(const std::string& assemblyName, const std::string& namespaze, const std::string& className) {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            return resolver.GetClass(assemblyName, namespaze, className);
        }

        std::vector<Il2CppClass*> AssemblyHelper::FindClassesByName(const std::string& className) {
            std::vector<Il2CppClass*> result;

            auto assemblies = GetAllAssemblies();
            for (auto assembly : assemblies) {
                if (!assembly || !assembly->image) {
                    continue;
                }

                auto classes = GetAllClasses(assembly->image);
                for (auto klass : classes) {
                    if (klass && klass->name && std::string(klass->name) == className) {
                        result.push_back(klass);
                    }
                }
            }

            return result;
        }

        std::string AssemblyHelper::GetAssemblyName(Il2CppAssembly* assembly) {
            if (!assembly || !assembly->image || !assembly->image->name) {
                return "";
            }

            return std::string(assembly->image->name);
        }

        void ThreadHelper::AttachThread() {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t attachFunc = moduleResolver.GetExportedFunction("il2cpp_thread_attach");

            if (attachFunc) {
                typedef void* (*AttachFunc)(void*);
                auto attach = reinterpret_cast<AttachFunc>(attachFunc);
                attach(resolver.GetDomain());
            }
        }

        void ThreadHelper::DetachThread() {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t detachFunc = moduleResolver.GetExportedFunction("il2cpp_thread_detach");

            if (detachFunc) {
                typedef void (*DetachFunc)(void*);
                auto detach = reinterpret_cast<DetachFunc>(detachFunc);
                detach(GetCurrentThread());
            }
        }

        bool ThreadHelper::IsThreadAttached() {
            return GetCurrentThread() != nullptr;
        }

        void* ThreadHelper::GetCurrentThread() {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t currentFunc = moduleResolver.GetExportedFunction("il2cpp_thread_current");

            if (!currentFunc) {
                return nullptr;
            }

            typedef void* (*CurrentFunc)();
            auto current = reinterpret_cast<CurrentFunc>(currentFunc);

            return current();
        }

        void* ThreadHelper::GetDomain() {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            return resolver.GetDomain();
        }

        void GarbageCollectorHelper::Collect() {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t collectFunc = moduleResolver.GetExportedFunction("il2cpp_gc_collect");

            if (collectFunc) {
                typedef void (*CollectFunc)(int);
                auto collect = reinterpret_cast<CollectFunc>(collectFunc);
                collect(-1);
            }
        }

        void GarbageCollectorHelper::Collect(int generation) {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t collectFunc = moduleResolver.GetExportedFunction("il2cpp_gc_collect");

            if (collectFunc) {
                typedef void (*CollectFunc)(int);
                auto collect = reinterpret_cast<CollectFunc>(collectFunc);
                collect(generation);
            }
        }

        void GarbageCollectorHelper::DisableGC() {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t disableFunc = moduleResolver.GetExportedFunction("il2cpp_gc_disable");

            if (disableFunc) {
                typedef void (*DisableFunc)();
                auto disable = reinterpret_cast<DisableFunc>(disableFunc);
                disable();
            }
        }

        void GarbageCollectorHelper::EnableGC() {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t enableFunc = moduleResolver.GetExportedFunction("il2cpp_gc_enable");

            if (enableFunc) {
                typedef void (*EnableFunc)();
                auto enable = reinterpret_cast<EnableFunc>(enableFunc);
                enable();
            }
        }

        int64_t GarbageCollectorHelper::GetHeapSize() {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t heapSizeFunc = moduleResolver.GetExportedFunction("il2cpp_gc_get_heap_size");

            if (heapSizeFunc) {
                typedef int64_t(*HeapSizeFunc)();
                auto heapSize = reinterpret_cast<HeapSizeFunc>(heapSizeFunc);
                return heapSize();
            }

            return 0;
        }

        int64_t GarbageCollectorHelper::GetUsedHeapSize() {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t usedSizeFunc = moduleResolver.GetExportedFunction("il2cpp_gc_get_used_size");

            if (usedSizeFunc) {
                typedef int64_t(*UsedSizeFunc)();
                auto usedSize = reinterpret_cast<UsedSizeFunc>(usedSizeFunc);
                return usedSize();
            }

            return 0;
        }

        Il2CppObject* ExceptionHelper::GetLastException() {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t getExceptionFunc = moduleResolver.GetExportedFunction("il2cpp_get_exception_argument_null");

            if (!getExceptionFunc) {
                return nullptr;
            }

            return nullptr;
        }

        void ExceptionHelper::ClearLastException() {
        }

        std::string ExceptionHelper::GetExceptionMessage(Il2CppObject* exception) {
            if (!exception) {
                return "";
            }

            Il2CppClass* exceptionClass = exception->klass;
            FieldInfo* messageField = FieldHelper::FindField(exceptionClass, "_message");

            if (messageField) {
                Il2CppString* message = FieldHelper::GetFieldValue<Il2CppString*>(exception, messageField);
                return StringHelper::ToString(message);
            }

            return "";
        }

        std::string ExceptionHelper::GetExceptionStackTrace(Il2CppObject* exception) {
            if (!exception) {
                return "";
            }

            Il2CppClass* exceptionClass = exception->klass;
            MethodInfo* getStackTraceMethod = MethodHelper::FindMethod(exceptionClass, "get_StackTrace", 0);

            if (getStackTraceMethod) {
                void* result = MethodHelper::InvokeMethodRaw(getStackTraceMethod, exception, nullptr);
                Il2CppString* stackTrace = *reinterpret_cast<Il2CppString**>(&result);
                return StringHelper::ToString(stackTrace);
            }

            return "";
        }

        void ExceptionHelper::ThrowException(Il2CppObject* exception) {
            if (!exception) {
                return;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t raiseFunc = moduleResolver.GetExportedFunction("il2cpp_raise_exception");

            if (raiseFunc) {
                typedef void (*RaiseFunc)(Il2CppObject*);
                auto raise = reinterpret_cast<RaiseFunc>(raiseFunc);
                raise(exception);
            }
        }

        Il2CppObject* ExceptionHelper::CreateException(const std::string& message) {
            Il2CppClass* exceptionClass = AssemblyHelper::FindClass("mscorlib", "System", "Exception");
            if (!exceptionClass) {
                return nullptr;
            }

            Il2CppObject* exception = ObjectHelper::CreateInstance(exceptionClass);
            if (!exception) {
                return nullptr;
            }

            FieldInfo* messageField = FieldHelper::FindField(exceptionClass, "_message");
            if (messageField) {
                Il2CppString* messageStr = StringHelper::CreateString(message);
                FieldHelper::SetFieldValue<Il2CppString*>(exception, messageField, messageStr);
            }

            return exception;
        }

        Il2CppObject* ReflectionHelper::GetTypeObject(Il2CppClass* klass) {
            if (!klass) {
                return nullptr;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t getTypeFunc = moduleResolver.GetExportedFunction("il2cpp_type_get_object");

            if (!getTypeFunc) {
                return nullptr;
            }

            typedef Il2CppObject* (*GetTypeFunc)(const Il2CppType*);
            auto getType = reinterpret_cast<GetTypeFunc>(getTypeFunc);

            return getType(&klass->byvalArg);
        }

        Il2CppObject* ReflectionHelper::GetMethodInfoObject(MethodInfo* method) {
            if (!method) {
                return nullptr;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t getMethodFunc = moduleResolver.GetExportedFunction("il2cpp_method_get_object");

            if (!getMethodFunc) {
                return nullptr;
            }

            typedef Il2CppObject* (*GetMethodFunc)(const MethodInfo*, Il2CppClass*);
            auto getMethod = reinterpret_cast<GetMethodFunc>(getMethodFunc);

            return getMethod(method, method->klass);
        }

        Il2CppObject* ReflectionHelper::GetFieldInfoObject(FieldInfo* field) {
            if (!field) {
                return nullptr;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t getFieldFunc = moduleResolver.GetExportedFunction("il2cpp_field_get_object");

            if (!getFieldFunc) {
                return nullptr;
            }

            typedef Il2CppObject* (*GetFieldFunc)(Il2CppClass*, FieldInfo*);
            auto getField = reinterpret_cast<GetFieldFunc>(getFieldFunc);

            return getField(field->parent, field);
        }

        Il2CppObject* ReflectionHelper::GetPropertyInfoObject(PropertyInfo* property) {
            if (!property) {
                return nullptr;
            }

            return nullptr;
        }

        Il2CppClass* ReflectionHelper::GetClassFromTypeObject(Il2CppObject* typeObject) {
            if (!typeObject) {
                return nullptr;
            }

            FieldInfo* typeHandleField = FieldHelper::FindField(typeObject->klass, "value");
            if (!typeHandleField) {
                return nullptr;
            }

            const Il2CppType* type = FieldHelper::GetFieldValue<const Il2CppType*>(typeObject, typeHandleField);
            return TypeHelper::GetClassFromType(type);
        }

        Il2CppObject* DelegateHelper::CreateDelegate(Il2CppClass* delegateClass, Il2CppObject* target, MethodInfo* method) {
            if (!delegateClass || !method) {
                return nullptr;
            }

            Il2CppObject* delegateObject = ObjectHelper::CreateInstance(delegateClass);
            if (!delegateObject) {
                return nullptr;
            }

            FieldInfo* targetField = FieldHelper::FindField(delegateClass, "m_target");
            FieldInfo* methodField = FieldHelper::FindField(delegateClass, "method");

            if (targetField) {
                FieldHelper::SetFieldValue<Il2CppObject*>(delegateObject, targetField, target);
            }

            if (methodField) {
                FieldHelper::SetFieldValue<MethodInfo*>(delegateObject, methodField, method);
            }

            return delegateObject;
        }

        void DelegateHelper::InvokeDelegate(Il2CppObject* delegateObject, void** args) {
            if (!delegateObject) {
                return;
            }

            MethodInfo* invokeMethod = MethodHelper::FindMethod(delegateObject->klass, "Invoke");
            if (invokeMethod) {
                MethodHelper::InvokeMethodRaw(invokeMethod, delegateObject, args);
            }
        }

        MethodInfo* DelegateHelper::GetDelegateMethod(Il2CppObject* delegateObject) {
            if (!delegateObject) {
                return nullptr;
            }

            FieldInfo* methodField = FieldHelper::FindField(delegateObject->klass, "method");
            if (methodField) {
                return FieldHelper::GetFieldValue<MethodInfo*>(delegateObject, methodField);
            }

            return nullptr;
        }

        Il2CppObject* DelegateHelper::GetDelegateTarget(Il2CppObject* delegateObject) {
            if (!delegateObject) {
                return nullptr;
            }

            FieldInfo* targetField = FieldHelper::FindField(delegateObject->klass, "m_target");
            if (targetField) {
                return FieldHelper::GetFieldValue<Il2CppObject*>(delegateObject, targetField);
            }

            return nullptr;
        }

        void EventHelper::AddEventHandler(Il2CppObject* instance, const std::string& eventName, Il2CppObject* handler) {
            if (!instance || !handler) {
                return;
            }

            std::string addMethodName = "add_" + eventName;
            MethodInfo* addMethod = MethodHelper::FindMethod(instance->klass, addMethodName, 1);

            if (addMethod) {
                void* args[] = { handler };
                MethodHelper::InvokeMethodRaw(addMethod, instance, args);
            }
        }

        void EventHelper::RemoveEventHandler(Il2CppObject* instance, const std::string& eventName, Il2CppObject* handler) {
            if (!instance || !handler) {
                return;
            }

            std::string removeMethodName = "remove_" + eventName;
            MethodInfo* removeMethod = MethodHelper::FindMethod(instance->klass, removeMethodName, 1);

            if (removeMethod) {
                void* args[] = { handler };
                MethodHelper::InvokeMethodRaw(removeMethod, instance, args);
            }
        }

        void EventHelper::RaiseEvent(Il2CppObject* instance, const std::string& eventName, void** args) {
            if (!instance) {
                return;
            }

            FieldInfo* eventField = FieldHelper::FindField(instance->klass, eventName);
            if (!eventField) {
                return;
            }

            Il2CppObject* delegateObject = FieldHelper::GetFieldValue<Il2CppObject*>(instance, eventField);
            if (delegateObject) {
                DelegateHelper::InvokeDelegate(delegateObject, args);
            }
        }

        bool EnumHelper::IsEnum(Il2CppClass* klass) {
            if (!klass) {
                return false;
            }

            return klass->enumtype != 0;
        }

        std::vector<std::string> EnumHelper::GetEnumNames(Il2CppClass* enumClass) {
            std::vector<std::string> names;

            if (!IsEnum(enumClass)) {
                return names;
            }

            for (uint16_t i = 0; i < enumClass->fieldCount; i++) {
                FieldInfo* field = &enumClass->fields[i];
                if (field && field->name && MethodHelper::IsMethodStatic(nullptr)) {
                    names.push_back(std::string(field->name));
                }
            }

            return names;
        }

        std::vector<int64_t> EnumHelper::GetEnumValues(Il2CppClass* enumClass) {
            std::vector<int64_t> values;

            if (!IsEnum(enumClass)) {
                return values;
            }

            for (uint16_t i = 0; i < enumClass->fieldCount; i++) {
                FieldInfo* field = &enumClass->fields[i];
                if (field && field->name) {
                    int64_t value = FieldHelper::GetStaticFieldValue<int64_t>(enumClass, field);
                    values.push_back(value);
                }
            }

            return values;
        }

        std::string EnumHelper::GetEnumName(Il2CppClass* enumClass, int64_t value) {
            if (!IsEnum(enumClass)) {
                return "";
            }

            for (uint16_t i = 0; i < enumClass->fieldCount; i++) {
                FieldInfo* field = &enumClass->fields[i];
                if (field && field->name) {
                    int64_t fieldValue = FieldHelper::GetStaticFieldValue<int64_t>(enumClass, field);
                    if (fieldValue == value) {
                        return std::string(field->name);
                    }
                }
            }

            return "";
        }

        int64_t EnumHelper::GetEnumValue(Il2CppClass* enumClass, const std::string& name) {
            if (!IsEnum(enumClass)) {
                return 0;
            }

            for (uint16_t i = 0; i < enumClass->fieldCount; i++) {
                FieldInfo* field = &enumClass->fields[i];
                if (field && field->name && std::string(field->name) == name) {
                    return FieldHelper::GetStaticFieldValue<int64_t>(enumClass, field);
                }
            }

            return 0;
        }

        Il2CppClass* GenericHelper::MakeGenericClass(Il2CppClass* genericClass, Il2CppClass** typeArgs, size_t typeArgCount) {
            if (!genericClass || !typeArgs) {
                return nullptr;
            }

            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            ModuleResolver& moduleResolver = resolver.GetModuleResolver();
            uintptr_t inflateFunc = moduleResolver.GetExportedFunction("il2cpp_class_inflate_generic_class");

            if (!inflateFunc) {
                return nullptr;
            }

            typedef Il2CppClass* (*InflateFunc)(Il2CppClass*, void*);
            auto inflate = reinterpret_cast<InflateFunc>(inflateFunc);

            return inflate(genericClass, nullptr);
        }

        MethodInfo* GenericHelper::MakeGenericMethod(MethodInfo* genericMethod, Il2CppClass** typeArgs, size_t typeArgCount) {
            if (!genericMethod || !typeArgs) {
                return nullptr;
            }

            return genericMethod;
        }

        bool GenericHelper::IsGenericClass(Il2CppClass* klass) {
            if (!klass) {
                return false;
            }

            return klass->isGeneric != 0;
        }

        bool GenericHelper::IsGenericMethod(MethodInfo* method) {
            if (!method) {
                return false;
            }

            return method->isGeneric != 0;
        }

        std::vector<Il2CppClass*> GenericHelper::GetGenericArguments(Il2CppClass* klass) {
            std::vector<Il2CppClass*> args;

            if (!IsGenericClass(klass)) {
                return args;
            }

            return args;
        }

        bool AttributeHelper::HasAttribute(Il2CppClass* klass, Il2CppClass* attributeClass) {
            if (!klass || !attributeClass) {
                return false;
            }

            return false;
        }

        bool AttributeHelper::HasAttribute(MethodInfo* method, Il2CppClass* attributeClass) {
            if (!method || !attributeClass) {
                return false;
            }

            return false;
        }

        bool AttributeHelper::HasAttribute(FieldInfo* field, Il2CppClass* attributeClass) {
            if (!field || !attributeClass) {
                return false;
            }

            return false;
        }

        std::vector<Il2CppObject*> AttributeHelper::GetAttributes(Il2CppClass* klass) {
            std::vector<Il2CppObject*> attributes;

            if (!klass) {
                return attributes;
            }

            return attributes;
        }

        std::vector<Il2CppObject*> AttributeHelper::GetAttributes(MethodInfo* method) {
            std::vector<Il2CppObject*> attributes;

            if (!method) {
                return attributes;
            }

            return attributes;
        }

        std::vector<Il2CppObject*> AttributeHelper::GetAttributes(FieldInfo* field) {
            std::vector<Il2CppObject*> attributes;

            if (!field) {
                return attributes;
            }

            return attributes;
        }

    }
}

