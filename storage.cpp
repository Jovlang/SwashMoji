#include <windows.h>
#include "storage.h"
#include "text.h"
#include <algorithm>
#include <atomic>
#include <charconv>
#include <fstream>
#include <limits>
#include <sstream>

namespace SwashMoji {
namespace {
constexpr size_t kMaxFileBytes = 4 * 1024 * 1024;
constexpr size_t kMaxRecordBytes = 16384;
constexpr size_t kMaxRecords = 50000;
constexpr unsigned int kVersion = 2;

bool Number(const std::string& text, unsigned int& result) {
    if (text.empty()) return false;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
    return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

std::string Escape(const std::wstring& value) {
    std::string result;
    for (char c : WideToUtf8(value)) {
        switch (c) {
        case '\\': result += "\\\\"; break;
        case '\t': result += "\\t"; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        default: result += c; break;
        }
    }
    return result;
}

bool Unescape(const std::string& value, std::wstring& result) {
    std::string bytes;
    for (size_t i = 0; i < value.size(); ++i) {
        char c = value[i];
        if (c == '\\') {
            if (++i == value.size()) return false;
            switch (value[i]) {
            case '\\': c = '\\'; break;
            case 't': c = '\t'; break;
            case 'n': c = '\n'; break;
            case 'r': c = '\r'; break;
            default: return false;
            }
        }
        if (c == '\0') return false;
        bytes += c;
    }
    result = Utf8ToWide(bytes);
    return bytes.empty() || !result.empty();
}

std::vector<std::string> Fields(const std::string& line) {
    std::vector<std::string> result;
    size_t start = 0;
    do {
        const size_t end = line.find('\t', start);
        result.push_back(line.substr(start, end - start));
        if (end == std::string::npos) break;
        start = end + 1;
    } while (true);
    return result;
}

enum class ReadStatus { Missing, Ok, Failed };
ReadStatus ReadFileBytes(const std::filesystem::path& path, std::string& bytes) {
    std::error_code error;
    const bool exists = std::filesystem::exists(path, error);
    if (error) return ReadStatus::Failed;
    if (!exists) return ReadStatus::Missing;
    const auto size = std::filesystem::file_size(path, error);
    if (error || size > kMaxFileBytes) return ReadStatus::Failed;
    std::ifstream file(path, std::ios::binary);
    if (!file) return ReadStatus::Failed;
    bytes.resize(static_cast<size_t>(size));
    if (!bytes.empty() && !file.read(bytes.data(), static_cast<std::streamsize>(bytes.size()))) return ReadStatus::Failed;
    // Catch a concurrently extended file rather than accepting a truncated read.
    return file.peek() == std::char_traits<char>::eof() && !file.bad() ? ReadStatus::Ok : ReadStatus::Failed;
}

bool SetSetting(Settings& settings, const std::string& name, const std::string& value) {
    unsigned int number{};
    if (!Number(value, number)) return false;
    if (name == "position_above_text_field" && number <= 1) settings.positionAboveTextField = number != 0;
    else if (name == "sort_by_usage" && number <= 1) settings.sortByUsage = number != 0;
    else if (name == "emoji_rows" && number >= 1 && number <= 3) settings.emojiRows = static_cast<int>(number);
    else if (name == "skin_tone" && number <= 5) settings.skinTone = static_cast<int>(number);
    else return false;
    return true;
}

Profile ImportLegacy(const std::filesystem::path& directory, const std::filesystem::path& fallback,
                     bool& failed, size_t& skipped) {
    Profile profile;
    for (const auto* name : {L"settings.txt", L"history.txt", L"usage.txt"}) {
        std::string bytes;
        auto status = ReadFileBytes(directory / name, bytes);
        if (status == ReadStatus::Missing && !fallback.empty()) status = ReadFileBytes(fallback / name, bytes);
        if (status == ReadStatus::Failed) { failed = true; continue; }
        if (status == ReadStatus::Missing) continue;
        std::istringstream input(bytes);
        std::string line;
        while (std::getline(input, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            bool valid = line.size() <= kMaxRecordBytes;
            if (valid && std::wstring(name) == L"settings.txt") {
                const auto equal = line.find('=');
                valid = equal != std::string::npos && SetSetting(profile.settings, line.substr(0, equal), line.substr(equal + 1));
            } else if (valid && std::wstring(name) == L"history.txt") {
                const auto glyph = Utf8ToWide(line);
                valid = !glyph.empty();
                if (valid && profile.history.size() < kMaxHistory) profile.history.push_back(glyph);
            } else if (valid) {
                const auto tab = line.find('\t');
                unsigned int count{};
                const auto glyph = Utf8ToWide(line.substr(0, tab));
                valid = tab != std::string::npos && !glyph.empty() && Number(line.substr(tab + 1), count) && count;
                if (valid) profile.usage[glyph] = count;
            }
            if (!valid) ++skipped;
        }
    }
    for (const auto& glyph : profile.history) {
        if (!profile.usage.count(glyph)) profile.usage[glyph] = 1;
    }
    return profile;
}

// Write-through temp files and same-directory rename leave either the previous
// complete destination or the new complete destination. Failed temps are removed.
bool AtomicWrite(const std::filesystem::path& destination, const std::string& bytes) {
    static std::atomic<unsigned long> sequence{};
    auto temporary = destination;
    temporary += L".tmp." + std::to_wstring(GetCurrentProcessId()) + L"." + std::to_wstring(++sequence);
    HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written{};
    bool ok = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr) && written == bytes.size();
    if (ok) ok = FlushFileBuffers(file) != FALSE;
    if (!CloseHandle(file)) ok = false;
    if (ok) ok = MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != FALSE;
    if (!ok) DeleteFileW(temporary.c_str());
    return ok;
}
}

std::string EncodeProfile(const Profile& profile) {
    std::vector<std::string> records{
        "setting\tposition_above_text_field\t" + std::to_string(profile.settings.positionAboveTextField),
        "setting\tsort_by_usage\t" + std::to_string(profile.settings.sortByUsage),
        "setting\temoji_rows\t" + std::to_string(profile.settings.emojiRows),
        "setting\tskin_tone\t" + std::to_string(profile.settings.skinTone)
    };
    for (const auto& glyph : profile.history) records.push_back("recent\t" + Escape(glyph));
    for (const auto& usage : profile.usage) records.push_back("usage\t" + Escape(usage.first) + "\t" + std::to_string(usage.second));
    for (const auto& entry : profile.aliases) {
        const auto& alias = entry.second;
        records.push_back("alias\t" + Escape(alias.phrase) + "\t" +
            (alias.target.kind == ResultKind::Emoji ? "emoji" : "combination") + "\t" + Escape(alias.target.value));
    }
    std::string bytes = "SwashMoji\t" + std::to_string(kVersion) + "\n";
    for (const auto& record : records) bytes += record + '\n';
    // A record count and mandatory final newline detect interrupted/truncated files.
    bytes += "end\t" + std::to_string(records.size()) + '\n';
    return bytes;
}

DecodedProfile DecodeProfile(const std::string& bytes) {
    DecodedProfile result;
    if (bytes.empty() || bytes.size() > kMaxFileBytes) return result;
    const auto firstLine = bytes.find('\n');
    auto headerLine = bytes.substr(0, firstLine);
    if (!headerLine.empty() && headerLine.back() == '\r') headerLine.pop_back();
    if (headerLine.compare(0, 3, "\xEF\xBB\xBF") == 0) headerLine.erase(0, 3);
    if (headerLine.size() > kMaxRecordBytes) return result;
    const auto header = Fields(headerLine);
    unsigned int version{};
    if (header.size() != 2 || header[0] != "SwashMoji" || !Number(header[1], version)) return result;
    result.version = version;
    if (version < 1 || version > kVersion) { result.format = ProfileFormat::Unsupported; return result; }
    if (firstLine == std::string::npos || bytes.back() != '\n') return result;
    size_t position = firstLine + 1;
    size_t recordCount = 0;
    while (position < bytes.size()) {
        const size_t end = bytes.find('\n', position);
        if (end == std::string::npos) return result;
        auto line = bytes.substr(position, end - position);
        position = end + 1;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.size() > kMaxRecordBytes) {
            if (++recordCount > kMaxRecords) return result;
            ++result.skippedRecords;
            continue;
        }
        const auto fields = Fields(line);
        unsigned int number{};
        if (fields[0] == "end") {
            if (fields.size() == 2 && Number(fields[1], number) && number == recordCount && position == bytes.size()) {
                result.format = ProfileFormat::Valid;
            }
            return result;
        }
        if (++recordCount > kMaxRecords) return result;
        bool valid = true;
        std::wstring glyph;
        if (valid && fields[0] == "setting") {
            valid = fields.size() == 3 && SetSetting(result.profile.settings, fields[1], fields[2]);
        } else if (valid && fields[0] == "recent") {
            valid = fields.size() == 2 && Unescape(fields[1], glyph) && !glyph.empty() && result.profile.history.size() < kMaxHistory;
            if (valid) result.profile.history.push_back(glyph);
        } else if (valid && fields[0] == "usage") {
            valid = fields.size() == 3 && Unescape(fields[1], glyph) && !glyph.empty() && Number(fields[2], number) && number;
            if (valid) result.profile.usage[glyph] = number;
        } else if (valid && fields[0] == "alias" && version >= 2) {
            std::wstring phrase, value;
            valid = fields.size() == 4 && Unescape(fields[1], phrase) && !phrase.empty() &&
                phrase.size() <= kMaxAliasLength && Unescape(fields[3], value) && !value.empty() &&
                (fields[2] == "emoji" || fields[2] == "combination") && result.profile.aliases.size() < kMaxAliases;
            const auto key = NormalizePhrase(phrase);
            valid = valid && !key.empty() && !result.profile.aliases.count(key);
            if (valid) result.profile.aliases[key] = {phrase, {fields[2] == "emoji" ? ResultKind::Emoji : ResultKind::Combination, value}};
        } else valid = false;
        if (!valid) ++result.skippedRecords;
    }
    return result;
}

ProfileStorage::ProfileStorage(std::filesystem::path directory, std::filesystem::path legacyDirectory)
    : directory_(std::move(directory)), legacyDirectory_(std::move(legacyDirectory)) {}

ProfileLoad ProfileStorage::Load() {
    ProfileLoad result;
    readOnly_ = false;
    std::string bytes;
    const auto primaryStatus = ReadFileBytes(directory_ / L"profile.tsv", bytes);
    const auto primary = primaryStatus == ReadStatus::Ok ? DecodeProfile(bytes) : DecodedProfile{};
    if (primary.format == ProfileFormat::Unsupported) {
        readOnly_ = true;
        result.diagnostic = L"Profile belongs to another version. Changes will not be saved.";
        return result;
    }
    if (primary.format == ProfileFormat::Valid) {
        result.profile = primary.profile;
        if (primary.version < kVersion) {
            std::wstring error;
            result.migrated = Save(result.profile, error);
            result.unsaved = !result.migrated;
            result.diagnostic = error;
        }
        if (primary.skippedRecords) result.diagnostic += L" Some invalid profile records were skipped.";
        return result;
    }

    const auto backupStatus = ReadFileBytes(directory_ / L"profile.tsv.bak", bytes);
    const auto backup = backupStatus == ReadStatus::Ok ? DecodeProfile(bytes) : DecodedProfile{};
    if (backup.format == ProfileFormat::Unsupported) {
        readOnly_ = true;
        result.diagnostic = L"Backup belongs to another version. Changes will not be saved.";
        return result;
    }
    if (backup.format == ProfileFormat::Valid) {
        result.profile = backup.profile;
        result.recovered = true;
        result.diagnostic = L"Profile recovered from the last complete backup.";
        std::wstring error;
        if (!Save(result.profile, error)) { result.unsaved = true; result.diagnostic += L" " + error; }
        return result;
    }
    if (primaryStatus != ReadStatus::Missing || backupStatus != ReadStatus::Missing) {
        readOnly_ = true;
        result.diagnostic = L"Profile could not be read. Existing files preserved; changes will not be saved.";
        return result;
    }
    bool failed = false;
    size_t skipped{};
    result.profile = ImportLegacy(directory_, legacyDirectory_, failed, skipped);
    if (failed) {
        readOnly_ = true;
        result.diagnostic = L"Legacy data could not be read. Changes will not be saved.";
        return result;
    }
    std::wstring error;
    result.migrated = Save(result.profile, error);
    result.unsaved = !result.migrated;
    result.diagnostic = error;
    if (skipped) result.diagnostic += L" Invalid legacy records were skipped.";
    return result;
}

bool ProfileStorage::Save(const Profile& profile, std::wstring& diagnostic) {
    diagnostic.clear();
    if (readOnly_) { diagnostic = L"Profile is read-only. Changes remain in memory."; return false; }
    const auto bytes = EncodeProfile(profile);
    const auto validation = DecodeProfile(bytes);
    if (validation.format != ProfileFormat::Valid || validation.skippedRecords || profile.aliases.size() > kMaxAliases) {
        diagnostic = L"Profile contains invalid or excessive data. Changes remain in memory.";
        return false;
    }
    std::error_code error;
    std::filesystem::create_directories(directory_, error);
    if (error) { diagnostic = L"Cannot create the profile directory. Changes remain in memory."; return false; }

    // Recheck disk before saving, including when an upgraded executable wrote it.
    std::string oldBytes;
    const auto oldStatus = ReadFileBytes(directory_ / L"profile.tsv", oldBytes);
    if (oldStatus == ReadStatus::Failed) {
        diagnostic = L"Cannot read the existing profile. Changes remain in memory.";
        return false;
    }
    const auto old = DecodeProfile(oldBytes);
    if (old.format == ProfileFormat::Unsupported) {
        readOnly_ = true;
        diagnostic = L"Profile belongs to another version. Changes remain in memory.";
        return false;
    }
    // Never replace a good backup with a corrupt primary recovered at startup.
    if (old.format == ProfileFormat::Valid && !old.skippedRecords &&
        !AtomicWrite(directory_ / L"profile.tsv.bak", oldBytes)) {
        diagnostic = L"Cannot save the profile backup. Changes remain in memory.";
        return false;
    }
    if (!AtomicWrite(directory_ / L"profile.tsv", bytes)) {
        diagnostic = L"Cannot save the profile. Changes remain in memory.";
        return false;
    }
    return true;
}
}
