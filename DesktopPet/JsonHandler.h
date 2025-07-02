#pragma once
#include <string>
#include <vector>
#include "include/json/json.h"

struct MemoData {
    std::string content;
    time_t timestamp;
};

class JsonHandler {
public:
    JsonHandler(const std::string& filePath);

    bool LoadMemos();
    bool SaveMemos();
    void AddMemo(const std::string& content);
    void DeleteMemo(int index);

    std::vector<MemoData> GetMemos() const { return memos; }

private:
    std::string filePath;
    std::vector<MemoData> memos;
};