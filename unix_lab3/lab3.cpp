#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include <openssl/evp.h>
#include <exception>
#include <iomanip>
#include <sstream>
using namespace std;

string computeSha1(const string& filePath) {
    ifstream input(filePath, ios_base::binary);
    if (!input.is_open()) {
        cout << "Не удалось открыть файл: " << filePath << endl;
        return "";
    }

    EVP_MD_CTX* hashCtx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(hashCtx, EVP_sha1(), nullptr);

    char buf[8192];
    while (input.read(buf, sizeof(buf)) || input.gcount() > 0) {
        EVP_DigestUpdate(hashCtx, buf, input.gcount());
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength = 0;
    EVP_DigestFinal_ex(hashCtx, digest, &digestLength);

    ostringstream result;
    result << hex << setfill('0');
    for (unsigned int i = 0; i < digestLength; ++i)
        result << setw(2) << (int)digest[i];

    return result.str();
}

void collectFiles(vector<string>& fileList, const string& rootDir) {
    for (const auto& entry : filesystem::directory_iterator(rootDir)) {
        try {
            if (filesystem::is_directory(entry)) {
                collectFiles(fileList, entry.path().string());
            } else if (filesystem::is_regular_file(entry)) {
                fileList.push_back(entry.path().string());
            }
        } catch (const filesystem::filesystem_error& e) {
            cout << "Ошибка при обходе каталога: " << e.what() << endl;
        }
    }
}

int main() {
    vector<string> allFiles;
    collectFiles(allFiles, "test_dir");

    unordered_map<string, filesystem::path> seenHashes;

    for (const auto& currentFile : allFiles) {
        try {
            string fileHash = computeSha1(currentFile);
            if (fileHash.empty())
                continue;

            auto it = seenHashes.find(fileHash);

            if (it == seenHashes.end()) {
                seenHashes[fileHash] = filesystem::path(currentFile);
            } else {
                filesystem::path original = it->second;

                if (filesystem::equivalent(original, currentFile))
                    continue;

                cout << "Удаляем: " << currentFile << endl;
                filesystem::remove(currentFile);

                filesystem::create_hard_link(original, currentFile);
                cout << "Создана жёсткая ссылка: " << currentFile
                     << " → " << original << endl;
            }
        } catch (std::exception& e) {
            cout << "Ошибка: " << e.what() << endl;
        }
    }
}
