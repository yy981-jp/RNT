#pragma once

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <sstream>


inline void launchTextEditor(const std::string& editorPath, const std::string& args, const std::string& path) {
    // 寿命を保持するための文字列リスト
    std::vector<std::string> argList;
    argList.push_back(editorPath);

    // args が空でない場合、スペース区切りで分割して追加
    if (!args.empty()) {
        std::stringstream ss(args);
        std::string arg;
        while (ss >> arg) {
            argList.push_back(arg);
        }
    }

    argList.push_back(path);

    // SDL_CreateProcess 用に C 文字列ポインタの配列を作成
    std::vector<const char*> cm_args;
    for (const auto& arg : argList) {
        cm_args.push_back(arg.c_str());
    }
    cm_args.push_back(nullptr); // 終端ポインタ

    SDL_Process* process = SDL_CreateProcess(cm_args.data(), false);
    if (!process)
        throw std::runtime_error(std::string{"launchTextEditor(): "} + SDL_GetError());

    int status = -1;
    SDL_WaitProcess(process, true, &status);
    SDL_DestroyProcess(process);
}
