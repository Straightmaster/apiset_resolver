#include <iostream>
#include <expected>
#include <print>
#include <Windows.h>
#include <winternl.h>
#pragma comment(lib,"ntdll.lib")



std::expected<std::string,NTSTATUS> ResolveApiSet(const std::string_view apiset)
{
    //functions
    //https://learn.microsoft.com/en-us/windows/win32/api/apiquery2/nf-apiquery2-isapisetimplemented exists now too
    //https://learn.microsoft.com/en-us/windows/win32/api/apiquery2/nf-apiquery2-getapisetmodulebasename too
    static NTSTATUS(*RtlIsApiSetImplemented)(const char* apiset) = reinterpret_cast<NTSTATUS(*)(const char*)>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlIsApiSetImplemented")); 
    static NTSTATUS(*ApiSetGetImplementationHost)(const char* ApiSet, bool* Success, PCUNICODE_STRING out) = reinterpret_cast<NTSTATUS(*)(const char*, bool*, PCUNICODE_STRING)>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "ApiSetGetImplementationHost"));

    //variables
    ANSI_STRING ansi{};
    UNICODE_STRING uc{};
    NTSTATUS status{};
    bool success{};
    std::string result{};
    

    if (!RtlIsApiSetImplemented || !ApiSetGetImplementationHost)
    {
        //resolves to "A required pointer is null."
        //STATUS_FWP_NULL_POINTER
        SetLastError(RtlNtStatusToDosError(0xC022001C));
        return std::unexpected(0xC022001C);
    }

    status = RtlIsApiSetImplemented(apiset.data());
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return std::unexpected(status);
    }

    //the returned UNICODE_STRING isn't allocated, purely a view of api set map, so no free'ing needed
    status = ApiSetGetImplementationHost(apiset.data(), &success, &uc);
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return std::unexpected(status);
    }

    //ansi string IS allocated, so we have to free ;)
    status = RtlUnicodeStringToAnsiString(&ansi, &uc, true);
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return std::unexpected(status);
    }

    //copy it over to an std::string
    result = ansi.Buffer;

    //and free
    RtlFreeAnsiString(&ansi);

    return result;
}

int main()
{
    std::expected<std::string,NTSTATUS> result = ResolveApiSet("api-ms-win-base-util-l1-1-0");
    if (!result) {
        std::println("Dos error: {:s}", std::system_category().message(GetLastError()));
        std::println("Nt status: {:x}", static_cast<unsigned int>(result.error()));
    }
    else
        std::println("{:s}", result.value());
}
