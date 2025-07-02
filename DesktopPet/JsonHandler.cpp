#include "JsonHandler.h"
#include <fstream>
#include <ctime>

JsonHandler::JsonHandler(const std::string& filePath) : filePath(filePath) {}

bool JsonHandler::LoadMemos() {
    memos.clear();

    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    Json::Value root;
    Json::CharReaderBuilder builder;
    JSONCPP_STRING errs;

    if (!Json::parseFromStream(builder, file, &root, &errs)) {
        return false;
    }

    for (const auto& memo : root) {
        MemoData data;
        data.content = memo["content"].asString();
        data.timestamp = memo["timestamp"].asInt64();
        memos.push_back(data);
    }

    return true;
}

bool JsonHandler::SaveMemos() {
    Json::Value root(Json::arrayValue);

    for (const auto& memo : memos) {
        Json::Value memoObj;
        memoObj["content"] = memo.content;
        memoObj["timestamp"] = (Json::Int64)memo.timestamp;
        root.append(memoObj);
    }

    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    Json::StreamWriterBuilder builder;
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    writer->write(root, &file);

    return true;
}

void JsonHandler::AddMemo(const std::string& content) {
    MemoData memo;
    memo.content = content;
    memo.timestamp = time(nullptr);
    memos.insert(memos.begin(), memo);
    SaveMemos();
}

void JsonHandler::DeleteMemo(int index) {
    if (index >= 0 && index < memos.size()) {
        memos.erase(memos.begin() + index);
        SaveMemos();
    }
}