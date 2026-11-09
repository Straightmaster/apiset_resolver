#include <Windows.h>
#include <winternl.h>
#include <iostream>
#include <optional>
#include <print>
#pragma comment(lib,"ntdll.lib")


std::optional<std::string> ResolveApiSet(const std::string_view apiset)
{
    //functions
    static NTSTATUS(*IsApiSetImplemented)(const char* apiset) = reinterpret_cast<NTSTATUS(*)(const char*)>(GetProcAddress(GetModuleHandleA("api-ms-win-core-apiquery-l2-1-0.dll"), "IsApiSetImplemented")); //or RtlIsApiSetImplemented in ntdll
    static NTSTATUS(*ApiSetGetImplementationHost)(const char* ApiSet, bool* Success, PCUNICODE_STRING out) = reinterpret_cast<NTSTATUS(*)(const char*, bool*, PCUNICODE_STRING)>(GetProcAddress(GetModuleHandleA("ntdll.dll"), "ApiSetGetImplementationHost"));

    //variables
    ANSI_STRING ansi{};
    UNICODE_STRING uc{};
    NTSTATUS status{};
    bool success{};
    std::string result{};

    if (!IsApiSetImplemented || !ApiSetGetImplementationHost)
    {
        //resolves to "A required pointer is null."
        //STATUS_FWP_NULL_POINTER
        SetLastError(RtlNtStatusToDosError(0xC022001C));
        return std::nullopt;
    }


    status = IsApiSetImplemented(apiset.data());
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return std::nullopt;
    }

    //the returned UNICODE_STRING isn't allocated, purely a view of api set map, so no free'ing needed
    status = ApiSetGetImplementationHost(apiset.data(), &success, &uc);
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return std::nullopt;
    }

    //ansi string IS allocated, so we have to free ;)
    status = RtlUnicodeStringToAnsiString(&ansi, &uc, true);
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return std::nullopt;
    }

    //copy it over to an std::string
    result = ansi.Buffer;

    //and free
    RtlFreeAnsiString(&ansi);

    return result;
}

int main()
{
    std::optional<std::string> result = ResolveApiSet("api-ms-win-base-util-l1-1-0");
    if (!result)
        std::println("{:s}", std::system_category().message(GetLastError()));
    else
        std::println("{:s}", result.value());
}