#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/evp.h>
#include <sstream>
#include <unordered_map>
#include <vector>
using namespace std;

string computeSha1(const string &filePath) {
    ifstream input(filePath, ios_base::binary);
    if (!input.is_open()) {
        cout << "ОШИБКА: Не удалось открыть файл: " << filePath << endl;
        return "";
    }

    EVP_MD_CTX *hashCtx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(hashCtx, EVP_sha1(), nullptr);
    char buf[8192];
    
    while (input.read(buf, sizeof(buf)) || input.gcount() > 0) {
        EVP_DigestUpdate(hashCtx, buf, input.gcount());
    }

    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int digestLength = 0;
    EVP_DigestFinal_ex(hashCtx, digest, &digestLength);
    EVP_MD_CTX_free(hashCtx);
    ostringstream result;
    result << hex << setfill('0');
    
    for (unsigned int i = 0; i < digestLength; ++i)
        result << setw(2) << (int)digest[i];

    return result.str();
}

void collectFiles(vector<string> &fileList, const string &rootDir) {
    for (const auto &entry : filesystem::directory_iterator(rootDir)) {
        if (filesystem::is_directory(entry)) {
            collectFiles(fileList, entry.path().string());
        } else if (filesystem::is_regular_file(entry)) {
            fileList.push_back(entry.path().string());
        }
    }
}

int main() {
    vector<string> allFiles;
    collectFiles(allFiles, "test_dir");
    unordered_map<string, filesystem::path> seenHashes;
    cout << "Всего найдено файлов в каталоге: " << allFiles.size() << "\n\n";

    for (const auto &currentFile : allFiles) {
        cout << "Обрабатываем файл: " << currentFile << endl;
        string fileHash = computeSha1(currentFile);

        if (fileHash.empty()) {
            cout << "Пропуск - хеш не получен\n";
            continue;
        }

        cout << "SHA1: " << fileHash << endl;
        auto it = seenHashes.find(fileHash);

        if (it == seenHashes.end()) {
            seenHashes[fileHash] = filesystem::path(currentFile);
            cout << "Файл уникален\n";
        } 
        else {
            filesystem::path original = it->second;
            cout << "Найден дубликат\n";
            cout << "Оригинальный файл: " << original << endl;

            if (filesystem::equivalent(original, currentFile)) {
                cout << "Пропуск: файл уже является жёсткой ссылкой на оригинал.\n";
                continue;
            }
            cout << "Удаляем: " << currentFile << endl;
            filesystem::remove(currentFile);
            filesystem::create_hard_link(original, currentFile);
            cout << "Создана жёсткая ссылка: " << currentFile << " -> " << original << "\n" << endl;
        }
        cout << endl;
    }
    cout << "\nОбработка завершена\n";
    cout << "Уникальных файлов: " << seenHashes.size() << endl;
    return 0;
}
