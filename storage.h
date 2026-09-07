#pragma once
#include "personalization.h"
#include <filesystem>

namespace SwashMoji {
enum class ProfileFormat { Valid, Invalid, Unsupported };
struct DecodedProfile {
    ProfileFormat format{ProfileFormat::Invalid};
    Profile profile;
    size_t skippedRecords{};
    unsigned int version{};
};

std::string EncodeProfile(const Profile& profile);
DecodedProfile DecodeProfile(const std::string& bytes);

struct ProfileLoad {
    Profile profile;
    bool migrated{};
    bool recovered{};
    bool unsaved{};
    std::wstring diagnostic;
};

class ProfileStorage {
public:
    explicit ProfileStorage(std::filesystem::path directory = {}, std::filesystem::path legacyDirectory = {});
    ProfileLoad Load();
    bool Save(const Profile& profile, std::wstring& diagnostic);
    bool ReadOnly() const { return readOnly_; }
private:
    std::filesystem::path directory_, legacyDirectory_;
    bool readOnly_{};
};
}
