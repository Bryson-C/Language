#include <iostream>
#include <vector>
#include <string>
#include <map>
#include "ctype.h"

#include "FileReader/FileReader.h"
#include "CLL.hpp"
#include "VariableHandler.hpp"

struct CLL_StringBuffer {
    std::vector<std::string> content;
    std::string buffer;

    void operator+=(char c) { buffer += c; }
    void operator+=(std::string str) { buffer += str; }
    void push() { if (!buffer.empty()) { content.push_back(buffer); buffer.clear(); } }
    void push(std::string str) { if (!str.empty()) { content.push_back(str); } }
};

// String Collectors:
std::vector<std::string> splitString(std::string& string, std::string pattern) {
    CLL_StringBuffer buffer;
    for (uint32_t i = 0; i < string.size(); i++) {
        for (int chr = 0, matches = 0, j = i; chr < pattern.size(); chr++, j++) {
            if (string[j] == pattern[chr]) matches++;
            if (matches == pattern.size()) buffer.push();
        }
        buffer += string[i];
    }
    buffer.push();
    return buffer.content;
}
std::vector<std::string> splitString(std::string& string, bool (*func)(char)) {
    CLL_StringBuffer buffer;

    int chr = 0;
    do {
        if (func(string[chr])) buffer.push();
        buffer.buffer += string[chr++];
    } while(chr < string.size());

    return buffer.content;
}
std::vector<std::string> removeWhiteSpace(std::vector<std::string>& strings) {
    for (int wrdIndex = 0; auto& wrd : strings) {
        if (wrd.empty()) { strings.erase(strings.begin()+wrdIndex); wrdIndex++; continue; }
        for (int index = 0; auto& chr : wrd) { if (isspace(chr)) wrd.erase(wrd.begin()+index); index++; }
        wrdIndex++;
    }
    return strings;
}


bool expects(std::vector<std::string>& vector, int index, std::vector<std::string> substr) {
    for (auto& i : vector)
        if (i != substr[0]) return false;
    return true;
}
bool isVarType(std::string str) {
    if (str == "Int") return true;
    return false;
}


#include <filesystem>

class Parser {
public:
    enum class type {
        IDENTIFIER,
        STRING,
        INTEGER,
        INITIALIZER,
        CONTAINER,
        SYMBOL,
        UNKNOWN,
    };
private:
    std::string content;
    std::vector<std::string> wordBuffer;
    std::vector<std::pair<uint32_t, type>> typeBuffer;
public:
    // Basic Initialization Of The Private Variables
    Parser() : wordBuffer({}), content("") {}
    // Reads The Given Path, Corrects The Path To Be Absolute
    Parser(const char* path) {
        // Correct Path To Be Absolute
        const char* correctedPath = std::filesystem::absolute(path).string().c_str();
        // Read File And Split The Words
        FileReader reader(correctedPath);
        content = reader.getChars();
        // Get The Words From The Word Buffer (Which Also Has The Line; Which Is Excluded)
        for (auto& wrd : reader.getWordBuffer()) wordBuffer.push_back(wrd.str);

        // TODO: Loop Through Word Buffer To Estimate The Type Of Data

    }
    // Checks The Previous And Next Words To Check Certain Parameters
    bool expects() {}
    //


    std::vector<std::string> getWordBuffer() { return wordBuffer; }
};



int main() {


    FileReader reader(R"(D:\Languages\CLL\data.lang)");
    auto wordBuffer = reader.getWordBuffer();

    struct Macro {
        using strvec = std::vector<std::string>;
        std::string activator;
        strvec format;
        strvec args;
    };

    // Values To Be Used During Compilation
    std::vector<Macro> macros;

    CLL_ScopedVariables compileTime;


    for (int i = 0; i < wordBuffer.size(); i++) {
        // We Skip `;` Because It Does Not Add To The Program
        if (wordBuffer[i].str == ";") continue;
        /*
        // TODO: Make Macros Record And Activate
        if (wordBuffer[i].str == "#") {
            Macro mc;
            i++;
            mc.activator = wordBuffer[i];
            std::cout << "New Macro: " << mc.activator << "\n";
            i++;
            if (wordBuffer[i].str == "[") {
                i++;
                while (wordBuffer[i].str != "]") mc.args.push_back(wordBuffer[i++].str);
                i++;
            } else {
                std::cerr << "Macro Requires Argument List After Activator\n";
            }

            if (wordBuffer[i].str == "=" && wordBuffer[i+1].str == ">") {
                i+=2;
                while (wordBuffer[i].str != ";") {
                    mc.format.push_back(wordBuffer[i++].str);
                }
                mc.format.push_back(";");
                macros.push_back(mc);
            } else {
                std::cerr << "Macro Expects That Following A Argument List The `=>` Operator Is Present\n";
            }
        }
        */

        if (isVarType(wordBuffer[i].str)) {
            std::string name;
            CLL_EVariableTypes type = CLL_StringToVarType(wordBuffer[i].str);
            i++;
            name = wordBuffer[i].str;
            i++;
            if (wordBuffer[i].str == ";") {
                compileTime.newScopedVariable(name, type, {});
                // We Are Done With This Variable
                continue;
            }
            if (wordBuffer[i].str == "=") {
                CLL_StringBuffer buffer;
                i++;
                while (wordBuffer[i].str != ";") {
                    if (type == CLL_EVariableTypes::String && isOperator(wordBuffer[i].str)) {
                        CLL_StdErr("String Operations Have Yet To Be Implemented", {CLL_StdLabels::Location}, {std::to_string(wordBuffer[i].line)});
                        //std::cerr << "String Operations Have Yet To Be Implemented\n  @: " << wordBuffer[i].line << "\n";
                        break;
                    }



                    if (wordBuffer[i].str == ",") {
                        buffer.push();
                        i++;
                        continue;
                    }
                    if (isOperator(wordBuffer[i+1].str)) {

                        std::string leftString = wordBuffer[i].str, rightSting = wordBuffer[i+2].str, OpString = wordBuffer[i+1].str;

                        auto result = CLL_PreformAutoOperation(compileTime, {leftString}, {rightSting}, OpString);
                        CLL_StdOut("Result Of Operation", {"Operation"}, {{leftString + " " + OpString + " " + rightSting}});
                        if (result.result != CLL_EOperationResult::Success) CLL_StdErr("Operation On Data Was Not Successful", {CLL_StdLabels::Cope}, {"Cannot Cope"});
                        for (auto& res : result.value) {
                            buffer.push(std::to_string(res));
                            std::cout << "  |: " << res << "\n";
                        }
                        i+=3;
                        continue;
                    }

                    CLL_VariableResult<std::string> variable = CLL_GetVariableOptionally(compileTime, wordBuffer[i].str);
                    if (variable.result == CLL_EVariableHandlerResult::VariablePresent) {
                        if (variable.variable.type != type)
                            CLL_StdOut("Variable Types Miss Match", {"Type Collision"},{{name + "Of Type " + CLL_VarTypeToString(type) + " And " + variable.variable.name + "Of Type " + CLL_VarTypeToString(variable.variable.type)}});

                        // TODO: This IF Statement Is For Getting Indices, I Still Need To Apply Single To Single Operations On Them
                        i++;
                        if (wordBuffer[i].str == "[") {
                            std::vector<size_t> indices;
                            // This Is To Skip The First `[`
                            i++;
                            while (wordBuffer[i].str != "]") {
                                if (isOperator(wordBuffer[i+1])) {
                                    auto result = CLL_PreformAutoOperation(compileTime, {wordBuffer[i]}, {wordBuffer[i+2]}, wordBuffer[i+1]);
                                    for (auto& resultIndex : result.value) {
                                        indices.push_back(resultIndex);
                                    }
                                    // We Only Need To Add 2 To The Iteration Because Once This If Statement Exits, The Iteration Will Go Up By 1
                                    i+=2;
                                } else if (wordBuffer[i].str == ",") {
                                    i++;
                                    continue;
                                } else if (isDigit(wordBuffer[i].str)) {
                                    indices.push_back(std::stoll(wordBuffer[i]));
                                } else
                                    CLL_StdErr("Unhandled Index Error", {CLL_StdLabels::Offender}, {wordBuffer[i].str});
                                i++;
                            }
                            // This Is To Skip The Second `]`
                            i++;
                            for (auto& index : indices) {
                                std::string pushedData;
                                if (index >= variable.variable.data.size() || index < 0) {
                                    CLL_StdErr("Index Is Out Of Bounds", {CLL_StdLabels::Offender, CLL_StdLabels::Index, CLL_StdLabels::Cope}, {variable.variable.name, std::to_string(index), "Using Int-64 Minimum"});
                                    pushedData = std::to_string(INT64_MIN);
                                } else {
                                    pushedData = variable.variable.data[index];
                                }
                                buffer.push(pushedData);
                            }
                            continue;
                        } else {
                            for (auto& vData : variable.variable.data)
                                buffer.push(vData);
                            i++;
                            continue;
                        }
                    }

                    buffer += wordBuffer[i].str;
                    i++;
                }
                buffer.push();
                compileTime.newScopedVariable(name, type, buffer.content);
                // We Are Done With This Variable
                continue;
            }

        } else {
            auto var = compileTime.getScopedVarByName(wordBuffer[i].str);
            if (var.result == CLL_EVariableHandlerResult::VariablePresent) {
                i++;
                if (wordBuffer[i].str == "?") {
                    CLL_StdOut("Variable Requested To Be Printed", {CLL_StdLabels::Variable},{var.variable.name});
                    if (!var.variable.data.empty()) {
                        for (auto &varval: var.variable.data)
                            std::cout << "  |: " << varval << "\n";
                    }
                    else {
                        CLL_StdErr("Variable Has No Values Therefore Will Not Be Printed",{CLL_StdLabels::OffendingVariable}, {var.variable.name});
                    }
                }
            } else {
                CLL_StdErr("Unknown Error", {CLL_StdLabels::Offender, CLL_StdLabels::Location}, {wordBuffer[i].str, std::to_string(wordBuffer[i].line)});
            }
        }


    }


    return 0;
}
