#include "IL2CPPResolver.h"
#include <functional>
#include "../lib/minhook/include/MinHook.h"
#include <memory>
namespace IL2CPPResolver {
    namespace Helpers {

        struct Il2CppArray {
            Il2CppObject object;
            void* bounds;
            uintptr_t maxLength;
        };

        struct Il2CppArraySize {
            Il2CppObject object;
            void* bounds;
            uintptr_t maxLength;
            void* vector[1];
        };

        template<typename T>
        struct Il2CppList {
            Il2CppObject object;
            Il2CppArray* items;
            int32_t size;
            int32_t version;
        };

        struct Il2CppDictionary {
            Il2CppObject object;
            Il2CppArray* entries;
            int32_t count;
            int32_t version;
            int32_t freeList;
            int32_t freeCount;
            void* comparer;
            void* keys;
            void* values;
        };

        class MemoryHelper {
        public:
            static bool ReadMemory(uintptr_t address, void* buffer, size_t size);
            static bool WriteMemory(uintptr_t address, const void* buffer, size_t size);

            template<typename T>
            static T Read(uintptr_t address);

            template<typename T>
            static bool Write(uintptr_t address, T value);

            static bool ProtectMemory(uintptr_t address, size_t size, DWORD protection, DWORD* oldProtection);
            static uintptr_t AllocateMemory(size_t size, DWORD protection);
            static bool FreeMemory(uintptr_t address);
            static bool IsValidAddress(uintptr_t address);
            static bool IsExecutableAddress(uintptr_t address);
        };

        class FieldHelper {
        public:
            static uintptr_t GetFieldAddress(Il2CppObject* instance, FieldInfo* field);
            static uintptr_t GetStaticFieldAddress(Il2CppClass* klass, FieldInfo* field);

            template<typename T>
            static T GetFieldValue(Il2CppObject* instance, FieldInfo* field);

            template<typename T>
            static void SetFieldValue(Il2CppObject* instance, FieldInfo* field, T value);

            template<typename T>
            static T GetStaticFieldValue(Il2CppClass* klass, FieldInfo* field);

            template<typename T>
            static void SetStaticFieldValue(Il2CppClass* klass, FieldInfo* field, T value);

            static FieldInfo* FindField(Il2CppClass* klass, const std::string& fieldName, bool searchParents = true);
            static std::vector<FieldInfo*> GetAllFields(Il2CppClass* klass, bool includeParents = true);
        };

        class PropertyHelper {
        public:
            template<typename T>
            static T GetPropertyValue(Il2CppObject* instance, PropertyInfo* property);

            template<typename T>
            static void SetPropertyValue(Il2CppObject* instance, PropertyInfo* property, T value);

            static PropertyInfo* FindProperty(Il2CppClass* klass, const std::string& propertyName, bool searchParents = true);
            static std::vector<PropertyInfo*> GetAllProperties(Il2CppClass* klass, bool includeParents = true);
        };

        class MethodHelper {
        public:
            static MethodInfo* FindMethod(Il2CppClass* klass, const std::string& methodName, int parameterCount = -1, bool searchParents = true);
            static std::vector<MethodInfo*> GetAllMethods(Il2CppClass* klass, bool includeParents = true);
            static std::vector<MethodInfo*> FindMethodsByName(Il2CppClass* klass, const std::string& methodName);

            template<typename ReturnType, typename... Args>
            static ReturnType InvokeMethod(MethodInfo* method, Il2CppObject* instance, Args... args);

            template<typename ReturnType, typename... Args>
            static ReturnType InvokeStaticMethod(MethodInfo* method, Args... args);

            static void* InvokeMethodRaw(MethodInfo* method, Il2CppObject* instance, void** args);
            static uintptr_t GetMethodAddress(MethodInfo* method);
            static bool IsMethodStatic(MethodInfo* method);
            static bool IsMethodVirtual(MethodInfo* method);
        };

        class ArrayHelper {
        public:
            static Il2CppArray* CreateArray(Il2CppClass* elementClass, uintptr_t length);
            static uintptr_t GetArrayLength(Il2CppArray* array);
            static uintptr_t GetArrayElementSize(Il2CppArray* array);

            template<typename T>
            static T GetArrayElement(Il2CppArray* array, uintptr_t index);

            template<typename T>
            static void SetArrayElement(Il2CppArray* array, uintptr_t index, T value);

            static uintptr_t GetArrayElementAddress(Il2CppArray* array, uintptr_t index);
            static std::vector<void*> ArrayToVector(Il2CppArray* array);

            template<typename T>
            static std::vector<T> ArrayToTypedVector(Il2CppArray* array);
        };

        class ListHelper {
        public:
            template<typename T>
            static Il2CppList<T>* CreateList(Il2CppClass* elementClass);

            template<typename T>
            static void AddToList(Il2CppList<T>* list, T item);

            template<typename T>
            static T GetListElement(Il2CppList<T>* list, int32_t index);

            template<typename T>
            static void SetListElement(Il2CppList<T>* list, int32_t index, T item);

            template<typename T>
            static int32_t GetListSize(Il2CppList<T>* list);

            template<typename T>
            static void ClearList(Il2CppList<T>* list);

            template<typename T>
            static std::vector<T> ListToVector(Il2CppList<T>* list);
        };

        class StringHelper {
        public:
            static Il2CppString* CreateString(const std::string& str);
            static Il2CppString* CreateString(const std::wstring& str);
            static Il2CppString* CreateString(const char* str);
            static Il2CppString* CreateString(const wchar_t* str);
            static std::string ToString(Il2CppString* str);
            static std::wstring ToWString(Il2CppString* str);
            static int32_t GetStringLength(Il2CppString* str);
            static bool CompareStrings(Il2CppString* str1, Il2CppString* str2);
            static Il2CppString* ConcatenateStrings(Il2CppString* str1, Il2CppString* str2);
        };

        class TypeHelper {
        public:
            static Il2CppClass* GetClassFromType(const Il2CppType* type);
            static const Il2CppType* GetTypeFromClass(Il2CppClass* klass);
            static std::string GetTypeName(const Il2CppType* type);
            static std::string GetClassName(Il2CppClass* klass);
            static std::string GetFullClassName(Il2CppClass* klass);
            static bool IsTypeValueType(const Il2CppType* type);
            static bool IsTypePointer(const Il2CppType* type);
            static bool IsTypeByRef(const Il2CppType* type);
            static bool IsSubclassOf(Il2CppClass* klass, Il2CppClass* baseClass);
            static bool ImplementsInterface(Il2CppClass* klass, Il2CppClass* interfaceClass);
        };

        class ObjectHelper {
        public:
            static Il2CppObject* CreateInstance(Il2CppClass* klass);
            static Il2CppObject* CreateInstance(const std::string& assemblyName, const std::string& namespaze, const std::string& className);
            static void DestroyInstance(Il2CppObject* instance);
            static Il2CppClass* GetObjectClass(Il2CppObject* instance);
            static bool IsInstanceOf(Il2CppObject* instance, Il2CppClass* klass);
            static Il2CppObject* CastObject(Il2CppObject* instance, Il2CppClass* targetClass);
            static Il2CppObject* CloneObject(Il2CppObject* instance);
            static uintptr_t GetObjectSize(Il2CppObject* instance);
        };

        class AssemblyHelper {
        public:
            static Il2CppAssembly* GetAssembly(const std::string& assemblyName);
            static Il2CppImage* GetImage(const std::string& assemblyName);
            static std::vector<Il2CppAssembly*> GetAllAssemblies();
            static std::vector<Il2CppClass*> GetAllClasses(Il2CppImage* image);
            static Il2CppClass* FindClass(const std::string& assemblyName, const std::string& namespaze, const std::string& className);
            static std::vector<Il2CppClass*> FindClassesByName(const std::string& className);
            static std::string GetAssemblyName(Il2CppAssembly* assembly);
        };

        class ThreadHelper {
        public:
            static void AttachThread();
            static void DetachThread();
            static bool IsThreadAttached();
            static void* GetCurrentThread();
            static void* GetDomain();
        };

        class GarbageCollectorHelper {
        public:
            static void Collect();
            static void Collect(int generation);
            static void DisableGC();
            static void EnableGC();
            static int64_t GetHeapSize();
            static int64_t GetUsedHeapSize();
        };

        class ExceptionHelper {
        public:
            static Il2CppObject* GetLastException();
            static void ClearLastException();
            static std::string GetExceptionMessage(Il2CppObject* exception);
            static std::string GetExceptionStackTrace(Il2CppObject* exception);
            static void ThrowException(Il2CppObject* exception);
            static Il2CppObject* CreateException(const std::string& message);
        };

        class ReflectionHelper {
        public:
            static Il2CppObject* GetTypeObject(Il2CppClass* klass);
            static Il2CppObject* GetMethodInfoObject(MethodInfo* method);
            static Il2CppObject* GetFieldInfoObject(FieldInfo* field);
            static Il2CppObject* GetPropertyInfoObject(PropertyInfo* property);
            static Il2CppClass* GetClassFromTypeObject(Il2CppObject* typeObject);
        };

        class DelegateHelper {
        public:
            static Il2CppObject* CreateDelegate(Il2CppClass* delegateClass, Il2CppObject* target, MethodInfo* method);
            static void InvokeDelegate(Il2CppObject* delegateObject, void** args);
            static MethodInfo* GetDelegateMethod(Il2CppObject* delegateObject);
            static Il2CppObject* GetDelegateTarget(Il2CppObject* delegateObject);
        };

        class EventHelper {
        public:
            static void AddEventHandler(Il2CppObject* instance, const std::string& eventName, Il2CppObject* handler);
            static void RemoveEventHandler(Il2CppObject* instance, const std::string& eventName, Il2CppObject* handler);
            static void RaiseEvent(Il2CppObject* instance, const std::string& eventName, void** args);
        };

        class EnumHelper {
        public:
            static bool IsEnum(Il2CppClass* klass);
            static std::vector<std::string> GetEnumNames(Il2CppClass* enumClass);
            static std::vector<int64_t> GetEnumValues(Il2CppClass* enumClass);
            static std::string GetEnumName(Il2CppClass* enumClass, int64_t value);
            static int64_t GetEnumValue(Il2CppClass* enumClass, const std::string& name);
        };

        class GenericHelper {
        public:
            static Il2CppClass* MakeGenericClass(Il2CppClass* genericClass, Il2CppClass** typeArgs, size_t typeArgCount);
            static MethodInfo* MakeGenericMethod(MethodInfo* genericMethod, Il2CppClass** typeArgs, size_t typeArgCount);
            static bool IsGenericClass(Il2CppClass* klass);
            static bool IsGenericMethod(MethodInfo* method);
            static std::vector<Il2CppClass*> GetGenericArguments(Il2CppClass* klass);
        };

        class AttributeHelper {
        public:
            static bool HasAttribute(Il2CppClass* klass, Il2CppClass* attributeClass);
            static bool HasAttribute(MethodInfo* method, Il2CppClass* attributeClass);
            static bool HasAttribute(FieldInfo* field, Il2CppClass* attributeClass);
            static std::vector<Il2CppObject*> GetAttributes(Il2CppClass* klass);
            static std::vector<Il2CppObject*> GetAttributes(MethodInfo* method);
            static std::vector<Il2CppObject*> GetAttributes(FieldInfo* field);
        };

        template<typename T>
        T MemoryHelper::Read(uintptr_t address) {
            T value;
            ReadMemory(address, &value, sizeof(T));
            return value;
        }

        template<typename T>
        bool MemoryHelper::Write(uintptr_t address, T value) {
            return WriteMemory(address, &value, sizeof(T));
        }

        template<typename T>
        T FieldHelper::GetFieldValue(Il2CppObject* instance, FieldInfo* field) {
            uintptr_t address = GetFieldAddress(instance, field);
            return MemoryHelper::Read<T>(address);
        }

        template<typename T>
        void FieldHelper::SetFieldValue(Il2CppObject* instance, FieldInfo* field, T value) {
            uintptr_t address = GetFieldAddress(instance, field);
            MemoryHelper::Write<T>(address, value);
        }

        template<typename T>
        T FieldHelper::GetStaticFieldValue(Il2CppClass* klass, FieldInfo* field) {
            uintptr_t address = GetStaticFieldAddress(klass, field);
            return MemoryHelper::Read<T>(address);
        }

        template<typename T>
        void FieldHelper::SetStaticFieldValue(Il2CppClass* klass, FieldInfo* field, T value) {
            uintptr_t address = GetStaticFieldAddress(klass, field);
            MemoryHelper::Write<T>(address, value);
        }

        template<typename T>
        T PropertyHelper::GetPropertyValue(Il2CppObject* instance, PropertyInfo* property) {
            if (!property || !property->get) {
                return T();
            }

            void* result = MethodHelper::InvokeMethodRaw(const_cast<MethodInfo*>(property->get), instance, nullptr);
            return *reinterpret_cast<T*>(&result);
        }

        template<typename T>
        void PropertyHelper::SetPropertyValue(Il2CppObject* instance, PropertyInfo* property, T value) {
            if (!property || !property->set) {
                return;
            }

            void* args[1] = { reinterpret_cast<void*>(&value) };
            MethodHelper::InvokeMethodRaw(const_cast<MethodInfo*>(property->set), instance, args);
        }

        template<typename ReturnType, typename... Args>
        ReturnType MethodHelper::InvokeMethod(MethodInfo* method, Il2CppObject* instance, Args... args) {
            void* argArray[] = { reinterpret_cast<void*>(&args)... };
            void* result = InvokeMethodRaw(method, instance, argArray);
            return *reinterpret_cast<ReturnType*>(&result);
        }

        template<typename ReturnType, typename... Args>
        ReturnType MethodHelper::InvokeStaticMethod(MethodInfo* method, Args... args) {
            return InvokeMethod<ReturnType>(method, nullptr, args...);
        }

        template<typename T>
        T ArrayHelper::GetArrayElement(Il2CppArray* array, uintptr_t index) {
            uintptr_t address = GetArrayElementAddress(array, index);
            return MemoryHelper::Read<T>(address);
        }

        template<typename T>
        void ArrayHelper::SetArrayElement(Il2CppArray* array, uintptr_t index, T value) {
            uintptr_t address = GetArrayElementAddress(array, index);
            MemoryHelper::Write<T>(address, value);
        }

        template<typename T>
        std::vector<T> ArrayHelper::ArrayToTypedVector(Il2CppArray* array) {
            std::vector<T> result;
            uintptr_t length = GetArrayLength(array);

            for (uintptr_t i = 0; i < length; i++) {
                result.push_back(GetArrayElement<T>(array, i));
            }

            return result;
        }

        template<typename T>
        Il2CppList<T>* ListHelper::CreateList(Il2CppClass* elementClass) {
            IL2CPPInternalResolver resolver;
            resolver.Initialize();

            Il2CppClass* listClass = resolver.GetClass("mscorlib", "System.Collections.Generic", "List`1");
            if (!listClass) {
                return nullptr;
            }

            Il2CppClass* genericArgs[] = { elementClass };
            Il2CppClass* concreteListClass = GenericHelper::MakeGenericClass(listClass, genericArgs, 1);

            return reinterpret_cast<Il2CppList<T>*>(ObjectHelper::CreateInstance(concreteListClass));
        }

        template<typename T>
        void ListHelper::AddToList(Il2CppList<T>* list, T item) {
            if (!list) {
                return;
            }

            Il2CppClass* listClass = list->object.klass;
            MethodInfo* addMethod = MethodHelper::FindMethod(listClass, "Add", 1);

            if (addMethod) {
                void* args[] = { reinterpret_cast<void*>(&item) };
                MethodHelper::InvokeMethodRaw(addMethod, reinterpret_cast<Il2CppObject*>(list), args);
            }
        }

        template<typename T>
        T ListHelper::GetListElement(Il2CppList<T>* list, int32_t index) {
            if (!list || !list->items) {
                return T();
            }

            return ArrayHelper::GetArrayElement<T>(list->items, index);
        }

        template<typename T>
        void ListHelper::SetListElement(Il2CppList<T>* list, int32_t index, T item) {
            if (!list || !list->items) {
                return;
            }

            ArrayHelper::SetArrayElement<T>(list->items, index, item);
        }

        template<typename T>
        int32_t ListHelper::GetListSize(Il2CppList<T>* list) {
            if (!list) {
                return 0;
            }

            return list->size;
        }

        template<typename T>
        void ListHelper::ClearList(Il2CppList<T>* list) {
            if (!list) {
                return;
            }

            Il2CppClass* listClass = list->object.klass;
            MethodInfo* clearMethod = MethodHelper::FindMethod(listClass, "Clear", 0);

            if (clearMethod) {
                MethodHelper::InvokeMethodRaw(clearMethod, reinterpret_cast<Il2CppObject*>(list), nullptr);
            }
        }

        template<typename T>
        std::vector<T> ListHelper::ListToVector(Il2CppList<T>* list) {
            std::vector<T> result;

            if (!list) {
                return result;
            }

            int32_t size = GetListSize(list);
            for (int32_t i = 0; i < size; i++) {
                result.push_back(GetListElement<T>(list, i));
            }

            return result;
        }

        namespace Hooks {

            inline bool init() {
                return MH_Initialize() == MH_OK;
            }

            template<typename FuncType>
            bool hook(const std::string& cls, const std::string& method, void* detour, FuncType** orig) {
                IL2CPPInternalResolver resolver;
                resolver.Initialize();

                std::vector<Il2CppAssembly*> assemblies = Helpers::AssemblyHelper::GetAllAssemblies();

                for (size_t i = 0; i < assemblies.size(); i++) {
                    Il2CppAssembly* assembly = assemblies[i];
                    if (!assembly || !assembly->image) continue;

                    std::vector<Il2CppClass*> classes = Helpers::AssemblyHelper::GetAllClasses(assembly->image);
                    for (size_t j = 0; j < classes.size(); j++) {
                        Il2CppClass* klass = classes[j];
                        if (!klass || !klass->name) continue;
                        if (std::string(klass->name) != cls) continue;

                        MethodInfo* m = Helpers::MethodHelper::FindMethod(klass, method, -1, false);
                        if (!m || !m->methodPointer) continue;

                        if (MH_CreateHook(m->methodPointer, detour, reinterpret_cast<void**>(orig)) != MH_OK)
                            return false;

                        if (MH_EnableHook(m->methodPointer) != MH_OK)
                            return false;

                        return true;
                    }
                }

                return false;
            }
        }

    }
}
