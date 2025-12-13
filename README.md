# IL2CPP Resolver

il2cpp hooking lib for unity games. auto resolves classes/methods, handles obfuscation, memory ops and more useful shit
## usage

```cpp
typedef void (*WeaponSounds_o)(void*);
WeaponSounds_o WeaponSounds_orig = nullptr;

void WeaponSounds(void* mobj) {
    auto obj = reinterpret_cast<Il2CppObject*>(mobj);
    FieldInfo* bazooka = FieldHelper::FindField(obj->klass, "bazooka");
    if (bazooka) FieldHelper::SetFieldValue<bool>(obj, bazooka, true);
    WeaponSounds_orig(mobj);
}

void Hookz() {
    Hooks::init();
    Hooks::hook("WeaponSounds", "Update", reinterpret_cast<void*>(&WeaponSounds), &WeaponSounds_orig);
}
```

searches all assemblies automatically. no need for namespace/assembly names

tested on pixel gun 3d

## build

vs2022+, minhook + rat included

## license

mit
