#include "RegGr.h"

const std::wstring FINAL_STATE_CH = L"F";
const std::wstring EMPTY_STATE_CH = L"H";
const std::wstring END_SYMBOL_CH = L"ε";

using namespace std;

struct Grammar {
    bool isLeftType = false;
    vector<wstring> statesWith_ε_Input = {};
    wstring FirstState = L"";
    map<wstring, map<wstring, vector<wstring>>> Productions;
};

vector<wstring> CombineLinesByRules(const vector<wstring>& lines) {
    vector<wstring> combinedLines;
    wstring currentLine;

    for (const wstring& line : lines) {
        if (line.empty()) continue;

        if (regex_search(line, wregex(LR"(^\s*<(\w+)>\s*->)"))) {
            if (!currentLine.empty()) {
                combinedLines.push_back(currentLine);
            }
            currentLine = line;
        }
        else {
            currentLine += L" " + line;
        }
    }
    if (!currentLine.empty()) {
        combinedLines.push_back(currentLine);
    }
    return combinedLines;
}

vector<wstring> ReadGrammarFromFile(const string& filename) {
    wifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error: Unable to open file " << filename << endl;
        exit(1);
    }
    //file.imbue(std::locale("en_US.UTF-8"));
    file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
    vector<wstring> lines;
    wstring line;
    while (getline(file, line)) {
        lines.push_back(line);
    }
    file.close();
    vector<wstring> grammar = CombineLinesByRules(lines);
    return grammar;
}

std::wstring TrimString(const std::wstring& str) {
    // Символы, которые нужно удалить (пробелы, табуляции, символы конца строки)
    const std::wstring whitespace = L" \t\n\r";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::wstring::npos) {
        return L"";
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

bool CheckLeftGrammar(vector<wstring> rules) {
    bool isRightSided = true;
    bool isLeftSided = true;
    for (const auto& rule : rules) {
        std::vector<wstring> tokens;
        wistringstream iss(rule);
        wstring part;
        while (getline(iss, part, L'-')) {
            if (!part.empty()) tokens.push_back(part);
        }
        if (tokens.size() != 2) continue;

        wstring lhs = tokens[0];
        vector<wstring> rhs;
        wistringstream rhsIss(tokens[1]);
        wstring rhsPart;
        while (getline(rhsIss, rhsPart, L'|')) {
            rhs.push_back(rhsPart);
        }
        bool first = true;
        for (const auto& productionInRule : rhs) {
            wstring production = productionInRule;
            if (first) {
                production.erase(0, 2);
                first = false;
            }
            production = TrimString(production);
            if (regex_match(production, wregex(LR"(<\w+>.*)"))) {
                isRightSided = false;
            }
            if (regex_match(production, wregex(LR"(.*<\w+>)"))) {
                isLeftSided = false;
            }
        }
    }
    if (isLeftSided) {
        return true;
    }
    else if (isRightSided) {
        return false;
    }
    return true;
}

void ParseRightGrammar(const vector<wstring>& rules, Grammar& grammar) {
    wregex grammarPattern(LR"(^\s*<(\w+)>\s*->\s*([\wε](?:\s+<\w+>)?(?:\s*\|\s*[\wε](?:\s+<\w+>)?)*)\s*$)");
    wregex transitionPattern(LR"(^\s*([\wε]*)\s*(?:<(\w*)>)?\s*$)");
    for (const auto& line : rules) {
        wsmatch match;
        if (regex_match(line, match, grammarPattern)) {
            wstring state = match[1];
            if (grammar.FirstState.empty()) {
                grammar.FirstState = state;
            }
            vector<wstring> transitions;
            wistringstream iss(match[2]);
            wstring transition;
            while (getline(iss, transition, L'|')) {
                transitions.push_back(transition);
            }
            if (grammar.Productions.find(state) == grammar.Productions.end()) {
                grammar.Productions[state] = map<wstring, vector<wstring>>();
            }
            for (const auto& trans : transitions) {
                wsmatch transMatch;
                if (regex_match(trans, transMatch, transitionPattern)) {
                    wstring symbol = transMatch[1];
                    wstring nextState = transMatch[2].matched ? transMatch[2].str() : FINAL_STATE_CH;

                    if (grammar.Productions[state].find(symbol) == grammar.Productions[state].end()) {
                        grammar.Productions[state][symbol] = vector<wstring>();
                    }
                    grammar.Productions[state][symbol].push_back(nextState);
                    //wcout << nextState << " " << symbol << " " << state << "\n";
                }
            }
        }
    }
    grammar.Productions[FINAL_STATE_CH] = map<wstring, vector<wstring>>();
}

void ParseLeftGrammar(const vector<wstring>& rules, Grammar& grammar) {
    wregex grammarPattern(LR"(^\s*<(\w+)>\s*->\s*((?:<\w+>\s+)?[\wε](?:\s*\|\s*(?:<\w+>\s+)?[\wε])*)\s*$)");
    wregex transitionPattern(LR"(^\s*(?:<(\w*)>)?\s*([\wε]*)\s*$)");

    for (const auto& rule : rules) {
        wsmatch match;
        if (regex_match(rule, match, grammarPattern)) {
            wstring state = match[1];
            vector<wstring> transitions;
            wistringstream iss(match[2]);
            wstring transition;
            while (getline(iss, transition, L'|')) {
                transitions.push_back(transition);
            }
            if (grammar.Productions.find(state) == grammar.Productions.end()) {
                grammar.Productions[state] = map<wstring, vector<wstring>>();
            }
            for (const auto& trans : transitions) {
                wsmatch transMatch;
                if (regex_match(trans, transMatch, transitionPattern)) {
                    wstring symbol = transMatch[2];
                    wstring nextState = transMatch[1].matched ? transMatch[1].str() : EMPTY_STATE_CH;
                    /*wcout << nextState << " " << symbol << " " << state << "\n";*/
                    if (grammar.Productions.find(nextState) == grammar.Productions.end()) {
                        grammar.Productions[nextState] = map<wstring, vector<wstring>>();
                        grammar.Productions[nextState][symbol] = vector<wstring>{ state };
                        if (symbol == END_SYMBOL_CH) {
                            grammar.statesWith_ε_Input.push_back(state);
                        }
                        // для конечных состояний {{H, E}, lol}
                    }
                    else {
                        if (grammar.Productions[nextState].find(symbol) == grammar.Productions[nextState].end()) {
                            grammar.Productions[nextState][symbol] = vector<wstring>{ state };
                        }
                        else {
                            grammar.Productions[nextState][symbol].push_back(state);
                        }
                    }
                }
            }
        }
    }
}

void ExportToFile(Grammar grammar, const std::string& outputFileName) {
    // Берём начальное состояние
    wstring initialState;
    if (grammar.isLeftType) {
        initialState = EMPTY_STATE_CH;
    }
    else if (!grammar.isLeftType) {
        initialState = grammar.FirstState;
    }
    else {
        throw invalid_argument("Grammar does not have any productions.");
    }

    // Собираем все состояния, начиная с начального
    vector<wstring> states = { initialState };
    for (const auto& [state, _] : grammar.Productions) {
        if (state != initialState) {
            states.push_back(state);
        }   
    }

    // Собираем все символы из грамматики
    set<wstring> symbolsSet;
    for (const auto& [_, transitions] : grammar.Productions) {
        for (const auto& [symbol, _] : transitions) {
            symbolsSet.insert(symbol);
        }
    }
    vector<wstring> symbols(symbolsSet.begin(), symbolsSet.end());
    sort(symbols.begin(), symbols.end());

    // Создаем заголовки CSV
    vector<wstring> headerOfFinals = { L"" };
    vector<wstring> finalStates;
    if (grammar.isLeftType) {
        finalStates = grammar.Productions[EMPTY_STATE_CH][END_SYMBOL_CH];
        // собираем для левосторонней состояния без переходов с входом по ε
        for (const auto& state : grammar.statesWith_ε_Input) {
            if (grammar.Productions[state].empty()) {
                finalStates.push_back(state);
            }
        }
    }
    else {
        finalStates.push_back(FINAL_STATE_CH);
    }
    for (const auto& state : states) {
        auto it = std::find(finalStates.begin(), finalStates.end(), state);
        headerOfFinals.push_back(it != finalStates.end() ? L"F" : L"");
    }
    vector<wstring> header2 = { L"" };
    for (size_t i = 0; i < states.size(); ++i) {
        header2.push_back(L"q" + to_wstring(i));
    }
    // Создаем строки для CSV
    map<wstring, wstring> stateIndexMap;
    for (size_t i = 0; i < states.size(); ++i) {
        stateIndexMap[states[i]] = L"q" + to_wstring(i);
        //wcout << L"q" << to_wstring(i) << " = " << states[i] << "\n";
    }

    vector<vector<wstring>> rows;
    for (const auto& symbol : symbols) {
        vector<wstring> row = { symbol };
        for (const auto& state : states) {
            if (grammar.Productions.find(state) != grammar.Productions.end() &&
                grammar.Productions.at(state).find(symbol) != grammar.Productions.at(state).end()) {
                vector<wstring> nextStates = grammar.Productions.at(state).at(symbol);
                vector<wstring> mappedStates;
                for (const auto& nextState : nextStates) {
                    mappedStates.push_back(stateIndexMap[nextState]);
                }
                if (!mappedStates.empty()) {
                    row.push_back(accumulate(mappedStates.begin() + 1, mappedStates.end(), mappedStates[0], [](const wstring& a, const wstring& b) { return a + L"," + b; }));
                }
                else {
                    row.push_back(L"");
                }
            }
            else {
                row.push_back(L"");
            }
        }
        rows.push_back(row);
    }
    // Записываем в файл
    wofstream writer(outputFileName);
    if (!writer.is_open()) {
        throw runtime_error("Could not open file for writing.");
    }
    writer.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
    writer << accumulate(headerOfFinals.begin() + 1, headerOfFinals.end(), headerOfFinals[0], [](const wstring& a, const wstring& b) { return a + L";" + b; }) << endl;
    writer << accumulate(header2.begin() + 1, header2.end(), header2[0], [](const wstring& a, const wstring& b) { return a + L";" + b; }) << endl;
    for (const auto& row : rows) {
        writer << accumulate(row.begin() + 1, row.end(), row[0], [](const wstring& a, const wstring& b) { return a + L";" + b; }) << endl;
    }

    writer.close();
}

int main(int argc, char* argv[])
{
    string grammarFile = argv[1];
    string outputFile = argv[2];
   /* string grammarFile = "left_input_2.txt";
    string outputFile = "output.csv";*/
    //std::wcout.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));
    vector<wstring> input = ReadGrammarFromFile(grammarFile);
    Grammar grammar;
    grammar.isLeftType = CheckLeftGrammar(input);
    if (grammar.isLeftType) {
        ParseLeftGrammar(input, grammar);
    }
    else
    {
        ParseRightGrammar(input, grammar);
    }
    ExportToFile(grammar,outputFile);
    return 0;
}
