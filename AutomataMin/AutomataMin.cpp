#include "AutomataMin.h"

using namespace std;

const string MEALY_PARAM = "mealy";
const string MOORE_PARAM = "moore";
const string BLANK_OUTPUT_CH = "_";
const string CLASS_CH = "X";
const bool NEED_INITIALIZATION = true;
const bool NOT_NEED_INITIALIZATION = false;

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator () (const std::pair<T1, T2>& pair) const {
        auto hash1 = std::hash<T1>{}(pair.first);
        auto hash2 = std::hash<T2>{}(pair.second);
        return hash1 ^ hash2;
    }
};

struct MooreAutomata {
    unordered_map<string, string> outputs;
    vector<vector<string>> transitions;
    vector<string> statesTable;
    vector<string> inputs;
};

struct MealyAutomata {
    vector<vector<pair<string, string>>> transitions;
    vector<string> statesTable;
    vector<string> inputs;
};

MooreAutomata ReadMoore(string& input_file) {
    MooreAutomata aut;
    ifstream file(input_file);
    string line;
    if (!file.is_open())
    {
        cerr << "Error: Could not open file " << input_file << endl;
        return aut;
    }
    // Чтение выходных символов
    getline(file, line);
    stringstream ss_outputs(line);
    string output;
    getline(ss_outputs, output, ';'); // Пропуск первого столбца
    vector<string> output_symbols;
    while (getline(ss_outputs, output, ';'))
    {
        output_symbols.push_back(output);
    }
    // Чтение имен состояний
    getline(file, line);
    stringstream ss_states(line);
    string state;
    getline(ss_states, state, ';'); // Пропуск первого столбца
    int stateIndex = 0;
    while (getline(ss_states, state, ';'))
    {
        aut.statesTable.push_back(state);
        aut.outputs[state] = output_symbols[stateIndex];
        stateIndex++;
    }
    // Чтение переходов
    while (getline(file, line))
    {
        stringstream ss(line);
        string input, transition;
        getline(ss, input, ';');
        aut.inputs.push_back(input);
        vector<string> state_transitions;
        while (getline(ss, transition, ';'))
        {
            state_transitions.push_back(transition);
        }
        aut.transitions.push_back(state_transitions);
    }
    return aut;
}

MealyAutomata ReadMealy(const string& input_file) {
    MealyAutomata mealyAutomata;
    ifstream file(input_file);
    if (!file.is_open()) {
        cerr << "Error: Could not open file " << input_file << endl;
        return mealyAutomata;
    }
    string line;
    // Чтение заголовка (состояния)
    getline(file, line);
    stringstream ss_states(line);
    string state;
    getline(ss_states, state, ';'); // Пропуск первого столбца
    while (getline(ss_states, state, ';')) {
        mealyAutomata.statesTable.push_back(state);
    }
    // Чтение переходов
    while (getline(file, line)) {
        stringstream ss(line);
        string input, transition;
        getline(ss, input, ';');
        mealyAutomata.inputs.push_back(input);
        vector<pair<string, string>> mealyTransitions;
        while (getline(ss, transition, ';')) {
            string nextState = transition.substr(0, transition.find('/'));
            string output = transition.substr(transition.find('/') + 1);
            mealyTransitions.push_back({ nextState, output });
        }
        mealyAutomata.transitions.push_back(mealyTransitions);
    }
    return mealyAutomata;
}

void ExportMooreToCSV(MooreAutomata automata, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return;
    }
    // outputs
    for (const auto& state : automata.statesTable) {
        if (automata.outputs[state] == BLANK_OUTPUT_CH) {
            file << ";";
        }
        else
        {
            file << ";" << automata.outputs[state];
        }
    }
    file << endl;
    for (const auto& state : automata.statesTable) {
        file << ";" << state;
    }
    file << endl;
    // inputs and transitions
    int inputIndex = 0;
    auto statesQual = automata.statesTable.size();
    for (const string& input : automata.inputs) {
        file << input << ";";
        for (int stateIndex = 0; stateIndex < automata.statesTable.size(); stateIndex++) {
            string nextState = automata.transitions[inputIndex][stateIndex];
            file << nextState;
            if (stateIndex != automata.statesTable.size() - 1) {
                file << ";";
            }
        }
        file << endl;
        inputIndex++;
    }
    file.close();
}

void ExportMealyToCSV(MealyAutomata automata, const string& filename) {
    ofstream file(filename);
    if (!file.is_open()) {
        cerr << "Failed to open file: " << filename << endl;
        return;
    }
    for (const auto& state : automata.statesTable) {
        file << ";" << state;
    }
    file << endl;
    int inputIndex = 0;
    auto statesQual = automata.statesTable.size();
    for (const string& input : automata.inputs) {
        file << input << ";";
        for (int stateIndex = 0; stateIndex < automata.statesTable.size(); stateIndex++) {
            auto nextState = automata.transitions[inputIndex][stateIndex];
            file << nextState.first << "/" << nextState.second;
            if (stateIndex != automata.statesTable.size() - 1) {
                file << ";";
            }
        }
        file << endl;
        inputIndex++;
    }
    file.close();
}

MooreAutomata RemoveUnreachableStatesMoore(MooreAutomata automata) {
    unordered_set<string> reachableStates;
    queue<string> toVisit;
    MooreAutomata procAut;
    procAut.outputs = automata.outputs;
    procAut.inputs = automata.inputs;
    procAut.transitions.resize(procAut.inputs.size());

    // Начинаем с начального состояния
    string startState = automata.statesTable[0];
    toVisit.push(startState);
    reachableStates.insert(startState);

    // Обход в ширину для поиска всех достижимых состояний
    while (!toVisit.empty()) {
        string currentState = toVisit.front();
        toVisit.pop();
        int stateIndex = 0;
        // Находим индекс текущего состояния
        for (const auto& state : automata.statesTable)
        {
            if (currentState == state)
            {
                break;
            }
            stateIndex++;
        }
        // Проходим по всем входным символам
        for (size_t i = 0; i < automata.transitions.size(); i++) {
            if (stateIndex >= automata.transitions[i].size()) {
                cerr << "Error: State index out of range in transitions matrix." << endl;
                continue;
            }
            string nextState = automata.transitions[i][stateIndex];
            if (reachableStates.find(nextState) == reachableStates.end()) {
                reachableStates.insert(nextState);
                toVisit.push(nextState);
            }
        }
    }

    // Удаляем недостижимые состояния
    vector<pair<string, int>>unreachableStatesWithIndex;
    int currentStateIndex = 0;
    for (const auto& state : automata.statesTable) {
        if (reachableStates.find(state) == reachableStates.end()) {
            unreachableStatesWithIndex.push_back({ state, currentStateIndex });
        }
        currentStateIndex++;
    }
    currentStateIndex = 0;
    for (const auto& state : unreachableStatesWithIndex) {
        procAut.outputs.erase(state.first);
        for (int stateIndex = currentStateIndex; stateIndex < automata.statesTable.size(); stateIndex++) {
            currentStateIndex++;
            if (automata.statesTable[stateIndex] == state.first) {
                break;
            }
            else {
                procAut.statesTable.push_back(automata.statesTable[stateIndex]);
                for (int input = 0; input < automata.transitions.size(); input++) {
                    procAut.transitions[input].push_back(automata.transitions[input][stateIndex]);
                }
            }
        }
    }
    if (currentStateIndex < automata.statesTable.size()) {
        for (int stateIndex = currentStateIndex; stateIndex < automata.statesTable.size(); stateIndex++) {
            procAut.statesTable.push_back(automata.statesTable[stateIndex]);
            for (int input = 0; input < automata.transitions.size(); input++) {
                procAut.transitions[input].push_back(automata.transitions[input][stateIndex]);
            }
        }
    }

    return procAut;
}

MealyAutomata RemoveUnreachableStatesMealy(MealyAutomata automata)
{
    unordered_set<string> reachableStates;
    queue<string> toVisit;
    MealyAutomata procAut;
    procAut.inputs = automata.inputs;
    procAut.transitions.resize(procAut.inputs.size());

    // Начинаем с начального состояния
    string startState = automata.statesTable[0];
    toVisit.push(startState);
    reachableStates.insert(startState);

    // Обход в ширину для поиска всех достижимых состояний
    while (!toVisit.empty()) {
        string currentState = toVisit.front();
        toVisit.pop();

        int stateIndex = 0;
        // Находим индекс текущего состояния
        for (const auto& state : automata.statesTable)
        {
            if (currentState == state)
            {
                break;
            }
            stateIndex++;
        }

        // Проходим по всем входным символам
        for (size_t i = 0; i < automata.transitions.size(); i++) {
            if (stateIndex >= automata.transitions[i].size()) {
                cerr << "Error: State index out of range in transitions matrix." << endl;
                continue;
            }
            string nextState = automata.transitions[i][stateIndex].first;
            if (reachableStates.find(nextState) == reachableStates.end()) {
                reachableStates.insert(nextState);
                toVisit.push(nextState);
            }
        }
    }

    // Удаляем недостижимые состояния
    vector<pair<string, int>>unreachableStatesWithIndex;
    int currentStateIndex = 0;
    for (const auto& state : automata.statesTable) {
        if (reachableStates.find(state) == reachableStates.end()) {
            unreachableStatesWithIndex.push_back({ state, currentStateIndex });
        }
        currentStateIndex++;
    }
    currentStateIndex = 0;
    for (const auto& state : unreachableStatesWithIndex) {
        for (int stateIndex = currentStateIndex; stateIndex < automata.statesTable.size(); stateIndex++) {
            currentStateIndex++;
            if (automata.statesTable[stateIndex] == state.first) {
                break;
            }
            else {
                procAut.statesTable.push_back(automata.statesTable[stateIndex]);
                for (int input = 0; input < automata.transitions.size(); input++) {
                    procAut.transitions[input].push_back(automata.transitions[input][stateIndex]);
                }
            }
        }
    }
    if (currentStateIndex < automata.statesTable.size()) {
        for (int stateIndex = currentStateIndex; stateIndex < automata.statesTable.size(); stateIndex++) {
            procAut.statesTable.push_back(automata.statesTable[stateIndex]);
            for (int input = 0; input < automata.transitions.size(); input++) {
                procAut.transitions[input].push_back(automata.transitions[input][stateIndex]);
            }
        }
    }
    return procAut;
}

void LookForSameState(vector<pair<string, string>> nextStates, int statesToCheck, vector<string>& statesTable, map<string, string>& classTable, vector<vector<pair<string, string>>> transitions,
    string stateName, int& classNum, vector<string> autStates, map<string, string>& newClassTable, MealyAutomata& aut) {
    bool same = false;
    for (int state = 0; state < statesToCheck; state++) {
        for (int input = 0; input < transitions.size(); input++) {
            if (classTable[nextStates[input].first] != classTable[transitions[input][state].first] || classTable[stateName] != classTable[aut.statesTable[state]]) {
                break;
            }
            same = true;
        }
        if (same) {
            newClassTable[stateName] = statesTable[state];
            break;
        }
    }
    if (!same) {
        classNum++;
        string newClass = CLASS_CH + to_string(classNum);
        newClassTable[stateName] = newClass;
        statesTable.push_back(newClass);
    }
}

void LookForSameOutputs(vector<pair<string, string>> outputs, int statesToCheck, vector<string>& statesTable, map<string, string>& classTable, vector<vector<pair<string, string>>> transitions,
    string stateName, int& classNum, vector<string> autStates) {
    bool same = false;
    for (int state = 0; state < statesToCheck; state++) {
        for (int input = 0; input < transitions.size(); input++) {
            if (outputs[input].second != transitions[input][state].second) {
                break;
            }
            same = true;
        }
        if (same) {
            classTable[stateName] = statesTable[state];
            break;
        }
    }
    if (!same) {
        classNum++;
        string newClass = CLASS_CH + to_string(classNum);
        classTable[stateName] = newClass;
        statesTable.push_back(newClass);
    }
}

map<string, string> GetClassTable(MealyAutomata aut, vector<string>& statesTable, map<string, string> currClassTable, bool needInitialization) {
    map<string, string> classTable;
    int classNum = 0;
    string newClass = CLASS_CH + to_string(classNum);
    classTable[aut.statesTable[0]] = newClass;
    statesTable.push_back(newClass);
    for (int state = 1; state < aut.transitions[0].size(); state++) {
        vector<pair<string, string>> ObjectsToCompareInState;
        for (int input = 0; input < aut.transitions.size(); input++) {
            ObjectsToCompareInState.push_back(aut.transitions[input][state]);
        }
        if (needInitialization) {
            LookForSameOutputs(ObjectsToCompareInState, state, statesTable, classTable, aut.transitions, aut.statesTable[state], classNum, aut.statesTable);
        }
        else {
            LookForSameState(ObjectsToCompareInState, state, statesTable, currClassTable, aut.transitions, aut.statesTable[state], classNum, aut.statesTable, classTable, aut);
        }
    }
    return classTable;
}

vector<vector<pair<string, string>>> GetNewTransitionTable(map<string, string> classTable, MealyAutomata& aut) {
    vector<vector<pair<string, string>>> newTransitionTable;
    int size = aut.inputs.size();
    newTransitionTable.resize(size);
    set<string> addedClasses;
    for (int state = 0; state < aut.transitions[0].size(); state++) {
        if (addedClasses.find(classTable[aut.statesTable[state]]) == addedClasses.end())
        {
            for (int input = 0; input < aut.transitions.size(); input++) {
                addedClasses.insert(classTable[aut.statesTable[state]]);
                string nextState = aut.transitions[input][state].first;
                string output = aut.transitions[input][state].second;
                //cout << nextState << "/" << output;
                pair<string, string> newTransition = { classTable[nextState], output };
                newTransitionTable[input].push_back(newTransition);

            }
        }
        if (addedClasses.size() == classTable.size()) {
            break;
        }
    }
    return newTransitionTable;
}

MealyAutomata MinimizeMealy(MealyAutomata automata) {
    MealyAutomata minAut;
    minAut.inputs = automata.inputs;
    map<string, string> currClassTable;
    map<string, string> classTable = GetClassTable(automata, minAut.statesTable, currClassTable, NEED_INITIALIZATION);
    vector<string> statesTable;
    bool canBeMinimized = true;
    while (canBeMinimized) {
        vector<string> currStatesTable;
        currClassTable = GetClassTable(automata, currStatesTable, classTable, NOT_NEED_INITIALIZATION);
        if (statesTable == currStatesTable) {
            canBeMinimized = false;
        }
        else {
            statesTable = currStatesTable;
        }
        if (!canBeMinimized) {
            minAut.transitions = GetNewTransitionTable(currClassTable, automata);
            minAut.statesTable = currStatesTable;
        }
    }
    return minAut;
}

map<string, string> InitilizeClassTable(MooreAutomata aut, vector<string>& statesTable, unordered_map<string, string>& outputs) {
    map<string, string> classTable;
    unordered_set<string> usedOutputs;
    map<string, string> outputWithClass;
    int classNum = 0;
    for (auto& state : aut.statesTable) {
        if (usedOutputs.find(aut.outputs[state]) == usedOutputs.end()) {
            string newClass = CLASS_CH + to_string(classNum);
            classTable[state] = newClass;
            outputs[newClass] = aut.outputs[state];
            outputWithClass[aut.outputs[state]] = newClass;
            statesTable.push_back(newClass);
            classNum++;
            usedOutputs.insert(aut.outputs[state]);
        }
        else {
            classTable[state] = outputWithClass[aut.outputs[state]];
        }
    }
    return classTable;
}

map<string, string> GetClassTableForMoore(MooreAutomata aut, map<string, string>currClassTable, vector<string>& statesTable, unordered_map<string, string>& outputs) {
    map<string, string> classTable;
    map<vector<string>, string> pastTransWithClass;
    int classNum = 0;
    for (int state = 0; state < aut.transitions[0].size(); state++) {
        vector<string> transitionsInState;
        for (int input = 0; input < aut.transitions.size(); input++) {
            transitionsInState.push_back(currClassTable[aut.transitions[input][state]]);
        }
        transitionsInState.push_back(currClassTable[aut.statesTable[state]]);
        if (pastTransWithClass.count(transitionsInState) == 0) {
            string newClass = CLASS_CH + to_string(classNum);
            classTable[aut.statesTable[state]] = newClass;
            outputs[newClass] = aut.outputs[aut.statesTable[state]];
            pastTransWithClass[transitionsInState] = newClass;
            statesTable.push_back(newClass);
            classNum++;
        }
        else {
            classTable[aut.statesTable[state]] = pastTransWithClass[transitionsInState];
        }
    }
    return classTable;
}

vector<vector<string>> GetNewTransitionTableForMoore(MooreAutomata aut, map<string, string> classTable) {
    vector<vector<string>> newTransitionTable;
    int size = aut.inputs.size();
    newTransitionTable.resize(size);
    set<string> addedClasses;
    for (int state = 0; state < aut.transitions[0].size(); state++) {
        if (addedClasses.find(classTable[aut.statesTable[state]]) == addedClasses.end())
        {
            for (int input = 0; input < aut.transitions.size(); input++) {
                addedClasses.insert(classTable[aut.statesTable[state]]);
                string nextState = aut.transitions[input][state];
                string newTransition = classTable[nextState];
                newTransitionTable[input].push_back(newTransition);

            }
        }
        if (addedClasses.size() == classTable.size()) {
            break;
        }
    }
    return newTransitionTable;
}

MooreAutomata MinimizeMoore(MooreAutomata automata) {
    MooreAutomata minAut;
    minAut.inputs = automata.inputs;
    unordered_map <string, string> outputs;
    vector<string> statesTable;
    map<string, string> currClassTable;
    map<string, string> classTable = InitilizeClassTable(automata, statesTable, outputs);
    bool canBeMinimized = true;
    while (canBeMinimized) {
        vector<string> currStatesTable;
        currClassTable = GetClassTableForMoore(automata, classTable, currStatesTable, outputs);
        if (currStatesTable == statesTable) {
            canBeMinimized = false;
            minAut.statesTable = currStatesTable;
        }
        else
        {
            classTable = currClassTable;
        }
    }
    minAut.transitions = GetNewTransitionTableForMoore(automata, classTable);
    minAut.outputs = outputs;
    return minAut;
}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        cerr << "Usage: " << "<work param> <input_file> <output_file>" << endl;
        return 1;
    }
    string workParam = argv[1];
    string inputFile = argv[2];
    string outputFile = argv[3];
    if (workParam != MEALY_PARAM && workParam != MOORE_PARAM)
    {
        cerr << "Wrong param" << endl;
        return 1;
    }
    if (workParam == MEALY_PARAM) {
        MealyAutomata mealyAut = ReadMealy(inputFile);
        mealyAut = RemoveUnreachableStatesMealy(mealyAut);
        mealyAut = MinimizeMealy(mealyAut);
        ExportMealyToCSV(mealyAut, outputFile);
    }
    else
    {
        if (workParam == MOORE_PARAM) {
            MooreAutomata aut = ReadMoore(inputFile);
            aut = RemoveUnreachableStatesMoore(aut);
            aut = MinimizeMoore(aut);
            ExportMooreToCSV(aut, outputFile);
        }
    }
    return 0;
}
